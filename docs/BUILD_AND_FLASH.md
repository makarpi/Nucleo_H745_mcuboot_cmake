# Build and Flash

This document describes the current build, signing, and flashing workflow for the standalone MCUboot project on STM32H745 / NUCLEO-H745.

At this stage, the project is built manually with VS Code and the CMake Tools extension. Helper scripts may be added later.

---

# Current project status

The bootloader project is CM7-only.

MCUboot validates and swaps one signed combined image:

```text
combined image = CM7 application + CM4 application
```

MCUboot does not boot CM4 directly. The boot flow is:

```text
Reset
→ CM7 MCUboot starts
→ MCUboot validates primary image
→ MCUboot performs swap if update exists in secondary
→ MCUboot jumps to CM7 application
→ CM7 application starts CM4
→ application performs self-test
→ application confirms image
```

---

# Flash layout

STM32H745 flash:

```text
Bank 1: 0x08000000 - 0x080FFFFF
Bank 2: 0x08100000 - 0x081FFFFF

Sector size: 128 KB = 0x20000
```

Project layout:

```text
0x08000000 - 0x0801FFFF   sector 0       MCUboot bootloader, 128 KB

0x08020000 - 0x080FFFFF   sectors 1-7    primary slot, 896 KB
0x08100000 - 0x081DFFFF   sectors 8-14   secondary slot, 896 KB
0x081E0000 - 0x081FFFFF   sector 15      scratch, 128 KB
```

Slot layout:

```text
0x08020000   MCUboot image header
0x08020200   CM7 vector table / CM7 application start
0x08060000   CM4 vector table / CM4 application start
```

Important constants:

```text
BOOTLOADER_ADDR     = 0x08000000
BOOTLOADER_SIZE     = 0x00020000

SLOT0_ADDR          = 0x08020000
SLOT0_SIZE          = 0x000E0000

SLOT1_ADDR          = 0x08100000
SLOT1_SIZE          = 0x000E0000

SCRATCH_ADDR        = 0x081E0000
SCRATCH_SIZE        = 0x00020000

MCUBOOT_HEADER_SIZE = 0x200
CM7_APP_ADDR        = 0x08020200
CM4_APP_ADDR        = 0x08060000
```

---

# Linker settings

## Bootloader CM7

The MCUboot bootloader is linked at the beginning of flash:

```text
FLASH ORIGIN = 0x08000000
FLASH LENGTH = 128K
```

Bootloader must fit in:

```text
0x08000000 - 0x0801FFFF
```

## Application CM7

The CM7 application is linked after the MCUboot image header:

```text
FLASH ORIGIN = 0x08020200
FLASH LENGTH = 0x0003FE00
```

This gives CM7 two flash sectors minus the MCUboot header.

## Application CM4

The CM4 application is linked inside the same MCUboot primary slot:

```text
FLASH ORIGIN = 0x08060000
FLASH LENGTH = 0x00090000
```

The CM4 image must not overwrite the MCUboot TLV or trailer area near the end of the primary slot.

---

# Build bootloader

## Current workflow

The bootloader is currently built with:

```text
VS Code
CMake Tools extension
STM32CubeCLT
Ninja
arm-none-eabi-gcc
```

Available CMake presets:

```text
Debug
Release
```

Recommended workflow:

1. Open the project folder in VS Code.
2. Select CMake preset:
   - `Debug` for development and verbose logs.
   - `Release` for optimized bootloader testing.
3. Run `CMake: Configure`.
4. Run `CMake: Build`.

The current CMake project uses a CM7 sub-build.

Expected output location:

```text
CM7/build/
```

Expected bootloader artifacts:

```text
Nucleo_H745_mcuboot_cmake_CM7.elf
Nucleo_H745_mcuboot_cmake_CM7.hex
Nucleo_H745_mcuboot_cmake_CM7.bin
```

Use `.hex` for normal flashing.

---

# Flash bootloader

Preferred artifact:

```text
CM7/build/Nucleo_H745_mcuboot_cmake_CM7.hex
```

Command:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\CM7\build\Nucleo_H745_mcuboot_cmake_CM7.hex `
  -v -rst
```

If flashing the `.bin`, an explicit address is required:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\CM7\build\Nucleo_H745_mcuboot_cmake_CM7.bin 0x08000000 `
  -v -rst
```

---

# Generate signing key

Generate private key:

```powershell
python .\mcuboot\scripts\imgtool.py keygen `
  -k .\keys\root-ec-p256.pem `
  -t ecdsa-p256
```

Generate public key C file:

```powershell
python .\mcuboot\scripts\imgtool.py getpub `
  -k .\keys\root-ec-p256.pem |
  Set-Content -Encoding ascii .\CM7\Core\Src\root-ec-p256-pub.c
```

Important:

```text
Do not commit root-ec-p256.pem.
Only the generated public key C file is compiled into the bootloader.
```

The private key should stay outside the firmware and outside the repository.

---

# Build combined CM7 + CM4 image

The bootloader expects one signed MCUboot image.

The combined image contains:

```text
CM7 application at 0x08020200
CM4 application at 0x08060000
```

The input images must be raw unsigned application binaries:

```text
CM7.bin
CM4.bin
```

Do not combine two already signed images.

Expected merge flow:

```text
CM7.bin + padding 0xFF + CM4.bin
→ combined_unsigned.bin
→ imgtool sign
→ combined_signed.bin
```

Example merge command:

```powershell
python .\tools\merge_h745_dualcore.py `
  --cm7 .\CM7_App\Debug\CM7.bin `
  --cm7-addr 0x08020200 `
  --cm4 .\CM4_App\Debug\CM4.bin `
  --cm4-addr 0x08060000 `
  --slot-start 0x08020000 `
  --header-size 0x200 `
  --slot-size 0xE0000 `
  -o .\Build\H745_CM7_CM4_combined_unsigned.bin
```

For the unsigned combined payload:

```text
CM4 offset = 0x08060000 - 0x08020200 = 0x0003FE00
```

For the signed image with MCUboot header:

```text
CM4 offset = 0x08060000 - 0x08020000 = 0x00040000
```

---

# Sign factory image

Use this when programming a known-good image directly into the primary slot.

The `--confirm` flag marks the image as already confirmed.

```powershell
python .\mcuboot\scripts\imgtool.py sign `
  -k .\keys\root-ec-p256.pem `
  --header-size 0x200 `
  --pad-header `
  --align 32 `
  --slot-size 0xE0000 `
  --version 1.0.0+0 `
  --pad `
  --confirm `
  .\Build\H745_CM7_CM4_combined_unsigned.bin `
  .\Build\H745_CM7_CM4_combined_signed.bin
```

The signed image must be flashed at the start of the primary slot:

```text
0x08020000
```

---

# Sign test update image

Use this when programming an update into the secondary slot.

Do not use `--confirm` for a test update.

```powershell
python .\mcuboot\scripts\imgtool.py sign `
  -k .\keys\root-ec-p256.pem `
  --header-size 0x200 `
  --pad-header `
  --align 32 `
  --slot-size 0xE0000 `
  --version 1.1.0+0 `
  --pad `
  .\Build\H745_CM7_CM4_combined_unsigned.bin `
  .\Build\H745_CM7_CM4_combined_signed.bin
```

After swap, the application must call image confirm after its self-test.

If the application does not confirm the image, MCUboot reverts to the previous version on the next reset.

---

# Flash primary image

Use this for factory programming or manual primary testing.

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08020000 `
  -v -rst
```

Expected MCUboot log:

```text
H745 MCUboot CM7 start
Primary image: magic=good
Image index: 0, Swap type: none
boot_go OK
image addr=0x08020000
jumping to app=0x08020200
```

Expected application log:

```text
Image confirmed
Hello vX
```

---

# Flash secondary update

Use this to test MCUboot update/swap.

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08100000 `
  -v -rst
```

Expected MCUboot log:

```text
Image index: 0, Swap type: test
Starting swap using scratch algorithm.
...
boot_go OK
jumping to app=0x08020200
```

Expected application log:

```text
Image confirmed
Hello vX
```

After reset, if confirm succeeded, expected MCUboot log:

```text
Image index: 0, Swap type: none
boot_go OK
jumping to app=0x08020200
```

---

# Flash complete device from scratch

Typical manual sequence:

```powershell
STM32_Programmer_CLI -c port=SWD -e all
```

Flash bootloader:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\CM7\build\Nucleo_H745_mcuboot_cmake_CM7.hex `
  -v
```

Flash confirmed primary image:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08020000 `
  -v -rst
```

---

# Image confirmation

The application confirms the image after successful startup and self-test.

Current confirm location:

```text
MCUBOOT_IMAGE_OK_ADDR = 0x080FFFC0
```

This address comes from:

```text
slot0 start      = 0x08020000
image_ok offset  = 0x000DFFC0
image_ok address = 0x080FFFC0
```

The application writes:

```text
0x01
```

to the `image_ok` field in the primary slot trailer.

On STM32H745, flash programming uses a 32-byte flash word, so the confirm function writes a 32-byte aligned flash word:

```text
byte 0 = 0x01
remaining bytes = 0xFF
```

A test image that is not confirmed will revert on next reset.

---

# Expected tested behavior

The current project has verified:

```text
valid signed primary boots
wrong-key image is rejected
permanent update works
test update works
rollback works when image is not confirmed
confirm prevents rollback
combined CM7+CM4 image boots
combined CM7+CM4 update swaps correctly
Release build works
power cut during scratch swap recovers correctly
```

---

# Notes about Release build

Release build is significantly faster than Debug during validation.

Typical Release validation timing for a combined image:

```text
bootutil_img_hash starts around 0.06 s
ECDSA verify starts around 0.5 s
validation completes around 0.8 s
```

The bootloader currently fits comfortably within 128 KB.

Typical Release size observed:

```text
FLASH usage: about 38 KB / 128 KB
```

---

# Notes about logs

For active debugging:

```c
#define MCUBOOT_LOG_LEVEL 4
```

For normal development:

```c
#define MCUBOOT_LOG_LEVEL 3
```

Recommended final log levels:

```text
0 = off
1 = err
2 = wrn
3 = inf
4 = dbg
```

Verbose flash erase/write logs should eventually be controlled separately, for example:

```c
#define FLASH_BACKEND_VERBOSE 0
```

---

# Notes about tools

At this stage, helper scripts are optional.

Planned future helper scripts:

```text
tools/merge_h745_dualcore.py
tools/sign_combined.ps1
tools/flash_bootloader.ps1
tools/flash_primary.ps1
tools/flash_secondary.ps1
tools/erase_all.ps1
```

Current workflow is manual through:

```text
VS Code / CMake Tools
STM32_Programmer_CLI
imgtool.py
```
