# Entropy Beacon

Bare-metal STM32L432KC firmware that harvests physical entropy from an ADXL345 accelerometer and BH1750 light sensor, runs NIST SP 800-90B health tests, and outputs conditioned 256-bit random values signed with Ed25519.

## Prerequisites

- `arm-none-eabi-gcc` toolchain
- `openocd` (for flashing/debugging)
- `make`

## Setup

Clone with submodules:

```
git clone --recurse-submodules <repo-url>
```

If already cloned:

```
git submodule update --init --recursive
```

## Build

```
cd firmware
make
```

Outputs `build/reb-01.elf`, `.hex`, and `.bin`.

## Flash

Connect a NUCLEO-L432KC via USB, then:

```
make flash
make flash-verbose   # includes per-burst diagnostics
```

## Serial Output

The ST-Link provides a virtual COM port over USB at 115200 baud (USART2, PA2 TX / PA15 RX).

```
ls /dev/tty.usbmodem*
screen /dev/tty.usbmodem<TAB> 115200
```

Exit screen: `Ctrl-A` then `K`, confirm with `Y`.

## Capture Entropy

Record conditioned output to a binary file (requires `pip install pyserial`):

```
python3 firmware/tools/capture.py /dev/tty.usbmodem<TAB> entropy.bin
```

## Debug

```
make debug
```

## Tests

Entropy pipeline and health test logic tested natively on the host:

```
cd firmware/test
make
```

## Project Structure

```
firmware/
  src/            Application source
  src/entropy/    Entropy pipeline (extraction, health tests, pool, conditioning)
  include/        Project headers
  startup/        Startup assembly
  lib/cfx/        Crypto primitives (SHA-256, Ed25519) — git submodule
  drivers/        STM32CubeL4 HAL — git submodule
  tools/          Host utilities (capture script)
  test/           Native unit tests
  STM32L432KC.ld  Linker script
  Makefile
```
