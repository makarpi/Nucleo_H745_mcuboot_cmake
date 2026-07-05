#pragma once

#include "stm32h7xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USTAW TUTAJ PINY DLA TWOJEJ PŁYTKI.
 *
 * Przykład:
 *   #define BOOT_LED1_GPIO_Port GPIOB
 *   #define BOOT_LED1_Pin       GPIO_PIN_0
 *
 *   #define BOOT_LED2_GPIO_Port GPIOE
 *   #define BOOT_LED2_Pin       GPIO_PIN_1
 */

#define BOOT_LED1_GPIO_Port     GPIOB
#define BOOT_LED1_Pin           GPIO_PIN_0

#define BOOT_LED2_GPIO_Port     GPIOB
#define BOOT_LED2_Pin           GPIO_PIN_14

/*
 * Jeśli LED jest aktywna stanem wysokim:
 *   GPIO_PIN_SET
 *
 * Jeśli LED jest aktywna stanem niskim:
 *   GPIO_PIN_RESET
 */
#define BOOT_LED1_ACTIVE_STATE  GPIO_PIN_SET
#define BOOT_LED2_ACTIVE_STATE  GPIO_PIN_SET

/*
 * 1 = pokazuje więcej informacji, ale dodaje krótkie opóźnienia diagnostyczne
 * 0 = minimalne opóźnienia, tylko podstawowe sygnały
 */
#define BOOT_LEDS_DIAGNOSTIC    1

/*
 * flash_area IDs z aktualnej mapy:
 *
 * 1 = primary
 * 2 = secondary
 * 3 = scratch
 */
#define BOOT_LED_FA_PRIMARY     1U
#define BOOT_LED_FA_SECONDARY   2U
#define BOOT_LED_FA_SCRATCH     3U

typedef enum {
    BOOT_LED_EVENT_START = 0,
    BOOT_LED_EVENT_BOOT_GO_START,
    BOOT_LED_EVENT_SECONDARY_PRESENT,
    BOOT_LED_EVENT_BOOT_OK,
    BOOT_LED_EVENT_BEFORE_JUMP,
} boot_led_event_t;

typedef enum {
    BOOT_LED_ERR_NONE = 0,
    BOOT_LED_ERR_BOOT_GO_FAILED = 1,
    BOOT_LED_ERR_BAD_APP_VECTOR = 2,
    BOOT_LED_ERR_FLASH_ERASE = 3,
    BOOT_LED_ERR_FLASH_WRITE = 4,
    BOOT_LED_ERR_FLASH_BACKEND = 5,
    BOOT_LED_ERR_UNEXPECTED = 6,
} boot_led_error_t;

void boot_leds_init(void);

void boot_leds_event(boot_led_event_t event);

void boot_leds_flash_erase_begin(uint8_t fa_id, uint32_t abs_addr);
void boot_leds_flash_erase_end(uint8_t fa_id, uint32_t abs_addr, int ok);

void boot_leds_flash_write_pulse(uint8_t fa_id, uint32_t abs_addr);

void boot_leds_set_error(boot_led_error_t error);
boot_led_error_t boot_leds_get_error(void);

void boot_leds_error_loop(boot_led_error_t error);

#ifdef __cplusplus
}
#endif