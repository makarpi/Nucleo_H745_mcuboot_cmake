# Test: Release power-cut during scratch swap

## Target

```text
Board: NUCLEO-H745
MCU: STM32H745
Bootloader: standalone MCUboot
Build: Release
Swap mode: scratch swap
Image: combined CM7+CM4
```

---

# Purpose

Verify that the device does not brick when power is cut during MCUboot scratch swap.

---

# Flash layout

```text
0x08000000 - 0x0801FFFF   MCUboot bootloader
0x08020000 - 0x080FFFFF   primary slot
0x08100000 - 0x081DFFFF   secondary slot
0x081E0000 - 0x081FFFFF   scratch
```

---

# Procedure

```text
1. Flash Release bootloader.
2. Flash confirmed combined image to primary slot.
3. Flash signed test update combined image to secondary slot.
4. Reset board to start swap.
5. Cut power during swap.
6. Restore power.
7. Repeat power cut at different swap stages.
8. Observe whether MCUboot resumes and completes swap.
```

---

# Expected result

```text
MCUboot must not brick.
MCUboot must resume or complete swap.
Final primary image must validate.
Application must boot.
Application must confirm image.
```

---

# Observed result

```text
PASS
```

MCUboot resumed swap after multiple power cuts and reached:

```text
boot_go OK
image addr=0x08020000
jumping to app=0x08020200
Image confirmed
Hello v6
```

A later reset reported no pending swap:

```text
Image index: 0, Swap type: none
```

---

# Notes

UART garbage after power cut was observed. This is considered a terminal / USB-serial artifact and not a bootloader failure.

---

# Important observed stages

Power was cut around these operations:

```text
erase scratch
write swap_info
erase primary trailer sector
erase secondary trailer sector
erase secondary data sector
erase primary data sector
before copy_done
during resumed swap
```

The bootloader still recovered.

---

# Conclusion

The Release bootloader passed power-cut recovery testing during scratch swap.

This is a valid stable baseline candidate:

```text
baseline_release_powercut_swap_ok
```
