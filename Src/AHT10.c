#include "AHT10.h"


/* -------------------------------------------------------------------------- */
/* AHT10                                                                      */
/* -------------------------------------------------------------------------- */

#define AHT10_ADDRESS                  (0x38U << 1)

#define AHT10_CMD_RESET                0xBAU

#define AHT10_CMD_INITIALIZE           0xE1U
#define AHT10_INIT_DATA_1              0x08U
#define AHT10_INIT_DATA_2              0x00U

#define AHT10_CMD_TRIGGER              0xACU
#define AHT10_TRIGGER_DATA_1           0x33U
#define AHT10_TRIGGER_DATA_2           0x00U


/* -------------------------------------------------------------------------- */
/* Timing                                                                     */
/* -------------------------------------------------------------------------- */

#define AHT10_STARTUP_DELAY_MS         100U
#define AHT10_MEASURE_INTERVAL_MS      2000U
#define AHT10_MEASURE_WAIT_MS          100U


/* -------------------------------------------------------------------------- */
/* Internal                                                                   */
/* -------------------------------------------------------------------------- */

typedef enum
{
    AHT10_STATE_IDLE = 0,
    AHT10_STATE_WAITING

} AHT10_State_t;


static I2C_HandleTypeDef *aht10_i2c = NULL;

static AHT10_State_t aht10_state = AHT10_STATE_IDLE;

static uint32_t aht10_next_measurement = 0;
static uint32_t aht10_trigger_time = 0;

static uint8_t aht10_rx_data[6];

static AHT10_Data_t aht10_data =
{
    .temperature = 0.0f,
    .humidity = 0.0f,
    .valid = false
};


/* -------------------------------------------------------------------------- */
/* Internal helper                                                            */
/* -------------------------------------------------------------------------- */

static bool AHT10_TimeReached(uint32_t now, uint32_t target)
{
    return ((int32_t)(now - target) >= 0);
}


/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

bool AHT10_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t reset_command;
    uint8_t init_command[3];

    if (hi2c == NULL)
    {
        return false;
    }

    aht10_i2c = hi2c;

    /* ---------------------------------------------------------------------- */
    /* Reset                                                                   */
    /* ---------------------------------------------------------------------- */

    reset_command = AHT10_CMD_RESET;

    if (HAL_I2C_Master_Transmit(
            aht10_i2c,
            AHT10_ADDRESS,
            &reset_command,
            1,
            100) != HAL_OK)
    {
        return false;
    }

    HAL_Delay(20);


    /* ---------------------------------------------------------------------- */
    /* Initialize                                                              */
    /* ---------------------------------------------------------------------- */

    init_command[0] = AHT10_CMD_INITIALIZE;
    init_command[1] = AHT10_INIT_DATA_1;
    init_command[2] = AHT10_INIT_DATA_2;

    if (HAL_I2C_Master_Transmit(
            aht10_i2c,
            AHT10_ADDRESS,
            init_command,
            3,
            100) != HAL_OK)
    {
        return false;
    }

    HAL_Delay(AHT10_STARTUP_DELAY_MS);


    /* ---------------------------------------------------------------------- */
    /* Initial state                                                           */
    /* ---------------------------------------------------------------------- */

    aht10_state = AHT10_STATE_IDLE;

    aht10_next_measurement =
        HAL_GetTick() + 100U;

    aht10_data.temperature = 0.0f;
    aht10_data.humidity = 0.0f;
    aht10_data.valid = false;

    return true;
}


/* -------------------------------------------------------------------------- */
/* Task                                                                       */
/* -------------------------------------------------------------------------- */

void AHT10_Task(void)
{
    uint32_t now;

    uint32_t temperature_raw;
    uint32_t humidity_raw;

    uint8_t trigger_command[3];


    if (aht10_i2c == NULL)
    {
        return;
    }


    now = HAL_GetTick();


    /* ---------------------------------------------------------------------- */
    /* Waiting for sensor measurement                                         */
    /* ---------------------------------------------------------------------- */

    if (aht10_state == AHT10_STATE_WAITING)
    {
        if ((now - aht10_trigger_time) < AHT10_MEASURE_WAIT_MS)
        {
            return;
        }


        /* ------------------------------------------------------------------ */
        /* Read 6 bytes                                                       */
        /* ------------------------------------------------------------------ */

        if (HAL_I2C_Master_Receive(
                aht10_i2c,
                AHT10_ADDRESS,
                aht10_rx_data,
                6,
                100) == HAL_OK)
        {
            /*
             * Bit 7 of byte 0:
             *
             * 1 = busy
             * 0 = measurement complete
             */

            if ((aht10_rx_data[0] & 0x80U) == 0U)
            {
                /* ---------------------------------------------------------- */
                /* Humidity                                                    */
                /* ---------------------------------------------------------- */

                humidity_raw =
                    ((uint32_t)aht10_rx_data[1] << 12) |
                    ((uint32_t)aht10_rx_data[2] << 4)  |
                    ((uint32_t)aht10_rx_data[3] >> 4);


                /* ---------------------------------------------------------- */
                /* Temperature                                                 */
                /* ---------------------------------------------------------- */

                temperature_raw =
                    ((uint32_t)(aht10_rx_data[3] & 0x0FU) << 16) |
                    ((uint32_t)aht10_rx_data[4] << 8) |
                    ((uint32_t)aht10_rx_data[5]);


                /* ---------------------------------------------------------- */
                /* Convert                                                      */
                /* ---------------------------------------------------------- */

                aht10_data.humidity =
                    ((float)humidity_raw / 1048576.0f) * 100.0f;


                aht10_data.temperature =
                    ((float)temperature_raw / 1048576.0f) *
                    200.0f -
                    50.0f;


                aht10_data.valid = true;
            }
        }


        aht10_state = AHT10_STATE_IDLE;

        aht10_next_measurement =
            now + AHT10_MEASURE_INTERVAL_MS;

        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Start new measurement                                                  */
    /* ---------------------------------------------------------------------- */

    if (AHT10_TimeReached(now, aht10_next_measurement))
    {
        trigger_command[0] = AHT10_CMD_TRIGGER;
        trigger_command[1] = AHT10_TRIGGER_DATA_1;
        trigger_command[2] = AHT10_TRIGGER_DATA_2;


        if (HAL_I2C_Master_Transmit(
                aht10_i2c,
                AHT10_ADDRESS,
                trigger_command,
                3,
                100) == HAL_OK)
        {
            aht10_trigger_time = now;

            aht10_state = AHT10_STATE_WAITING;
        }
        else
        {
            /*
             * Communication error.
             * Try again after a short delay.
             */

            aht10_next_measurement =
                now + 500U;
        }
    }
}


/* -------------------------------------------------------------------------- */
/* Get Data                                                                   */
/* -------------------------------------------------------------------------- */

bool AHT10_GetData(AHT10_Data_t *data)
{
    if (data == NULL)
    {
        return false;
    }

    *data = aht10_data;

    return aht10_data.valid;
}
