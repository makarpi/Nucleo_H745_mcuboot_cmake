# Bootloader LED Diagnostics

This document describes the proposed two-LED diagnostic interface for the standalone MCUboot bootloader on STM32H745.

The goal is to provide useful bootloader state information on boards where UART logs are not available.

---

# Purpose

The bootloader may run on a target board without exposed UART or VCOM.

Two LEDs are used to show:

```text
bootloader start
primary image validation
secondary update detection
scratch swap activity
flash erase/write activity
successful jump to application
fatal boot errors
```

This mechanism is intended for bring-up, production diagnostics, and field troubleshooting.

---

# LED roles

The bootloader uses two LEDs:

```text
LED1 = STATUS / phase / error latch
LED2 = ACTIVITY / flash activity / error code
```

Recommended behavior:

```text
LED1:
  shows high-level bootloader states
  stays ON in fatal error mode

LED2:
  shows flash erase/write activity
  blinks numeric error codes in fatal error mode
```

---

# Electrical assumptions

Each LED can be active-high or active-low.

Configuration example:

```c
#define BOOT_LED1_GPIO_Port     GPIOB
#define BOOT_LED1_Pin           GPIO_PIN_0

#define BOOT_LED2_GPIO_Port     GPIOB
#define BOOT_LED2_Pin           GPIO_PIN_14

#define BOOT_LED1_ACTIVE_STATE  GPIO_PIN_SET
#define BOOT_LED2_ACTIVE_STATE  GPIO_PIN_SET
```

For active-low LEDs:

```c
#define BOOT_LED1_ACTIVE_STATE  GPIO_PIN_RESET
#define BOOT_LED2_ACTIVE_STATE  GPIO_PIN_RESET
```

---

# Files

Recommended files:

```text
CM7/Core/Inc/boot_status_leds.h
CM7/Core/Src/boot_status_leds.c
```

Optional future config split:

```text
CM7/Core/Inc/boot_status_leds_config.h
```

---

# User-visible LED protocol

## Normal boot without update

```text
Both LEDs blink once        bootloader entered
LED1 blinks once            boot_go / validation started
Both LEDs blink twice       boot_go OK
Both LEDs blink three times bootloader is about to jump to application
Application starts
```

Expected sequence:

```text
START
→ VALIDATE
→ BOOT_OK
→ BEFORE_JUMP
→ APP
```

---

## Boot with update in secondary slot

```text
Both LEDs blink once        bootloader entered
LED1 blinks twice           secondary image appears to be present
LED1 area code blinks       erase area indication
LED2 ON                     flash erase in progress
LED2 flicker                flash writes / swap status updates
Both LEDs blink twice       boot_go OK
Both LEDs blink three times bootloader is about to jump to application
Application starts
```

Expected sequence:

```text
START
→ SECONDARY_PRESENT
→ SWAP / FLASH_ACTIVITY
→ BOOT_OK
→ BEFORE_JUMP
→ APP
```

---

## Fatal error mode

Fatal error mode uses:

```text
LED1 ON continuously
LED2 blinks numeric error code
pause
repeat
```

Example:

```text
LED1 ON
LED2: blink blink blink
pause
LED2: blink blink blink
pause
```

This means:

```text
error code 3
```

---

# Error codes

```text
1 blink  = boot_go failed / no valid bootable image
2 blinks = invalid application vector
3 blinks = flash erase failed
4 blinks = flash write failed
5 blinks = flash backend / range / read error
6 blinks = unexpected error
```

| LED2 code | Meaning | Typical cause |
|---:|---|---|
| 1 | `BOOT_LED_ERR_BOOT_GO_FAILED` | no valid signed image, invalid signature, corrupt primary |
| 2 | `BOOT_LED_ERR_BAD_APP_VECTOR` | app reset vector outside primary slot, bad SP, invalid image layout |
| 3 | `BOOT_LED_ERR_FLASH_ERASE` | HAL erase failure, wrong bank/sector, locked/protected flash |
| 4 | `BOOT_LED_ERR_FLASH_WRITE` | HAL program failure, unaligned write handling bug, protected flash |
| 5 | `BOOT_LED_ERR_FLASH_BACKEND` | range error, flash map bug, read/write offset bug |
| 6 | `BOOT_LED_ERR_UNEXPECTED` | fallback for unknown fatal error |

---

# Flash area indication

Before a flash erase operation, LED1 can blink the area code.

```text
1 blink  = primary slot
2 blinks = secondary slot
3 blinks = scratch area
4 blinks = unknown area
```

During erase:

```text
LED2 ON = erase in progress
```

During flash write activity:

```text
LED2 toggles periodically
```

This makes scratch swap visible even without UART.

---

# Diagnostic mode

The LED module can support two operating modes:

```c
#define BOOT_LEDS_DIAGNOSTIC 1
```

Diagnostic mode:

```text
shows area-code blinks before erase
adds small delays so humans can observe the phase
best for bring-up and validation
```

Production/minimal mode:

```c
#define BOOT_LEDS_DIAGNOSTIC 0
```

Production mode:

```text
no intentional phase delays
only start / activity / success / fatal error indication
best for release builds
```

---

# Recommended behavior by build type

## Debug / bring-up

```c
#define BOOT_LEDS_DIAGNOSTIC 1
#define MCUBOOT_LOG_LEVEL 0
```

UART logs may be disabled when unavailable.

LEDs become the primary diagnostic interface.

## Release

```c
#define BOOT_LEDS_DIAGNOSTIC 0
#define MCUBOOT_LOG_LEVEL 0
```

or:

```c
#define MCUBOOT_LOG_LEVEL 1
```

if an error-only log backend exists.

---

# Public API

Recommended header:

```c
#pragma once

#include "stm32h7xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_LED1_GPIO_Port     GPIOB
#define BOOT_LED1_Pin           GPIO_PIN_0

#define BOOT_LED2_GPIO_Port     GPIOB
#define BOOT_LED2_Pin           GPIO_PIN_14

#define BOOT_LED1_ACTIVE_STATE  GPIO_PIN_SET
#define BOOT_LED2_ACTIVE_STATE  GPIO_PIN_SET

#define BOOT_LEDS_DIAGNOSTIC    1

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
```

---

# Integration in bootloader main

## Include

```c
#include "boot_status_leds.h"
#include "bootutil/image.h"
```

## Initialization

Call after HAL and clock initialization:

```c
HAL_Init();
SystemClock_Config();

MX_GPIO_Init();

boot_leds_init();
boot_leds_event(BOOT_LED_EVENT_START);
```

If `boot_leds_init()` initializes LED GPIO by itself, `MX_GPIO_Init()` may still remain for other GPIOs.

---

# Secondary image detection

Before `boot_go()`:

```c
const struct image_header *slot1_hdr =
    (const struct image_header *)0x08100000UL;

if (slot1_hdr->ih_magic == IMAGE_MAGIC) {
    boot_leds_event(BOOT_LED_EVENT_SECONDARY_PRESENT);
}

boot_leds_event(BOOT_LED_EVENT_BOOT_GO_START);
```

This is only a simple indication that a valid MCUboot image header appears to be present in secondary.

The actual signature validation is still performed by MCUboot.

---

# boot_go failure handling

```c
struct boot_rsp rsp;
fih_ret rc = boot_go(&rsp);

if (FIH_NOT_EQ(rc, FIH_SUCCESS)) {
    boot_led_error_t led_error = boot_leds_get_error();

    if (led_error == BOOT_LED_ERR_NONE) {
        led_error = BOOT_LED_ERR_BOOT_GO_FAILED;
    }

    boot_leds_error_loop(led_error);
}
```

---

# Vector sanity check before jump

After successful `boot_go()`:

```c
uint32_t image_addr = STM32_FLASH_BASE_ADDR + rsp.br_image_off;
uint32_t app_addr   = image_addr + rsp.br_hdr->ih_hdr_size;

uint32_t app_sp     = *(uint32_t *)(app_addr + 0U);
uint32_t app_reset  = *(uint32_t *)(app_addr + 4U);

if ((app_reset < 0x08020000UL) || (app_reset >= 0x08100000UL)) {
    boot_leds_error_loop(BOOT_LED_ERR_BAD_APP_VECTOR);
}

if ((app_sp < 0x20000000UL) || (app_sp >= 0x30000000UL)) {
    boot_leds_error_loop(BOOT_LED_ERR_BAD_APP_VECTOR);
}

boot_leds_event(BOOT_LED_EVENT_BOOT_OK);
boot_leds_event(BOOT_LED_EVENT_BEFORE_JUMP);

boot_jump_to_image(app_addr);
```

The SP range is intentionally broad because STM32H745 can use different RAM regions.

---

# Integration in flash backend

In `flash_map_backend.c`:

```c
#include "boot_status_leds.h"
```

## Erase begin/end

Before `HAL_FLASHEx_Erase()`:

```c
boot_leds_flash_erase_begin(fa->fa_id, abs_addr);

HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &sector_error);

boot_leds_flash_erase_end(fa->fa_id, abs_addr, st == HAL_OK);
```

On error:

```c
if (st != HAL_OK) {
    boot_leds_set_error(BOOT_LED_ERR_FLASH_ERASE);

    HAL_FLASH_Lock();
    return -1;
}
```

## Flash write pulse

Around `HAL_FLASH_Program()`:

```c
boot_leds_flash_write_pulse(fa->fa_id, abs_addr);

HAL_StatusTypeDef st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                         abs_addr,
                                         (uint32_t)flashword_buf);

if (st != HAL_OK) {
    boot_leds_set_error(BOOT_LED_ERR_FLASH_WRITE);

    HAL_FLASH_Lock();
    return -1;
}
```

## Backend errors

For range/read/write validation errors:

```c
boot_leds_set_error(BOOT_LED_ERR_FLASH_BACKEND);
```

Examples:

```c
if (fa == NULL) {
    boot_leds_set_error(BOOT_LED_ERR_FLASH_BACKEND);
    return -1;
}

if ((off + len) > fa->fa_size) {
    boot_leds_set_error(BOOT_LED_ERR_FLASH_BACKEND);
    return -1;
}
```

---

# Recommended implementation notes

## Do not add long delays in critical flash operations

LED indication should not significantly change flash operation timing.

Allowed:

```text
short blinks before erase in diagnostic mode
LED2 ON during erase
LED2 periodic toggle during write
```

Avoid:

```text
long delay between erase and write
long delay while flash is unlocked
blocking animations inside low-level write loops
```

## Keep fatal error loops simple

Fatal error mode should not depend on UART, interrupts, scheduler, or application code.

It should only use:

```text
GPIO
HAL_Delay()
SysTick
```

## Keep LED logic independent from MCUboot internals

LED module should not parse MCUboot trailers directly.

It should only receive high-level events from:

```text
main.c
flash_map_backend.c
```

---

# Example user interpretation

## Case 1: valid primary, no update

Observed:

```text
Both LEDs blink once
LED1 blinks once
Both LEDs blink twice
Both LEDs blink three times
Application starts
```

Meaning:

```text
bootloader entered
boot_go started
image valid
jump to application
```

---

## Case 2: update is being swapped

Observed:

```text
Both LEDs blink once
LED1 blinks twice
LED1 blinks 3 times
LED2 ON for about 1 second
LED1 blinks 1 time
LED2 ON
LED2 flickers
Both LEDs blink three times
Application starts
```

Meaning:

```text
bootloader entered
secondary update detected
scratch erase
primary erase
swap writes
jump to updated application
```

---

## Case 3: no valid image

Observed:

```text
LED1 ON continuously
LED2 blinks once
pause
LED2 blinks once
pause
```

Meaning:

```text
boot_go failed
no valid bootable image or signature validation failed
```

---

## Case 4: flash backend error

Observed:

```text
LED1 ON continuously
LED2 blinks five times
pause
repeat
```

Meaning:

```text
flash backend/range/read/write API error
check flash map, offsets, sector offset convention
```

---

# Test procedure

## Basic test

```text
1. Build bootloader with LED diagnostics enabled.
2. Flash known-good confirmed primary image.
3. Reset board.
4. Verify normal boot sequence.
```

Expected:

```text
START indication
BOOT_GO_START indication
BOOT_OK indication
BEFORE_JUMP indication
application starts
```

## Update test

```text
1. Flash known-good confirmed primary image.
2. Flash signed test update to secondary slot.
3. Reset board.
4. Observe secondary/update/swap activity.
5. Confirm that application starts and confirms image.
```

Expected:

```text
secondary present indication
flash erase/write activity
boot OK
jump to application
```

## Error test

```text
1. Flash invalid or wrong-key image.
2. Reset board.
3. Observe fatal error code.
```

Expected:

```text
LED1 ON continuously
LED2 error code repeats
```

---

# Production recommendation

For production release:

```c
#define BOOT_LEDS_DIAGNOSTIC 0
#define MCUBOOT_LOG_LEVEL 0
```

Recommended remaining visible signals:

```text
bootloader start
flash activity
fatal error code
before jump
```

For development / board bring-up:

```c
#define BOOT_LEDS_DIAGNOSTIC 1
#define MCUBOOT_LOG_LEVEL 0
```

---

# Limitations

Two LEDs cannot show the complete MCUboot log.

The LED protocol is intended to show phase and error class, not detailed cryptographic or flash metadata.

For deep debugging, use at least one of:

```text
SWD breakpoints
SWO / ITM
SEGGER RTT
temporary UART
RAM log buffer read by application
```

---

# Summary

The two-LED diagnostic interface gives a usable no-UART view of MCUboot behavior:

```text
LED1 = boot phase / fatal error latch
LED2 = flash activity / numeric error code
```

It allows the user to identify:

```text
bootloader entered
update detected
scratch swap in progress
flash area being erased
boot success
jump to application
fatal failure class
```

Recommended document tag after implementation:

```text
baseline_led_diagnostics_ok
```
