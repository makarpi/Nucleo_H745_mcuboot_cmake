/*
 * boot_jump.c
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#include "boot_jump.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>

/* Jeśli używasz UART3 w bootloaderze */
extern UART_HandleTypeDef huart3;

static void deinit_peripherals(void)
{
    HAL_UART_DeInit(&huart3);
    HAL_GPIO_DeInit(LD1_GPIO_Port, LD1_Pin);

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
}

void boot_jump_to_image(uint32_t app_address)
{
    typedef void (*jump_function_t)(void);

    uint32_t app_stack = *(__IO uint32_t *)(app_address + 0U);
    uint32_t app_reset = *(__IO uint32_t *)(app_address + 4U);

    deinit_peripherals();

    /*
     * Bardzo ważne przy aplikacji przesuniętej na 0x08020200.
     */
    SCB->VTOR = app_address;

    __DSB();
    __ISB();

    /*
     * Ustaw MSP aplikacji.
     */
    __set_MSP(app_stack);

//    /*
//     * Dla bezpieczeństwa wyczyść PSP, ale nie kombinujemy więcej.
//     */
//    __set_PSP(0);

    __DSB();
    __ISB();


    __enable_irq();

    jump_function_t run_application = (jump_function_t)app_reset;
    run_application();

    while (1) {
    }
}

