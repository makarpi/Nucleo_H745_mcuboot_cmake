# Test: Release reset during scratch swap

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

Verify that the device does not brick when it is reset during MCUboot scratch swap.

This test is different from a full power-cut test. In this test, the board is reset while power remains present.

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
5. Press reset during scratch swap.
6. Let the board boot again.
7. Repeat reset at different swap stages if needed.
8. Observe whether MCUboot resumes and completes swap.
```

---

# Expected result

```text
MCUboot must not brick.
MCUboot must resume or continue swap.
Final primary image must validate.
Application must boot.
Application must confirm image.
```

---

# Observed result

```text
PASS
```

MCUboot resumed the swap after reset and reached:

```text
boot_go OK
image addr=0x08020000
jumping to app=0x08020200
Image confirmed
```

---

# Important observed behavior

After reset during swap, MCUboot used the trailer and swap status to continue the interrupted operation.

Expected log fragments:

```text
Boot source: primary slot
Starting swap using scratch algorithm.
writing swap status
writing copy_done
boot_validate_slot: slot 0
boot_go OK
jumping to app=0x08020200
```

---

# Conclusion

The Release bootloader passed reset recovery testing during scratch swap.

Recommended baseline tag:

```text
baseline_release_reset_swap_ok
```
