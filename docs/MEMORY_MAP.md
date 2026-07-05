# Memory Map

This document describes the flash layout used by the standalone MCUboot port for STM32H745 / NUCLEO-H745.

The bootloader is CM7-only. The application image is a single signed MCUboot image containing both CM7 and CM4 firmware.

---

# Device flash overview

STM32H745 flash size used in this project:

```text
Total flash: 2 MB

Bank 1:
0x08000000 - 0x080FFFFF

Bank 2:
0x08100000 - 0x081FFFFF

Sector size:
128 KB = 0x20000

Total sectors:
16 sectors
```

Sector numbering used in this project:

```text
Sector 0   0x08000000 - 0x0801FFFF
Sector 1   0x08020000 - 0x0803FFFF
Sector 2   0x08040000 - 0x0805FFFF
Sector 3   0x08060000 - 0x0807FFFF
Sector 4   0x08080000 - 0x0809FFFF
Sector 5   0x080A0000 - 0x080BFFFF
Sector 6   0x080C0000 - 0x080DFFFF
Sector 7   0x080E0000 - 0x080FFFFF

Sector 8   0x08100000 - 0x0811FFFF
Sector 9   0x08120000 - 0x0813FFFF
Sector 10  0x08140000 - 0x0815FFFF
Sector 11  0x08160000 - 0x0817FFFF
Sector 12  0x08180000 - 0x0819FFFF
Sector 13  0x081A0000 - 0x081BFFFF
Sector 14  0x081C0000 - 0x081DFFFF
Sector 15  0x081E0000 - 0x081FFFFF
```

---

# Final MCUboot flash layout

```text
0x08000000 - 0x0801FFFF   sector 0       MCUboot bootloader
0x08020000 - 0x080FFFFF   sectors 1-7    primary slot / slot0
0x08100000 - 0x081DFFFF   sectors 8-14   secondary slot / slot1
0x081E0000 - 0x081FFFFF   sector 15      scratch
```

Sizes:

```text
Bootloader:     128 KB = 0x20000
Primary slot:   896 KB = 0xE0000
Secondary slot: 896 KB = 0xE0000
Scratch:        128 KB = 0x20000
```

---

# Address constants

```c
#define STM32_FLASH_BASE_ADDR       0x08000000UL
#define STM32_FLASH_SECTOR_SIZE     0x00020000UL

#define BOOTLOADER_ADDR             0x08000000UL
#define BOOTLOADER_SIZE             0x00020000UL

#define SLOT0_ADDR                  0x08020000UL
#define SLOT0_SIZE                  0x000E0000UL

#define SLOT1_ADDR                  0x08100000UL
#define SLOT1_SIZE                  0x000E0000UL

#define SCRATCH_ADDR                0x081E0000UL
#define SCRATCH_SIZE                0x00020000UL

#define MCUBOOT_HEADER_SIZE         0x00000200UL

#define CM7_APP_ADDR                0x08020200UL
#define CM4_APP_ADDR                0x08060000UL

#define STM32_FLASH_WRITE_ALIGN     32U
```

---

# Bootloader region

The bootloader is placed in sector 0:

```text
0x08000000 - 0x0801FFFF
```

Bootloader linker settings:

```text
FLASH ORIGIN = 0x08000000
FLASH LENGTH = 128K
```

The bootloader must never exceed sector 0.

Current observed sizes:

```text
Debug:   about 62 KB
Release: about 38 KB
```

Both fit safely inside the 128 KB bootloader sector.

---

# Primary slot

Primary slot address range:

```text
0x08020000 - 0x080FFFFF
```

Size:

```text
0xE0000 = 896 KB
```

The primary slot contains the active signed MCUboot image.

Layout:

```text
0x08020000   MCUboot image header
0x08020200   CM7 vector table / CM7 application start
0x08060000   CM4 vector table / CM4 application start
...
0x080FFxxx   MCUboot TLV / trailer area
```

The bootloader validates the primary image and jumps to:

```text
0x08020200
```

The CM7 application is then responsible for starting CM4.

---

# Secondary slot

Secondary slot address range:

```text
0x08100000 - 0x081DFFFF
```

Size:

```text
0xE0000 = 896 KB
```

The secondary slot contains a signed update image.

The image stored in secondary is still linked for primary addresses:

```text
CM7 linked at 0x08020200
CM4 linked at 0x08060000
```

This is correct because MCUboot swaps the secondary image into the primary slot before booting it.

Update image flashing address:

```text
0x08100000
```

Example:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08100000 `
  -v -rst
```

---

# Scratch area

Scratch address range:

```text
0x081E0000 - 0x081FFFFF
```

Size:

```text
0x20000 = 128 KB
```

The scratch area is used by MCUboot scratch-swap mode.

Configuration:

```c
#define MCUBOOT_SWAP_USING_SCRATCH 1
```

Scratch must be exactly one flash sector in this layout.

---

# Combined CM7 + CM4 image layout

The signed MCUboot image contains both cores:

```text
one MCUboot image = CM7 firmware + CM4 firmware
```

MCUboot image number:

```c
#define MCUBOOT_IMAGE_NUMBER 1
```

This project does not use MCUboot multi-image mode.

The combined image layout in the primary slot:

```text
0x08020000   MCUboot image header, size 0x200
0x08020200   CM7 vector table
0x08020200   CM7 application payload
0x08060000   CM4 vector table
0x08060000   CM4 application payload
...
0x080FFxxx   TLV + MCUboot trailer
```

---

# CM7 application region

CM7 application starts at:

```text
0x08020200
```

CM7 occupies the first two application sectors, minus the MCUboot header.

Region:

```text
0x08020200 - 0x0805FFFF
```

Size:

```text
0x3FE00
```

CM7 linker settings:

```text
FLASH ORIGIN = 0x08020200
FLASH LENGTH = 0x0003FE00
```

CM7 vector table address:

```text
0x08020200
```

The CM7 application should set VTOR early:

```c
SCB->VTOR = 0x08020200UL;
```

---

# CM4 application region

CM4 application starts at:

```text
0x08060000
```

This gives CM4 the remaining application area inside the same MCUboot image.

Safe initial CM4 linker settings:

```text
FLASH ORIGIN = 0x08060000
FLASH LENGTH = 0x00090000
```

This gives CM4 576 KB and leaves reserve space near the end of the slot for MCUboot TLV and trailer.

CM4 vector table address:

```text
0x08060000
```

The CM4 image must be linked for:

```text
0x08060000
```

It must not be linked for:

```text
0x08100000
```

`0x08100000` is the secondary slot address and must not be used as a linker origin for the application.

---

# Important offset calculations

Unsigned combined payload base:

```text
payload_base = SLOT0_ADDR + MCUBOOT_HEADER_SIZE
payload_base = 0x08020000 + 0x200
payload_base = 0x08020200
```

CM4 offset inside unsigned combined payload:

```text
CM4_APP_ADDR - payload_base
0x08060000 - 0x08020200 = 0x0003FE00
```

CM4 offset inside signed MCUboot image:

```text
CM4_APP_ADDR - SLOT0_ADDR
0x08060000 - 0x08020000 = 0x00040000
```

So:

```text
combined_unsigned.bin:
CM4 starts at file offset 0x3FE00

combined_signed.bin:
CM4 starts at file offset 0x40000
```

---

# MCUboot image header

The MCUboot image header is located at the beginning of the slot:

```text
0x08020000
```

Header size:

```text
0x200
```

The actual CM7 vector table is located after the header:

```text
0x08020200
```

Signing command must use:

```text
--header-size 0x200
--pad-header
```

for raw `.bin` application payloads.

---

# MCUboot trailer

The MCUboot trailer is located near the end of each slot.

For the primary slot:

```text
slot0 start = 0x08020000
slot0 size  = 0x000E0000
slot0 end   = 0x08100000
```

The image confirmation flag is located at:

```text
image_ok offset = 0x000DFFC0
image_ok addr   = 0x080FFFC0
```

Formula:

```text
0x08020000 + 0x000DFFC0 = 0x080FFFC0
```

The application confirms the image by writing `0x01` to:

```text
0x080FFFC0
```

On STM32H745, flash programming uses 32-byte flash words, so the application writes a 32-byte aligned flash word:

```text
byte 0 = 0x01
remaining bytes = 0xFF
```

---

# Flash map backend offsets

MCUboot `flash_area` offsets are global offsets relative to:

```text
0x08000000
```

Current flash areas:

```c
static const struct flash_area flash_map[] = {
    {
        .fa_id = FLASH_AREA_BOOTLOADER,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00000000UL,
        .fa_size = 0x00020000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_0_PRIMARY,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00020000UL,
        .fa_size = 0x000E0000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_0_SECONDARY,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00100000UL,
        .fa_size = 0x000E0000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_SCRATCH,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x001E0000UL,
        .fa_size = 0x00020000UL,
    },
};
```

Important:

```text
fa_off is global offset from 0x08000000.
scratch fa_off is 0x001E0000, not 0x081E0000.
```

Absolute address calculation:

```c
absolute_address = STM32_FLASH_BASE_ADDR + fa->fa_off + offset;
```

---

# Sector offset convention

MCUboot expects sector offsets returned by:

```c
flash_area_get_sectors()
flash_area_to_sectors()
flash_area_get_sector()
```

to be relative to the flash area, not global flash offsets.

Correct:

```c
sectors[i].fs_off = i * STM32_FLASH_SECTOR_SIZE;
```

Wrong:

```c
sectors[i].fs_off = fa->fa_off + i * STM32_FLASH_SECTOR_SIZE;
```

Returning global offsets here causes range errors during scratch erase and swap.

---

# Bank and sector mapping

STM32H745 flash banks:

```text
Bank 1 base = 0x08000000
Bank 2 base = 0x08100000
Bank size   = 0x00100000
Sector size = 0x00020000
```

Examples:

```text
0x08020000 → Bank 1, sector 1
0x08040000 → Bank 1, sector 2
0x08060000 → Bank 1, sector 3
0x080E0000 → Bank 1, sector 7

0x08100000 → Bank 2, sector 0
0x08120000 → Bank 2, sector 1
0x08140000 → Bank 2, sector 2
0x081C0000 → Bank 2, sector 6
0x081E0000 → Bank 2, sector 7
```

---

# Valid boot flow

Normal boot:

```text
Reset
→ CM7 bootloader starts at 0x08000000
→ MCUboot validates primary image at 0x08020000
→ MCUboot jumps to 0x08020200
→ CM7 application starts
→ CM7 application starts CM4 from 0x08060000
→ application self-test
→ application confirms image
```

Update boot:

```text
Signed combined image is written to secondary slot at 0x08100000
→ reset
→ MCUboot validates secondary image
→ MCUboot swaps secondary into primary using scratch
→ MCUboot validates new primary
→ MCUboot jumps to 0x08020200
→ CM7 application starts
→ CM7 starts CM4
→ application confirms image
```

Rollback boot:

```text
Test image is swapped into primary
→ application does not confirm image
→ reset
→ MCUboot reverts to previous confirmed image
```

---

# Safety notes

The bootloader must only jump to the CM7 application after MCUboot validation succeeds.

The CM4 core must not run unvalidated firmware before MCUboot finishes.

CM4 startup belongs to the CM7 application, not to the bootloader.

The bootloader project should remain CM7-only.

---

# Summary

Final memory map:

```text
0x08000000 - 0x0801FFFF   MCUboot bootloader

0x08020000 - 0x080FFFFF   primary slot
  0x08020000              MCUboot image header
  0x08020200              CM7 application
  0x08060000              CM4 application
  0x080FFFC0              image_ok trailer flag

0x08100000 - 0x081DFFFF   secondary slot

0x081E0000 - 0x081FFFFF   scratch
```
