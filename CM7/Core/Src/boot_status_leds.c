#include "boot_status_leds.h"

static volatile boot_led_error_t g_boot_led_error = BOOT_LED_ERR_NONE;

static GPIO_PinState inactive_state(GPIO_PinState active)
{
    return (active == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

static void gpio_clock_enable(GPIO_TypeDef *port)
{
#if defined(GPIOA)
    if (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); return; }
#endif
#if defined(GPIOB)
    if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); return; }
#endif
#if defined(GPIOC)
    if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); return; }
#endif
#if defined(GPIOD)
    if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); return; }
#endif
#if defined(GPIOE)
    if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); return; }
#endif
#if defined(GPIOF)
    if (port == GPIOF) { __HAL_RCC_GPIOF_CLK_ENABLE(); return; }
#endif
#if defined(GPIOG)
    if (port == GPIOG) { __HAL_RCC_GPIOG_CLK_ENABLE(); return; }
#endif
#if defined(GPIOH)
    if (port == GPIOH) { __HAL_RCC_GPIOH_CLK_ENABLE(); return; }
#endif
#if defined(GPIOI)
    if (port == GPIOI) { __HAL_RCC_GPIOI_CLK_ENABLE(); return; }
#endif
#if defined(GPIOJ)
    if (port == GPIOJ) { __HAL_RCC_GPIOJ_CLK_ENABLE(); return; }
#endif
#if defined(GPIOK)
    if (port == GPIOK) { __HAL_RCC_GPIOK_CLK_ENABLE(); return; }
#endif

    (void)port;
}

static void led1_write(int on)
{
    HAL_GPIO_WritePin(BOOT_LED1_GPIO_Port,
                      BOOT_LED1_Pin,
                      on ? BOOT_LED1_ACTIVE_STATE : inactive_state(BOOT_LED1_ACTIVE_STATE));
}

static void led2_write(int on)
{
    HAL_GPIO_WritePin(BOOT_LED2_GPIO_Port,
                      BOOT_LED2_Pin,
                      on ? BOOT_LED2_ACTIVE_STATE : inactive_state(BOOT_LED2_ACTIVE_STATE));
}

static void led1_toggle(void)
{
    HAL_GPIO_TogglePin(BOOT_LED1_GPIO_Port, BOOT_LED1_Pin);
}

static void led2_toggle(void)
{
    HAL_GPIO_TogglePin(BOOT_LED2_GPIO_Port, BOOT_LED2_Pin);
}

static void leds_all_off(void)
{
    led1_write(0);
    led2_write(0);
}

static void leds_all_on(void)
{
    led1_write(1);
    led2_write(1);
}

static void blink_led1(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        led1_write(1);
        HAL_Delay(on_ms);
        led1_write(0);
        HAL_Delay(off_ms);
    }
}

static void blink_led2(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        led2_write(1);
        HAL_Delay(on_ms);
        led2_write(0);
        HAL_Delay(off_ms);
    }
}

static void blink_both(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        leds_all_on();
        HAL_Delay(on_ms);
        leds_all_off();
        HAL_Delay(off_ms);
    }
}

void boot_leds_init(void)
{
    gpio_clock_enable(BOOT_LED1_GPIO_Port);
    gpio_clock_enable(BOOT_LED2_GPIO_Port);

    GPIO_InitTypeDef gpio = {0};

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = BOOT_LED1_Pin;
    HAL_GPIO_Init(BOOT_LED1_GPIO_Port, &gpio);

    gpio.Pin = BOOT_LED2_Pin;
    HAL_GPIO_Init(BOOT_LED2_GPIO_Port, &gpio);

    leds_all_off();
}

void boot_leds_event(boot_led_event_t event)
{
    switch (event) {
    case BOOT_LED_EVENT_START:
        blink_both(1, 120, 120);
        break;

    case BOOT_LED_EVENT_BOOT_GO_START:
        blink_led1(1, 100, 100);
        break;

    case BOOT_LED_EVENT_SECONDARY_PRESENT:
        blink_led1(2, 100, 100);
        break;

    case BOOT_LED_EVENT_BOOT_OK:
        blink_both(2, 80, 80);
        break;

    case BOOT_LED_EVENT_BEFORE_JUMP:
        blink_both(3, 60, 60);
        break;

    default:
        break;
    }
}

static uint32_t area_code_from_fa_id(uint8_t fa_id)
{
    switch (fa_id) {
    case BOOT_LED_FA_PRIMARY:
        return 1U;

    case BOOT_LED_FA_SECONDARY:
        return 2U;

    case BOOT_LED_FA_SCRATCH:
        return 3U;

    default:
        return 4U;
    }
}

void boot_leds_flash_erase_begin(uint8_t fa_id, uint32_t abs_addr)
{
    (void)abs_addr;

#if BOOT_LEDS_DIAGNOSTIC
    /*
     * Przed erase:
     * LED1 pokazuje obszar:
     *   1 blink = primary
     *   2 blinks = secondary
     *   3 blinks = scratch
     */
    blink_led1(area_code_from_fa_id(fa_id), 50, 60);
    HAL_Delay(80);
#endif

    /*
     * LED2 świeci przez cały erase.
     * To dobrze pokazuje długie operacje kasowania sektorów.
     */
    led2_write(1);
}

void boot_leds_flash_erase_end(uint8_t fa_id, uint32_t abs_addr, int ok)
{
    (void)fa_id;
    (void)abs_addr;

    led2_write(0);

    if (!ok) {
        boot_leds_set_error(BOOT_LED_ERR_FLASH_ERASE);
    }
}

void boot_leds_flash_write_pulse(uint8_t fa_id, uint32_t abs_addr)
{
    (void)fa_id;
    (void)abs_addr;

    /*
     * Nie opóźniamy flash write.
     * Tylko co jakiś czas toggle LED2, żeby było widać aktywność.
     */
    static uint32_t write_counter = 0;

    write_counter++;

    if ((write_counter & 0x3FU) == 0U) {
        led2_toggle();
    }
}

void boot_leds_set_error(boot_led_error_t error)
{
    if (g_boot_led_error == BOOT_LED_ERR_NONE) {
        g_boot_led_error = error;
    }
}

boot_led_error_t boot_leds_get_error(void)
{
    return g_boot_led_error;
}

void boot_leds_error_loop(boot_led_error_t error)
{
    if (error == BOOT_LED_ERR_NONE) {
        error = BOOT_LED_ERR_UNEXPECTED;
    }

    leds_all_off();

    /*
     * LED1 stale ON = fatal error.
     * LED2 miga kod błędu.
     */
    led1_write(1);

    while (1) {
        blink_led2((uint32_t)error, 140, 180);
        HAL_Delay(1200);
    }
}