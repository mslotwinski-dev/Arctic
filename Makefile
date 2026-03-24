CC = gcc
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector
LDFLAGS = -m32 -nostdlib -T src/boot/linker.ld -Wl,--build-id=none

BUILD_DIR = build

all: run

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: src/boot/boot.asm | $(BUILD_DIR)
	nasm -f elf32 src/boot/boot.asm -o $(BUILD_DIR)/boot.o

$(BUILD_DIR)/video.o: src/io/video.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/io/video.c -o $(BUILD_DIR)/video.o

$(BUILD_DIR)/kernel.o: src/kernel/kmain.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/kernel/kmain.c -o $(BUILD_DIR)/kernel.o

$(BUILD_DIR)/arctic.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/video.o $(BUILD_DIR)/kernel.o
	$(CC) $(LDFLAGS) $(BUILD_DIR)/boot.o $(BUILD_DIR)/video.o $(BUILD_DIR)/kernel.o -o $(BUILD_DIR)/arctic.bin

run: $(BUILD_DIR)/arctic.bin
	qemu-system-i386 -kernel $(BUILD_DIR)/arctic.bin

clean:
	rm -rf $(BUILD_DIR)