# REB-01 Remote Entropy Beacon

Bare-metal firmware for the STM32L432KC (NUCLEO-L432KC). Currently runs a blink test to verify toolchain and MCU operation.

## Prerequisites

- `arm-none-eabi-gcc` toolchain
- `openocd` (for flashing/debugging)
- `make`

## Setup

Clone with submodules (pulls in STM32CubeL4 HAL drivers):

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
```

## Debug

```
make debug
```

Starts OpenOCD and connects GDB.

## Project Structure

```
firmware/
  src/            Application source (main.c, interrupts, system init)
  include/        Project headers (HAL config)
  startup/        Startup assembly
  drivers/        STM32CubeL4 HAL (git submodule)
  STM32L432KC.ld  Linker script
  Makefile
```
