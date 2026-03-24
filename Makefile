CC = i686-elf-gcc
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector
LDFLAGS = -m32 -nostdlib -T src/boot/linker.ld -Wl,--build-id=none

BUILD_DIR = build

all: run

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: src/boot/boot.asm | $(BUILD_DIR)
	nasm -f elf32 src/boot/boot.asm -o $(BUILD_DIR)/boot.o

$(BUILD_DIR)/interrupt.o: src/arch/interrupt.asm | $(BUILD_DIR)
	nasm -f elf32 src/arch/interrupt.asm -o $(BUILD_DIR)/interrupt.o

$(BUILD_DIR)/print.o: src/io/print.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/io/print.c -o $(BUILD_DIR)/print.o

$(BUILD_DIR)/gdt.o: src/arch/gdt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/arch/gdt.c -o $(BUILD_DIR)/gdt.o

$(BUILD_DIR)/idt.o: src/arch/idt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/arch/idt.c -o $(BUILD_DIR)/idt.o

$(BUILD_DIR)/pic.o: src/arch/pic.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/arch/pic.c -o $(BUILD_DIR)/pic.o

$(BUILD_DIR)/interrupt_handlers.o: src/arch/interrupt_handlers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/arch/interrupt_handlers.c -o $(BUILD_DIR)/interrupt_handlers.o

$(BUILD_DIR)/keyboard.o: src/drivers/keyboard.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/drivers/keyboard.c -o $(BUILD_DIR)/keyboard.o

$(BUILD_DIR)/kernel.o: src/kernel/kmain.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/kernel/kmain.c -o $(BUILD_DIR)/kernel.o

$(BUILD_DIR)/arctic.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/print.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/kernel.o
	$(CC) $(LDFLAGS) $(BUILD_DIR)/boot.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/print.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/kernel.o -o $(BUILD_DIR)/arctic.bin

run: $(BUILD_DIR)/arctic.bin
	qemu-system-i386 -kernel $(BUILD_DIR)/arctic.bin

clean:
	rm -rf $(BUILD_DIR)