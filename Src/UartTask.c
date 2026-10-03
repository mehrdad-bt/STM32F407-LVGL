#include "UartTask.h"

#include "main.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "MainPage.h"


/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */

#define UART_RX_BUFFER_SIZE      64U
#define UART_PACKET_QUEUE_SIZE   2U
#define UART_TIMEOUT_MS          500U


/* -------------------------------------------------------------------------- */
/* External UART                                                              */
/* -------------------------------------------------------------------------- */

extern UART_HandleTypeDef huart1;


/* -------------------------------------------------------------------------- */
/* RX Variables                                                               */
/* -------------------------------------------------------------------------- */

static uint8_t uart_rx_byte = 0U;

static char uart_packet_queue[
    UART_PACKET_QUEUE_SIZE
][UART_RX_BUFFER_SIZE];

static volatile uint8_t uart_rx_index = 0U;

static volatile uint8_t uart_write_index = 0U;

static volatile uint8_t uart_read_index = 0U;

static volatile uint8_t uart_packet_count = 0U;


/* -------------------------------------------------------------------------- */
/* Connection State                                                           */
/* -------------------------------------------------------------------------- */

static uint32_t uart_last_valid_packet_time = 0U;

static bool uart_link_active = false;


/* -------------------------------------------------------------------------- */
/* Parse Packet                                                               */
/* -------------------------------------------------------------------------- */

static bool UART_ParsePacket(
    const char *buffer,
    float *voltage,
    float *current
)
{
    char *end_ptr;

    const char *current_start;

    float parsed_voltage;
    float parsed_current;


    if (
        buffer == NULL ||
        voltage == NULL ||
        current == NULL
    )
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Parse Voltage                                                           */
    /* ---------------------------------------------------------------------- */

    parsed_voltage =
        strtof(
            buffer,
            &end_ptr
        );


    if (end_ptr == buffer)
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Reject NaN                                                              */
    /* ---------------------------------------------------------------------- */

    if (
        parsed_voltage !=
        parsed_voltage
    )
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Comma                                                                   */
    /* ---------------------------------------------------------------------- */

    if (*end_ptr != ',')
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Parse Current                                                           */
    /* ---------------------------------------------------------------------- */

    current_start =
        end_ptr + 1;


    parsed_current =
        strtof(
            current_start,
            &end_ptr
        );


    if (end_ptr == current_start)
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Reject NaN                                                              */
    /* ---------------------------------------------------------------------- */

    if (
        parsed_current !=
        parsed_current
    )
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Allow CR and whitespace                                                 */
    /* ---------------------------------------------------------------------- */

    while (
        *end_ptr == ' '  ||
        *end_ptr == '\t' ||
        *end_ptr == '\r'
    )
    {
        end_ptr++;
    }


    /* ---------------------------------------------------------------------- */
    /* Nothing else allowed                                                    */
    /* ---------------------------------------------------------------------- */

    if (*end_ptr != '\0')
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Voltage range                                                           */
    /* ---------------------------------------------------------------------- */

    if (
        parsed_voltage < 0.0f ||
        parsed_voltage > 30.0f
    )
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Current range                                                           */
    /* ---------------------------------------------------------------------- */

    if (
        parsed_current < 0.0f ||
        parsed_current > 3.0f
    )
    {
        return false;
    }


    /* ---------------------------------------------------------------------- */
    /* Valid                                                                    */
    /* ---------------------------------------------------------------------- */

    *voltage =
        parsed_voltage;

    *current =
        parsed_current;


    return true;
}


/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

void UartTask_Init(void)
{
    uart_rx_byte = 0U;

    uart_rx_index = 0U;

    uart_write_index = 0U;

    uart_read_index = 0U;

    uart_packet_count = 0U;

    uart_last_valid_packet_time = 0U;

    uart_link_active = false;


    if (
        HAL_UART_Receive_IT(
            &huart1,
            &uart_rx_byte,
            1U
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/* -------------------------------------------------------------------------- */
/* Run                                                                        */
/* -------------------------------------------------------------------------- */

void UartTask_Run(void)
{
    char packet[UART_RX_BUFFER_SIZE];

    float voltage;

    float current;


    /* ---------------------------------------------------------------------- */
    /* Process Packet                                                          */
    /* ---------------------------------------------------------------------- */

    if (uart_packet_count > 0U)
    {
        /*
         * Protect queue access while copying packet.
         */

        __disable_irq();


        memcpy(
            packet,
            uart_packet_queue[uart_read_index],
            UART_RX_BUFFER_SIZE
        );


        uart_read_index++;


        if (
            uart_read_index >=
            UART_PACKET_QUEUE_SIZE
        )
        {
            uart_read_index = 0U;
        }


        uart_packet_count--;


        __enable_irq();


        /* ------------------------------------------------------------------ */
        /* Validate Packet                                                      */
        /* ------------------------------------------------------------------ */

        if (
            UART_ParsePacket(
                packet,
                &voltage,
                &current
            )
        )
        {
            /* -------------------------------------------------------------- */
            /* Valid packet                                                     */
            /* -------------------------------------------------------------- */

            uart_last_valid_packet_time =
                HAL_GetTick();

            uart_link_active = true;


            /* -------------------------------------------------------------- */
            /* Send packet to MainPage                                          */
            /* -------------------------------------------------------------- */

            MainPage_OnUartPacket(
                voltage,
                current
            );
        }
    }


    /* ---------------------------------------------------------------------- */
    /* UART Timeout                                                            */
    /* ---------------------------------------------------------------------- */

    if (uart_link_active)
    {
        if (
            (
                HAL_GetTick() -
                uart_last_valid_packet_time
            ) >= UART_TIMEOUT_MS
        )
        {
            uart_link_active = false;

            MainPage_OnUartTimeout();
        }
    }
}


/* -------------------------------------------------------------------------- */
/* RX Complete Callback                                                       */
/* -------------------------------------------------------------------------- */

void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance != USART1)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* End Of Packet                                                           */
    /* ---------------------------------------------------------------------- */

    if (uart_rx_byte == '\n')
    {
        if (
            uart_packet_count <
            UART_PACKET_QUEUE_SIZE
        )
        {
            uart_packet_queue[
                uart_write_index
            ][
                uart_rx_index
            ] = '\0';


            uart_write_index++;


            if (
                uart_write_index >=
                UART_PACKET_QUEUE_SIZE
            )
            {
                uart_write_index = 0U;
            }


            uart_packet_count++;
        }


        uart_rx_index = 0U;
    }


    /* ---------------------------------------------------------------------- */
    /* Ignore CR                                                               */
    /* ---------------------------------------------------------------------- */

    else if (uart_rx_byte == '\r')
    {
        /*
         * CR is ignored.
         */
    }


    /* ---------------------------------------------------------------------- */
    /* Store Normal Byte                                                       */
    /* ---------------------------------------------------------------------- */

    else
    {
        if (
            uart_packet_count ==
            0U
        )
        {
            if (
                uart_rx_index <
                UART_RX_BUFFER_SIZE - 1U
            )
            {
                uart_packet_queue[
                    uart_write_index
                ][
                    uart_rx_index
                ] =
                    (char)uart_rx_byte;


                uart_rx_index++;
            }
            else
            {
                /*
                 * Packet too long.
                 */

                uart_rx_index = 0U;
            }
        }
    }


    /* ---------------------------------------------------------------------- */
    /* Receive Next Byte                                                       */
    /* ---------------------------------------------------------------------- */

    if (
        HAL_UART_Receive_IT(
            &huart1,
            &uart_rx_byte,
            1U
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/* -------------------------------------------------------------------------- */
/* UART Error Callback                                                        */
/* -------------------------------------------------------------------------- */

void HAL_UART_ErrorCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance != USART1)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Reset RX                                                                */
    /* ---------------------------------------------------------------------- */

    uart_rx_index = 0U;

    uart_write_index = 0U;

    uart_read_index = 0U;

    uart_packet_count = 0U;


    /* ---------------------------------------------------------------------- */
    /* Restart RX                                                              */
    /* ---------------------------------------------------------------------- */

    HAL_UART_Receive_IT(
        &huart1,
        &uart_rx_byte,
        1U
    );
}
