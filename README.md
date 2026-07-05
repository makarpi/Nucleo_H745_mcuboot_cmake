# STM32H745 Standalone MCUboot Port

This repository contains a standalone MCUboot port for STM32H745 / NUCLEO-H745.

The bootloader project is CM7-only. The application firmware is built as one signed combined MCUboot image containing both CM7 and CM4 firmware.

```text
MCUboot image = CM7 application + CM4 application
```

MCUboot validates, swaps, rolls back, and confirms one image. It does not use MCUboot multi-image mode.

---

## Current status

Verified features:

```text
valid signed primary boot
wrong-key image rejection
scratch swap update
test update
rollback without confirm
confirm prevents rollback
combined CM7+CM4 image boot
combined CM7+CM4 image update
Debug CMake build
Release CMake build
Release power-cut recovery during scratch swap
```

---

## Main flash layout

```text
0x08000000 - 0x0801FFFF   MCUboot bootloader, 128 KB
0x08020000 - 0x080FFFFF   primary slot, 896 KB
0x08100000 - 0x081DFFFF   secondary slot, 896 KB
0x081E0000 - 0x081FFFFF   scratch, 128 KB
```

Application layout inside primary slot:

```text
0x08020000   MCUboot image header
0x08020200   CM7 vector table / CM7 application
0x08060000   CM4 vector table / CM4 application
```

---

## Documents

```text
docs/MEMORY_MAP.md
docs/BUILD_AND_FLASH.md
docs/TEST_MATRIX.md
docs/PORT_NOTES.md
docs/RELEASE_PROCEDURE.md
keys/README_KEYS.md
docs/tests/release_powercut_swap_ok.md
```

---

## Important safety rule

The private signing key must not be committed.

```text
root-ec-p256.pem   private key, do not commit
root-ec-p256-pub.c public key source, compiled into bootloader
```

---

## Recommended current baseline tag

```text
baseline_release_powercut_swap_ok
```
