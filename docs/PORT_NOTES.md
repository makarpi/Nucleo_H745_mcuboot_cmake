# Port Notes

This document describes the STM32H745 standalone MCUboot port.

The port is bare-metal / STM32 HAL based and does not use Zephyr.

---

# Architecture

```text
Reset
→ CM7 bootloader starts
→ MCUboot validates primary image
→ MCUboot swaps secondary image if update is pending
→ MCUboot jumps to CM7 application
→ CM7 application starts CM4
```

The bootloader project is CM7-only.

CM4 firmware is not part of the bootloader project. CM4 firmware is part of the signed combined application image.

---

# MCUboot mode

Current configuration:

```c
#define MCUBOOT_SIGN_EC256
#define MCUBOOT_USE_TINYCRYPT

#define MCUBOOT_VALIDATE_PRIMARY_SLOT

#define MCUBOOT_IMAGE_NUMBER 1
#define MCUBOOT_SWAP_USING_SCRATCH 1

#define MCUBOOT_BOOT_MAX_ALIGN 32
#define MCUBOOT_MAX_IMG_SECTORS 8

#define MCUBOOT_WATCHDOG_FEED() do { } while (0)

#define MCUBOOT_HAVE_LOGGING
#define MCUBOOT_LOG_LEVEL 3
```

Do not define these for current scratch-swap setup:

```c
#define MCUBOOT_PRIMARY_ONLY
#define MCUBOOT_OVERWRITE_ONLY
#define MCUBOOT_DIRECT_XIP
#define MCUBOOT_RAM_LOAD
```

---

# Source files used from MCUboot

Required files from:

```text
mcuboot/boot/bootutil/src
```

```text
bootutil_area.c
bootutil_find_key.c
bootutil_img_hash.c
bootutil_img_security_cnt.c
bootutil_loader.c
bootutil_misc.c
bootutil_public.c
fault_injection_hardening.c
image_ecdsa.c
image_validate.c
loader.c
swap_misc.c
swap_scratch.c
tlv.c
```

Exclude:

```text
boot_record.c
caps.c
encrypted.c
encrypted_psa.c
ed25519_psa.c
image_ed25519.c
image_rsa.c
ram_load.c
swap_move.c
swap_offset.c
fault_injection_hardening_delay_rng_mbedtls.c
fault_injection_hardening_delay_rng_psa.c
```

TinyCrypt sources:

```text
ecc.c
ecc_dsa.c
ecc_platform_specific.c
sha256.c
utils.c
```

MbedTLS ASN.1 sources:

```text
asn1parse.c
platform_util.c
```

---

# Keys

The generated public key source contains:

```c
ecdsa_pub_key
ecdsa_pub_key_len
```

Wrapper:

```c
#include "bootutil/sign_key.h"

extern const unsigned char ecdsa_pub_key[];
extern const unsigned int ecdsa_pub_key_len;

const struct bootutil_key bootutil_keys[] = {
    {
        .key = ecdsa_pub_key,
        .len = &ecdsa_pub_key_len,
    },
};

const int bootutil_key_cnt = 1;
```

---

# Logging

MCUboot logging is routed to UART through:

```text
mcuboot_platform_log()
```

Recommended implementation builds one line in a buffer and sends it with `HAL_UART_Transmit()`.

```c
void mcuboot_platform_log(const char *level, const char *fmt, ...);
```

Retarget `_write()` may also route `printf()` to USART3.

---

# Flash layout

```text
0x08000000 - 0x0801FFFF   bootloader
0x08020000 - 0x080FFFFF   primary
0x08100000 - 0x081DFFFF   secondary
0x081E0000 - 0x081FFFFF   scratch
```

Flash area offsets are relative to base flash address `0x08000000`.

```c
#define STM32_FLASH_BASE_ADDR 0x08000000UL
```

`flash_area.fa_off` values:

```text
bootloader: 0x00000000
primary:    0x00020000
secondary:  0x00100000
scratch:    0x001E0000
```

Important:

```text
scratch fa_off is 0x001E0000, not 0x081E0000.
```

---

# Flash backend sector offset rule

This was a critical bug during porting.

MCUboot expects sector offsets returned by sector APIs to be relative to the flash area.

Correct:

```c
sectors[i].fs_off = i * STM32_FLASH_SECTOR_SIZE;
```

Wrong:

```c
sectors[i].fs_off = fa->fa_off + i * STM32_FLASH_SECTOR_SIZE;
```

Returning global offsets caused scratch erase range errors.

---

# Flash programming

STM32H745 flash programming granularity is 32 bytes.

```c
#define STM32_FLASH_WRITE_ALIGN 32U
```

`flash_area_write()` must support MCUboot trailer writes of small sizes, including:

```text
size = 1
size = 4
```

Therefore, the backend must not reject all unaligned small writes.

Implementation should internally align writes to a 32-byte flash word, fill temporary buffer with `0xFF`, copy the payload at the correct offset, and program one 32-byte flash word.

---

# Jump to application

MCUboot jumps to the CM7 application at:

```text
0x08020200
```

Minimal working boot jump:

```c
void boot_jump_to_image(uint32_t app_address)
{
    typedef void (*jump_function_t)(void);

    uint32_t app_stack = *(__IO uint32_t *)(app_address + 0U);
    uint32_t app_reset = *(__IO uint32_t *)(app_address + 4U);

    deinit_peripherals();

    SCB->VTOR = app_address;
    __DSB();
    __ISB();

    __set_MSP(app_stack);

    __DSB();
    __ISB();

    __enable_irq();

    jump_function_t run_application = (jump_function_t)app_reset;
    run_application();

    while (1) {
    }
}
```

Do not start CM4 in the bootloader.

CM4 startup belongs to the CM7 application.

---

# Confirm image

The app confirms a test image by writing `0x01` to:

```text
0x080FFFC0
```

This is the `image_ok` field in the primary slot trailer.

The application should only confirm after its self-test passes.

For combined CM7+CM4 images, self-test should include a CM4-alive check.

---

# CMake notes

Current project uses VS Code CMake Tools and presets:

```text
Debug
Release
```

CMake currently builds a CM7 sub-build under:

```text
CM7/build/
```

Future cleanup may flatten this into a single root build directory.

---

# Known good observations

Release bootloader size:

```text
about 38 KB / 128 KB
```

Release image validation is significantly faster than Debug.

Power cuts during scratch swap have been tested and MCUboot resumed the swap correctly.

---

# Known pitfalls

```text
Do not link CM4 at 0x08100000.
Do not include CM4 project in bootloader.
Do not commit private signing key.
Do not return global sector offsets from flash_area_get_sectors().
Do not use two signed images for CM7 and CM4.
Do not use MCUboot multi-image mode for this design.
```
