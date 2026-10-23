# Securing TinyML Model Deployment on ESP32-C6

Firmware, attack scripts, and build configuration for the Bachelor's thesis
**"Securing TinyML Model Deployment on ESP32-C6: Protecting Against Firmware
Tampering and Model Extraction"** (Ton That Nam, Turku University of Applied
Sciences, 2026).

The thesis deploys a small TinyML gas-classification model on an ESP32-C6,
attacks it on an unprotected build, then enables the chip's hardware security
features (Secure Boot V2 + Flash Encryption) and repeats the attacks to measure
what is blocked and at what cost. This repository contains everything needed to
reproduce that work.

---

## Hardware & toolchain

- **Board:** ESP32-C6 (RISC-V)
- **Framework:** ESP-IDF v6.1
- **Flashing:** UART, `esptool`

## The model

`ethanol_detect` — a feed-forward neural network ported to plain C from the
AISmellX research project's ONNX model.

- Input: a 10-step heater-profile window → 28 engineered features
- Topology: 28 → 32 → 16 → 2 (ReLU, ReLU, Softmax), **1,490 parameters**
- Output: two classes (0 = ethanol, 1 = water)

Implemented in [`main/gas_model.c`](main/gas_model.c) /
[`main/gas_model.h`](main/gas_model.h); weights in
[`main/model_weights.h`](main/model_weights.h).

## Firmware variants

The project builds `main/hello_world_main.c`. Four variants are provided; swap
the one you want into `hello_world_main.c` (or copy its contents) before
building:

| File | Purpose |
|---|---|
| `main/hello_world_main_baseline.c`  | Unprotected baseline: runs the model, prints result + latency |
| `main/hello_world_main_tampered.c`  | **Attack 2** — runs the model but reports a falsified class |
| `main/hello_world_main_vuln.c`      | **Attack 3** — adds a planted stack buffer-overflow bug + weight-leak routine |
| `main/hello_world_main.c` (current) | Protected-build status firmware: reports Secure Boot / Flash Encryption state, runs the model, keeps the Attack-3 code for re-testing |

## The three attacks

1. **Model extraction** — dump the flash with `esptool read-flash` and recover
   the weights. On the unprotected build the weights appear in cleartext; under
   Flash Encryption the dump is ciphertext.
2. **Firmware tampering** — flash a modified image. On the unprotected build it
   boots and lies; under Secure Boot V2 the modified image fails verification and
   will not boot.
3. **Buffer overflow** — send an over-long command into a 32-byte buffer to
   overwrite the saved return address. Helper: [`main/overflow_test.py`](main/overflow_test.py)
   (sends 80 bytes in one write) and `main/exploit3.py`. This is a **runtime**
   bug and is **not** blocked by Secure Boot or Flash Encryption.

## Build & flash

Unprotected build:

```bash
idf.py build
idf.py -p <PORT> flash monitor
```

Protected build (Secure Boot V2 + Flash Encryption enabled in `menuconfig`,
Security features; Development mode): once the eFuses are burned the device is
encrypted, so **always** use the encrypted flash command:

```bash
idf.py -p <PORT> encrypted-flash monitor
```

See the thesis, Chapter 4.5, for the full enabling procedure (key generation,
menuconfig options, partition-table offset change, first-boot eFuse burn).

## Results (summary)

| Attack | Unprotected | Protected |
|---|---|---|
| 1 — Flash dump | weights extracted | blocked (ciphertext) — *Flash Encryption* |
| 2 — Tampering | false result booted | blocked (will not boot) — *Secure Boot V2* |
| 3 — Buffer overflow | control-flow hijack | not blocked (runtime bug) |

Cost of protection: boot time 236 → 315 ms, inference latency 1,548 → 1,694 µs
(~9%), flash +~60 KB (bootloader roughly doubles).

## Security note

The Secure Boot signing key (`secure_boot_signing_key.pem`) is **intentionally
excluded** from this repository (see `.gitignore`) and must never be committed —
its secrecy is what Secure Boot relies on. Flash dumps and build artifacts
(`*.bin`, `build/`) are excluded as well.

## Author

Ton That Nam — Information and Communications Technology, Turku University of
Applied Sciences, 2026.
