# Tools

Helper scripts are planned for this project, but the current workflow still uses VS Code / CMake Tools and manual STM32_Programmer_CLI commands.

Planned tools:

```text
merge_h745_dualcore.py
sign_combined.ps1
flash_bootloader.ps1
flash_primary.ps1
flash_secondary.ps1
erase_all.ps1
inspect_image.ps1
```

---

# Current manual commands

Flash bootloader:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\CM7\build\Nucleo_H745_mcuboot_cmake_CM7.hex `
  -v -rst
```

Flash primary:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08020000 `
  -v -rst
```

Flash secondary:

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w .\Build\H745_CM7_CM4_combined_signed.bin 0x08100000 `
  -v -rst
```
