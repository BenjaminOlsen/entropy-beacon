######################################################################
# REB-01 Remote Entropy Beacon — Firmware Makefile
# Target: STM32L432KC (NUCLEO-L432KC)
######################################################################

# Toolchain
PREFIX = arm-none-eabi-
CC     = $(PREFIX)gcc
AS     = $(PREFIX)gcc -x assembler-with-cpp
CP     = $(PREFIX)objcopy
SZ     = $(PREFIX)size

# Target
TARGET = reb-01

# Build directory
BUILD_DIR = build

# Paths
CUBE_DIR    = drivers/STM32CubeL4/Drivers
HAL_DIR     = $(CUBE_DIR)/STM32L4xx_HAL_Driver
CMSIS_DIR   = $(CUBE_DIR)/CMSIS
CMSIS_DEV   = $(CMSIS_DIR)/Device/ST/STM32L4xx
CFX_DIR     = lib/cfx

# Sources — application
C_SOURCES = \
    src/main.c \
    src/stm32l4xx_it.c \
    src/system_stm32l4xx.c \
    src/adxl345.c \
    src/bh1750.c \
    src/uart.c \
    src/entropy/entropy.c

# Sources — HAL (only what we need)
C_SOURCES += \
    $(HAL_DIR)/Src/stm32l4xx_hal.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_rcc.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_rcc_ex.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_gpio.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_cortex.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_pwr.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_pwr_ex.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_flash.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_flash_ex.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_flash_ramfunc.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_dma.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_i2c.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_i2c_ex.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_uart.c \
    $(HAL_DIR)/Src/stm32l4xx_hal_uart_ex.c

# Startup
ASM_SOURCES = \
    startup/startup_stm32l432xx.s

# Includes
C_INCLUDES = \
    -Iinclude \
    -I$(HAL_DIR)/Inc \
    -I$(CMSIS_DEV)/Include \
    -I$(CMSIS_DIR)/Include \
    -I$(CFX_DIR)/include

# MCU flags
CPU    = -mcpu=cortex-m4
FPU    = -mfpu=fpv4-sp-d16
FLOAT  = -mfloat-abi=hard
MCU    = $(CPU) -mthumb $(FPU) $(FLOAT)

# C flags
CFLAGS  = $(MCU) $(C_INCLUDES) -DSTM32L432xx -DUSE_HAL_DRIVER
CFLAGS += -Wall -Wextra -Og -g -gdwarf-2
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -std=c11

# Assembler flags
ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

# Linker flags
LDSCRIPT = STM32L432KC.ld
LDFLAGS  = $(MCU) -specs=nano.specs -T$(LDSCRIPT)
LDFLAGS += -lc -lm -lnosys
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections

######################################################################
# Build rules
######################################################################

# Object files
OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(SZ) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CP) -O binary -S $< $@

$(BUILD_DIR):
	mkdir -p $@

######################################################################
# Flash & debug
######################################################################

flash: $(BUILD_DIR)/$(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "program $(BUILD_DIR)/$(TARGET).bin 0x08000000 verify reset exit"

debug: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg &
	$(PREFIX)gdb -ex "target remote :3333" -ex "monitor reset halt" $<

######################################################################
# Clean
######################################################################

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash debug
