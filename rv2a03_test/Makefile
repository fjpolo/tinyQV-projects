PROJECT_NAME ?= rv2a03_test

PROJECT_SOURCES ?= main.c

RISCV_TOOLCHAIN ?= /opt/tinyQV

CC = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-gcc
AS = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-as
AR = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ar
LD = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ld
OBJCOPY = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-objcopy

ifneq ($(wildcard ../../tinyQV-sdk-fjpolo/timer.h),)
TINYQV_SDK ?= ../../tinyQV-sdk-fjpolo
else
TINYQV_SDK ?= ../../tinyQV-sdk
endif

all: $(PROJECT_NAME).bin $(PROJECT_NAME).hex

clean:
	rm -f *.o *.elf *.bin *.hex

CFLAGS ?=

ifeq ($(findstring SIM,$(CFLAGS)),SIM)
TINYQV_LIB ?= $(TINYQV_SDK)/tinyQV-sim.a
MEMMAP ?= $(TINYQV_SDK)/memmap_sim
else
TINYQV_LIB ?= $(TINYQV_SDK)/tinyQV.a
MEMMAP ?= $(TINYQV_SDK)/memmap
endif

%.o: %.c
	$(CC) $(CFLAGS) -O2 -I$(TINYQV_SDK) -I$(TINYQV_SDK)/peripherals -march=rv32ec_zicsr_zcb_zicond -mabi=ilp32e -mno-strict-align -nostdlib -nostartfiles -ffreestanding -ffunction-sections -fdata-sections -Wall -Werror -lc -c $< -o $@

%.o: %.s
	$(AS) -march=rv32ec_zicsr_zcb_zicond -mabi=ilp32e $< -o $@

$(PROJECT_NAME).elf: $(PROJECT_SOURCES:.c=.o) $(TINYQV_SDK)/peripherals/rv2a03.o
	$(LD) $^ $(TINYQV_SDK)/start.o $(TINYQV_LIB) $(RISCV_TOOLCHAIN)/riscv32-unknown-elf/lib/libc.a $(RISCV_TOOLCHAIN)/lib/gcc/riscv32-unknown-elf/*/libgcc.a  -T $(MEMMAP) --gc-sections -o $@

$(PROJECT_NAME).bin: $(PROJECT_NAME).elf
	$(OBJCOPY) $< -O binary $@

$(PROJECT_NAME).hex: $(PROJECT_NAME).bin
	od -An -t x1 -w4 -v $< > $@
