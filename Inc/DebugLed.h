#ifndef DEBUG_LED_H
#define DEBUG_LED_H

/*
 * Diagnostic LEDs of the STM32F4-Discovery (no debugger needed).
 *
 *   PD12 green  : SysTick interrupt alive  -> blinks every 0.5 s
 *   PD13 orange : main loop alive          -> blinks every 0.25 s
 *   PD14 red    : CPU fault (Hard/Bus/Usage/MemManage) -> solid ON
 *   PD15 blue   : LVGL assert (memory pool exhausted ...) -> solid ON
 *   red + blue  : Error_Handler() -> both solid ON
 *
 * Only direct register access is used, so it works even when
 * HAL / LVGL are broken. Remove the calls when the problem is solved.
 */

#include "main.h"

#define DBG_LED_GREEN   (1U << 12)
#define DBG_LED_ORANGE  (1U << 13)
#define DBG_LED_RED     (1U << 14)
#define DBG_LED_BLUE    (1U << 15)

static inline void DebugLed_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    /* short read-back delay after enabling the clock */
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(0xFFU << 24);
    GPIOD->MODER |=  (0x55U << 24);   /* PD12..PD15 = output */

    GPIOD->BSRR = (uint32_t)(0xF000U) << 16;   /* all OFF */
}

static inline void DebugLed_Toggle(uint32_t mask)
{
    GPIOD->ODR ^= mask;
}

/* Call from SysTick_Handler (every 1 ms). */
static inline void DebugLed_SysTick(void)
{
    static uint32_t n = 0U;

    if (++n >= 500U)
    {
        n = 0U;
        DebugLed_Toggle(DBG_LED_GREEN);
    }
}

/* Call from the main loop (as often as possible). */
static inline void DebugLed_MainTick(void)
{
    static uint32_t last = 0U;
    uint32_t now = HAL_GetTick();

    if ((now - last) >= 250U)
    {
        last = now;
        DebugLed_Toggle(DBG_LED_ORANGE);
    }
}

/* Never returns. Turns the given LEDs on and stops. */
static inline void DebugLed_Fatal(uint32_t mask)
{
    __disable_irq();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;
    GPIOD->MODER &= ~(0xFFU << 24);
    GPIOD->MODER |=  (0x55U << 24);

    GPIOD->BSRR = (uint32_t)(0xF000U) << 16;   /* all OFF */
    GPIOD->BSRR = mask;                        /* chosen ON */

    while (1)
    {
    }
}

#endif /* DEBUG_LED_H */
