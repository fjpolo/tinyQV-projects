NAME = rv2a03

PROJECT_SOURCES ?= rv2a03.c main.c

# --- Toolchain Definitions ---
RISCV_TOOLCHAIN ?= /opt/tinyQV

CC = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-gcc
AS = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-as
AR = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ar
LD = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-ld
OBJCOPY = $(RISCV_TOOLCHAIN)/bin/riscv32-unknown-elf-objcopy

TINYQV_SDK ?= ../../tinyQV-sdk

# --- Source and Object File Handling ---
C_SOURCES = $(filter %.c, $(PROJECT_SOURCES))
AS_SOURCES = $(filter %.s, $(PROJECT_SOURCES))

# List of intermediate assembly files (generated from C sources)
AS_SOURCES_INTERMEDIATE = $(C_SOURCES:.c=.s)

# Combined list of all object files needed for linking
OBJECTS = $(C_SOURCES:.c=.o) $(AS_SOURCES:.s=.o)

# --- Targets ---

# The 'all' target now also ensures all intermediate .s files are generated
all: $(NAME).bin $(NAME).hex $(NAME).mem $(AS_SOURCES_INTERMEDIATE)

clean:
	# Removed the unnecessary 'cd' command and added .s files to cleanup
	rm -f *.o *.elf *.bin *.hex *.mem *.s

# --- NEW RULE: Generate Intermediate Assembly File (.s) from C File (.c) ---
# The -S flag tells GCC to compile the C file but stop before assembly,
# outputting the result to a .s file.
%.s: %.c
	$(CC) -O2 -I$(TINYQV_SDK) -march=rv32ec_zcb -mabi=ilp32e -mno-strict-align -nostdlib -nostartfiles -ffreestanding -ffunction-sections -fdata-sections -Wall -Werror -lc -S $< -o $@

# Rule for C files: now depends on the C file, but uses the compiler to generate the .o directly
# This is typically faster than having the .o target explicitly depend on the new .s target.
%.o: %.c
	$(CC) -O2 -I$(TINYQV_SDK) -march=rv32ec_zcb -mabi=ilp32e -mno-strict-align -nostdlib -nostartfiles -ffreestanding -ffunction-sections -fdata-sections -Wall -Werror -lc -c $< -o $@

# Rule for Assembly files (unchanged)
%.o: %.s
	$(AS) -march=rv32ec_zcb -mabi=ilp32e $< -o $@

# Linking the ELF (Updated to use $(OBJECTS) list)
$(NAME).elf: $(OBJECTS)
	$(LD) $^ $(TINYQV_SDK)/start.o $(TINYQV_SDK)/tinyQV-sim.a $(RISCV_TOOLCHAIN)/riscv32-unknown-elf/lib/libc.a $(RISCV_TOOLCHAIN)/lib/gcc/riscv32-unknown-elf/*/libgcc.a -T $(TINYQV_SDK)/memmap_sim --gc-sections -o $@

# Extract raw binary (unchanged)
$(NAME).bin: $(NAME).elf
	$(OBJCOPY) $< -O binary $@

# Existing OD-based hex conversion
$(NAME).hex: $(NAME).bin
	od -An -t x1 -w4 -v $< > $@

# Generate clean Verilog Memory File (.mem)
$(NAME).mem: $(NAME).bin
	xxd -c 4 -p $< > $@
