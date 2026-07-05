# Test Matrix

This document tracks validation status of the standalone MCUboot port for STM32H745 / NUCLEO-H745.

The bootloader is CM7-only. The application image is one signed combined MCUboot image containing CM7 and CM4 firmware.

---

# Current test summary

| ID | Test | Build | Result | Notes |
|---:|---|---|---|---|
| T01 | Valid signed primary boots | Debug | PASS | Signed CM7-only image booted |
| T02 | Wrong-key image rejected | Debug | PASS | Fake signature did not boot |
| T03 | Permanent update swaps | Debug | PASS | Secondary image swapped and booted |
| T04 | Test update swaps | Debug | PASS | Test image in secondary swapped to primary |
| T05 | Test update without confirm reverts | Debug | PASS | Rollback to previous confirmed image |
| T06 | Test update with confirm stays | Debug | PASS | `image_ok` prevents rollback |
| T07 | Combined CM7+CM4 primary boots | Debug | PASS | One signed combined image |
| T08 | Combined CM7+CM4 update swaps | Debug | PASS | Secondary combined image swapped correctly |
| T09 | CMake Debug build boots | Debug | PASS | New CMake project booted signed image |
| T10 | CMake Release build boots | Release | PASS | Release build validates and boots |
| T11 | Release scratch swap | Release | PASS | Release build swapped combined image |
| T12 | Release reset during swap | Release | PASS | MCUboot resumed swap after reset |
| T13 | Release power cut during swap | Release | PASS | MCUboot resumed swap after power cuts |
| T14 | Release final confirmed state | Release | PASS | Final image booted and stayed confirmed |

---

# Test environment

```text
MCU: STM32H745
Board: NUCLEO-H745
Bootloader core: CM7
Application cores: CM7 + CM4

Signature: ECDSA P-256
Crypto backend: TinyCrypt
Swap mode: scratch swap
Image count: 1
Primary validation: enabled
```

---

# Detailed test cases

## T01 - Valid signed primary boots

Expected log pattern:

```text
Primary image: magic=good
Image index: 0, Swap type: none
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

## T02 - Wrong-key image rejected

Expected log pattern:

```text
Image in the primary slot is not valid!
boot_go failed
```

Result:

```text
PASS
```

---

## T03 - Permanent update swaps

Expected log pattern:

```text
Image index: 0, Swap type: perm
Starting swap using scratch algorithm.
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

## T04 - Test update swaps

Expected log pattern:

```text
Image index: 0, Swap type: test
Starting swap using scratch algorithm.
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

## T05 - Test update without confirm reverts

Expected log pattern:

```text
Primary image: magic=good, copy_done=0x1, image_ok=0x3
Image index: 0, Swap type: revert
Starting swap using scratch algorithm.
```

Result:

```text
PASS
```

---

## T06 - Test update with confirm stays

Expected log pattern after second reset:

```text
Primary image: magic=good, copy_done=0x1, image_ok=0x1
Image index: 0, Swap type: none
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

## T07 - Combined CM7+CM4 primary boots

Expected result:

```text
MCUboot validates one combined image.
MCUboot jumps to CM7 application.
CM7 application starts CM4.
```

Result:

```text
PASS
```

---

## T08 - Combined CM7+CM4 update swaps

Expected log pattern:

```text
Image index: 0, Swap type: test
Starting swap using scratch algorithm.
boot_go OK
jumping to app=0x08020200
Image confirmed
```

Result:

```text
PASS
```

---

## T09 - CMake Debug build boots

Result:

```text
PASS
```

---

## T10 - CMake Release build boots

Observed Release size:

```text
FLASH: about 38 KB / 128 KB
RAM: about 3.8 KB / 512 KB
```

Result:

```text
PASS
```

---

## T11 - Release scratch swap

Expected log pattern:

```text
Starting swap using scratch algorithm.
writing copy_done
boot_validate_slot: slot 0
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

## T12 - Release reset during swap

Purpose:

```text
Verify that the device does not brick when reset is pressed during scratch swap.
```

Expected result:

```text
MCUboot resumes or continues swap after reset.
Final image validates.
Application boots.
Application confirms image.
```

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

Result:

```text
PASS
```

Recommended tag:

```text
baseline_release_reset_swap_ok
```

---

## T13 - Release power cut during swap

Purpose:

```text
Verify that the device does not brick when power is cut during scratch swap.
```

Observed important log pattern:

```text
Starting swap using scratch algorithm.
flash erase: addr=0x081E0000
flash erase: addr=0x080E0000
flash erase: addr=0x081C0000
flash erase: addr=0x08060000
flash erase: addr=0x08120000
flash erase: addr=0x08040000
flash erase: addr=0x08100000
flash erase: addr=0x08020000
writing copy_done
boot_validate_slot: slot 0
boot_go OK
jumping to app=0x08020200
Image confirmed
Hello v6
```

Result:

```text
PASS
```

Recommended tag:

```text
baseline_release_powercut_swap_ok
```

Notes:

```text
UART garbage after power cut is considered terminal / USB-serial artifact.
It is not treated as bootloader failure.
```

---

## T14 - Release final confirmed state

Expected log pattern:

```text
Primary image: magic=good, copy_done=0x1, image_ok=0x1
Image index: 0, Swap type: none
boot_go OK
jumping to app=0x08020200
```

Result:

```text
PASS
```

---

# Tests still to do

| ID | Test | Priority | Notes |
|---:|---|---|---|
| T15 | CM4 alive check before confirm | High | Confirm should happen only after CM4 starts correctly |
| T16 | Corrupted secondary image rejected | High | Modify one byte in secondary image |
| T17 | Corrupted primary image rejected | High | Confirm boot_go failure path |
| T18 | Secondary image with valid signature but older version | Medium | Needed before anti-rollback |
| T19 | Anti-rollback / security counter | Medium | Not implemented yet |
| T20 | WRP protection for bootloader sector | High for production | Protect 0x08000000 - 0x0801FFFF |
| T21 | Long repeated update cycle | Medium | 50+ update cycles |
| T22 | Power cut during revert | High | Similar to power cut during forward swap |
| T23 | Release build with log level 3 | Medium | Normal runtime log level |
| T24 | Full erase and factory programming flow | High | Reproduce production programming |
| T25 | App-side update writer to slot1 | Future | Transport-independent update API |
| T26 | Recovery mode | Future | UART/CAN/Ethernet/SD/USB transport |

---

# Regression procedure

Before tagging a new stable baseline, run at minimum:

```text
1. Build bootloader Debug.
2. Build bootloader Release.
3. Flash Release bootloader.
4. Boot confirmed primary combined image.
5. Flash signed test update to secondary.
6. Verify scratch swap.
7. Verify application confirm.
8. Reset and verify no rollback.
9. Flash wrong-key image to secondary or primary test area.
10. Verify rejection.
```

Recommended baseline criteria:

```text
No build errors.
No unexpected warnings.
Bootloader fits in 128 KB.
Signed primary boots.
Signed secondary swaps.
Wrong signature is rejected.
Test image confirms and stays.
Unconfirmed test image reverts.
Release build passes same tests as Debug.
```

---

# Baseline tags

Suggested tags:

```text
baseline_cmake_debug_ok
baseline_cmake_release_ok
baseline_combined_cm7_cm4_update_ok
baseline_release_reset_swap_ok
baseline_release_powercut_swap_ok
```
