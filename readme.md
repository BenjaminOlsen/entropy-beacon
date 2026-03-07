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

## Serial Output

The Nucleo's ST-Link provides a virtual COM port over the same USB cable used for flashing. Firmware prints sensor readings to USART2 (PA2 TX / PA15 RX) at 115200 baud.

Check the board is connected:

```
system_profiler SPUSBDataType | grep -A5 "STLink"
ls /dev/tty.usbmodem*
```

Open the serial console:

```
screen /dev/tty.usbmodem<TAB> 115200
```

Exit screen: `Ctrl-A` then `K`, confirm with `Y`.

Press the **reset button** on the Nucleo to see boot messages from the beginning.

## Capture Entropy

Record conditioned output to a binary file (requires `pip install pyserial`):

```
python3 firmware/tools/capture.py /dev/tty.usbmodem<TAB> entropy.bin
```

Use `make flash-verbose` to see per-burst diagnostics while capturing.

## Debug

```
make debug
```

Starts OpenOCD and connects GDB.

## Tests

Pure logic (entropy pipeline, health tests, etc.) is tested natively on the host:

```
cd firmware/test
make
```

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
