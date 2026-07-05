# Signing Keys

This document describes key handling for the STM32H745 standalone MCUboot project.

---

# Key type

Current signing algorithm:

```text
ECDSA P-256
```

Current crypto backend in bootloader:

```text
TinyCrypt
```

Relevant MCUboot config:

```c
#define MCUBOOT_SIGN_EC256
#define MCUBOOT_USE_TINYCRYPT
```

---

# Generate private key

```powershell
python .\mcuboot\scripts\imgtool.py keygen `
  -k .\keys\root-ec-p256.pem `
  -t ecdsa-p256
```

The private key is used only on the signing machine.

It must not be committed.

---

# Generate public key source

```powershell
python .\mcuboot\scripts\imgtool.py getpub `
  -k .\keys\root-ec-p256.pem |
  Set-Content -Encoding ascii .\CM7\Core\Src\root-ec-p256-pub.c
```

Use ASCII encoding on Windows PowerShell.

Do not use plain `>` if it creates UTF-16 output.

---

# Files

Private key:

```text
keys/root-ec-p256.pem
```

Do not commit.

Public key source:

```text
CM7/Core/Src/root-ec-p256-pub.c
```

Commit this file.

Key wrapper:

```text
CM7/Core/Src/keys.c
```

Commit this file.

---

# Key wrapper

Generated `root-ec-p256-pub.c` contains:

```c
ecdsa_pub_key
ecdsa_pub_key_len
```

MCUboot expects:

```c
bootutil_keys
bootutil_key_cnt
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

# Git ignore rule

The repository must ignore private keys:

```gitignore
*.pem
keys/*.pem
```

Before committing, check:

```powershell
git status --short | Select-String ".pem"
```

If a private key was staged by mistake:

```powershell
git restore --staged .\keys\root-ec-p256.pem
```

---

# Security notes

The private key signs firmware images.

The public key verifies signatures in the bootloader.

If an attacker can replace the bootloader, they can replace the public key or disable signature checks. Therefore, the bootloader sector must eventually be protected using STM32 option bytes such as WRP/RDP.

Do not use development keys for production firmware.

---

# Key rotation

Not implemented yet.

Potential future strategy:

```text
bootloader contains multiple public keys
new images can be signed by old or new key during transition
after transition, old key is removed in a new bootloader release
```

This requires careful production planning.
