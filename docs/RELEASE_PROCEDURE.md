# Release Procedure

This document describes the recommended release process for the STM32H745 standalone MCUboot project.

The release image is one signed combined MCUboot image containing both CM7 and CM4 firmware.

---

# Release goals

A release is considered valid when:

```text
bootloader builds in Release
bootloader fits in 128 KB
combined CM7+CM4 image signs correctly
primary image boots
secondary update swaps correctly
application confirms image
second reset does not roll back
wrong-key image is rejected
```

---

# Versioning

Use MCUboot version format:

```text
major.minor.revision+build
```

Example:

```text
1.2.3+0
```

Recommended naming:

```text
H745_CM7_CM4_v1.2.3_combined_unsigned.bin
H745_CM7_CM4_v1.2.3_combined_signed.bin
```

---

# Build bootloader Release

Use VS Code CMake Tools:

```text
1. Select preset: Release
2. Configure
3. Build
```

Expected output:

```text
CM7/build/Nucleo_H745_mcuboot_cmake_CM7.elf
CM7/build/Nucleo_H745_mcuboot_cmake_CM7.hex
CM7/build/Nucleo_H745_mcuboot_cmake_CM7.bin
```

Check bootloader size:

```text
Must fit within 128 KB.
```

Current expected Release size:

```text
about 38 KB
```

---

# Build applications

CM7 application linker:

```text
FLASH ORIGIN = 0x08020200
FLASH LENGTH = 0x0003FE00
```

CM4 application linker:

```text
FLASH ORIGIN = 0x08060000
FLASH LENGTH = 0x00090000
```

Build raw binaries:

```text
CM7.bin
CM4.bin
```

---

# Merge CM7 and CM4

Merge raw unsigned binaries into one unsigned payload.

```powershell
python .\tools\merge_h745_dualcore.py `
  --cm7 .\CM7_App\Release\CM7.bin `
  --cm7-addr 0x08020200 `
  --cm4 .\CM4_App\Release\CM4.bin `
  --cm4-addr 0x08060000 `
  --slot-start 0x08020000 `
  --header-size 0x200 `
  --slot-size 0xE0000 `
  -o .\Build\H745_CM7_CM4_combined_unsigned.bin
```

Verify:

```text
CM7 reset vector is in range 0x08020200...
CM4 reset vector is in range 0x08060000...
combined image fits in slot
```

---

# Sign factory image

Use `--confirm` for factory primary image.

```powershell
python .\mcuboot\scripts\imgtool.py sign `
  -k .\keys\root-ec-p256.pem `
  --header-size 0x200 `
  --pad-header `
  --align 32 `
  --slot-size 0xE0000 `
  --version 1.2.3+0 `
  --pad `
  --confirm `
  .\Build\H745_CM7_CM4_combined_unsigned.bin `
  .\Build\H745_CM7_CM4_combined_signed.bin
```

---

# Sign test update image

Do not use `--confirm` for test update.

```powershell
python .\mcuboot\scripts\imgtool.py sign `
  -k .\keys\root-ec-p256.pem `
  --header-size 0x200 `
  --pad-header `
  --align 32 `
  --slot-size 0xE0000 `
  --version 1.2.3+0 `
  --pad `
  .\Build\H745_CM7_CM4_combined_unsigned.bin `
  .\Build\H745_CM7_CM4_combined_signed.bin
```

---

# Factory programming test

Erase all:

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

Expected result:

```text
boot_go OK
jumping to app=0x08020200
Image confirmed
```

---

# Update test

Flash test update image to secondary:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08100000 `
  -v -rst
```

Expected result:

```text
Image index: 0, Swap type: test
Starting swap using scratch algorithm.
boot_go OK
jumping to app=0x08020200
Image confirmed
```

Reset again.

Expected result:

```text
Image index: 0, Swap type: none
```

---

# Rollback test

Build/sign test image without application confirm, or temporarily disable confirm in app.

Expected after second reset:

```text
Image index: 0, Swap type: revert
```

The device should return to the previous confirmed image.

---

# Wrong-key test

Sign an image with a different ECDSA P-256 key.

Expected result:

```text
image validation fails
application does not start
```

---

# Power-cut test

During scratch swap:

```text
cut power
restore power
repeat at different swap stages
```

Expected result:

```text
MCUboot resumes or completes swap.
Device does not brick.
Final image validates and boots.
```

---

# Release artifacts to preserve

For each release, preserve:

```text
bootloader ELF/HEX/BIN
bootloader map file
CM7 ELF/HEX/BIN/MAP
CM4 ELF/HEX/BIN/MAP
combined unsigned binary
combined signed binary
signing command
image version
git commit hash
test log
```

Recommended release folder:

```text
release/
  v1.2.3/
    bootloader/
    app/
    logs/
    signing/
```

Do not store private signing keys in release artifacts.

---

# Git tagging

After successful release validation:

```bash
git tag release_v1.2.3
```

For internal bootloader baseline:

```bash
git tag baseline_release_powercut_swap_ok
```
