PROJECT_NAME ?= logo

PROJECT_OBJS ?= main.o logo.o frame.o

RISCV_TOOLCHAIN ?= /opt/tinyQV

CC = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-gcc
AS = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-as
AR = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ar
LD = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ld
OBJCOPY = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-objcopy

TINYQV_SDK ?= ../../tinyQV-sdk

all: $(PROJECT_NAME).bin $(PROJECT_NAME).hex

clean:
	cd $(dir $(PROJECT_NAME)) && rm *.o *.elf *.bin *.hex

%.o: %.c
	$(CC) -O2 -I$(TINYQV_SDK) -march=rv32ec_zicsr_zcb_zicond -mabi=ilp32e -mno-strict-align -nostdlib -nostartfiles -ffreestanding -ffunction-sections -fdata-sections -Wall -Werror -Wno-unused-function -Wno-format -lc -c $< -o $@

%.o: %.s
	$(AS) -march=rv32ec_zicsr_zcb_zicond -mabi=ilp32e $< -o $@

$(PROJECT_NAME).elf: $(PROJECT_OBJS)
	$(LD) $^ $(TINYQV_SDK)/start.o $(TINYQV_SDK)/tinyQV.a $(RISCV_TOOLCHAIN)/riscv32-unknown-elf/lib/libc.a $(RISCV_TOOLCHAIN)/riscv32-unknown-elf/lib/libm.a $(RISCV_TOOLCHAIN)/lib/gcc/riscv32-unknown-elf/*/libgcc.a  -T $(TINYQV_SDK)/memmap --gc-sections -o $@

$(PROJECT_NAME).bin: $(PROJECT_NAME).elf
	$(OBJCOPY) $< -O binary $@

$(PROJECT_NAME).hex: $(PROJECT_NAME).bin
	od -An -t x1 -w4 -v $< > $@
