# Arctic

> A minimal, modular 32-bit operating system kernel written in C and NASM, targeting the x86 (i386) architecture. Arctic is designed as a learning-friendly yet industry-aligned reference kernel that boots via GRUB2, manages memory through a bitmap-based physical memory manager, and supports basic I/O, interrupt handling, a simple filesystem, and ELF application loading — all in under 32-bit Protected Mode.

---

## Table of Contents

- [Technology Stack](#technology-stack)
- [Memory Map](#memory-map)
- [Building & Running](#building--running)
  - [Prerequisites](#prerequisites)
  - [Cross-Compiler Setup](#cross-compiler-setup)
  - [Compile the Kernel](#compile-the-kernel)
  - [Run with QEMU](#run-with-qemu)
  - [Boot with GRUB](#boot-with-grub)
- [Kernel Structure](#kernel-structure)
  - [Bootloader & Entry Point](#bootloader--entry-point)
  - [Global Descriptor Table (GDT)](#global-descriptor-table-gdt)
  - [Interrupt Descriptor Table (IDT) & PIC](#interrupt-descriptor-table-idt--pic)
  - [VGA Text Mode Driver](#vga-text-mode-driver)
  - [Physical Memory Manager (PMM)](#physical-memory-manager-pmm)
  - [Kernel Heap (kmalloc)](#kernel-heap-kmalloc)
  - [Filesystem & Initrd](#filesystem--initrd)
  - [System Calls](#system-calls)
- [System Lifecycle](#system-lifecycle)
- [Application Loading](#application-loading)
- [Developer Guide](#developer-guide)
- [License](#license)

---

## Technology Stack

| Component       | Details                                                         |
|-----------------|-----------------------------------------------------------------|
| **Architecture**| x86 (i386), 32-bit Protected Mode                               |
| **Bootloader**  | Multiboot-compliant (GRUB2 or native QEMU `-kernel` loader)     |
| **Kernel Format**| ELF32 executable                                               |
| **Assembly**    | NASM — entry point (`_start`), ISR stubs, low-level helpers     |
| **C Standard**  | C99/C11 — kernel logic, memory management, drivers              |
| **Compiler**    | `i686-elf-gcc` with `-std=gnu99 -ffreestanding -O2 -Wall -Wextra` |
| **Linker Script**| `linker.ld` — controls section placement and kernel load address |
| **Emulator**    | QEMU (`qemu-system-i386`) for development and testing           |

---

## Memory Map

```
0x00000000 - 0x000FFFFF   Reserved (BIOS, VGA buffer at 0xB8000)
0x00100000                Kernel load address (.text, .data, .bss)
                          ├── .text   — executable code
                          ├── .rodata — read-only constants
                          ├── .data   — initialised global data
                          └── .bss    — zero-initialised data
                                        └── Kernel stack (16 KB, grows down)
[after .bss]              Kernel heap — initialised after parsing multiboot_info_t
[GRUB modules region]     Initrd / ramdisk — TAR archive loaded by GRUB into free RAM
```

- The linker script (`linker.ld`) places the Multiboot header at the very start of `.text` so GRUB can locate the magic value `0x1BADB002`.
- The kernel stack is a statically allocated 16 KB buffer in `.bss`; `_start` sets `ESP` to its top before calling `kmain`.
- Heap start address is computed at runtime by scanning the memory map provided by `multiboot_info_t`.

---

## Building & Running

### Prerequisites

- Linux host (Ubuntu 22.04+ recommended) or WSL2
- NASM assembler (`nasm`)
- QEMU (`qemu-system-i386`)
- GRUB2 utilities (`grub-mkrescue`, `grub-pc-bin`, `xorriso`) — for ISO creation
- `i686-elf` cross-compiler toolchain (see below)

### Cross-Compiler Setup

Arctic requires a freestanding cross-compiler that targets `i686-elf` so that no host libc assumptions bleed into the kernel.

```bash
# Quick install via a pre-built toolchain (example — adjust for your distro)
sudo apt install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo

# Build binutils for i686-elf
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

tar xf binutils-*.tar.xz && mkdir build-binutils && cd build-binutils
../binutils-*/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make -j$(nproc) && make install && cd ..

# Build GCC (core only — no libc needed)
tar xf gcc-*.tar.xz && mkdir build-gcc && cd build-gcc
../gcc-*/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --without-headers
make -j$(nproc) all-gcc all-target-libgcc
make install-gcc install-target-libgcc
```

Verify the toolchain:

```bash
i686-elf-gcc --version
```

### Compile the Kernel

```bash
make          # assembles NASM sources, compiles C sources, links kernel.elf
make clean    # removes all build artifacts
```

The `Makefile` invokes `i686-elf-gcc` with:

```
-std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib
```

and links with `linker.ld` to produce `kernel.elf` (ELF32).

### Run with QEMU

```bash
# Boot the kernel directly (no GRUB required — QEMU acts as a Multiboot loader)
qemu-system-i386 -kernel kernel.elf

# With a ramdisk (initrd) module
qemu-system-i386 -kernel kernel.elf -initrd initrd.tar

# With serial output visible on the host terminal
qemu-system-i386 -kernel kernel.elf -serial stdio
```

### Boot with GRUB

1. Build a bootable ISO:

   ```bash
   make iso      # creates arctic.iso via grub-mkrescue
   ```

2. Boot the ISO:

   ```bash
   qemu-system-i386 -cdrom arctic.iso
   ```

   GRUB reads `grub.cfg`, loads `kernel.elf` and any modules (initrd), validates the Multiboot header, and jumps to `_start`.

---

## Kernel Structure

```
src/
├── boot/
│   ├── boot.asm          # _start, Multiboot header, early stack setup
│   └── linker.ld         # Memory layout script
├── kernel/
│   ├── kmain.c           # Kernel entry: initialises all subsystems
│   ├── gdt.c / gdt.h     # Global Descriptor Table
│   ├── idt.c / idt.h     # Interrupt Descriptor Table
│   ├── isr.asm           # ISR stubs (exceptions 0-31, IRQs 32-47)
│   └── pic.c             # 8259A PIC remapping
├── drivers/
│   ├── vga.c / vga.h     # VGA text-mode driver (0xB8000)
│   ├── keyboard.c        # Keyboard IRQ1 handler & scancode decoder
│   └── io.h              # inb() / outb() inline assembly
├── memory/
│   ├── pmm.c / pmm.h     # Physical memory manager (bitmap allocator)
│   └── heap.c / heap.h   # Kernel heap (kmalloc / kfree)
├── fs/
│   ├── initrd.c          # TAR-based ramdisk parser
│   └── vfs.c             # Minimal VFS shim
├── proc/
│   ├── elf.c             # ELF32 loader
│   └── syscall.c         # int 0x80 dispatcher
└── include/              # Shared headers
```

### Bootloader & Entry Point

- **`boot.asm`** declares the Multiboot header with magic `0x1BADB002`, flags, and checksum so GRUB can verify the image.
- `_start` sets `ESP` to the top of the 16 KB kernel stack (in `.bss`), pushes the GRUB magic number and `multiboot_info_t *` onto the stack, and calls `kmain`.
- A20 line enabling and the switch to 32-bit flat Protected Mode are handled by GRUB before handing control to the kernel.

### Global Descriptor Table (GDT)

- Three descriptors: null, 32-bit flat code segment (`0x00000000-0xFFFFFFFF`, ring 0), and 32-bit flat data segment (same range, ring 0).
- Loaded with `lgdt`; a far jump flushes the segment registers and ensures the CPU is executing from the new CS.
- A Task State Segment (TSS) entry can be added here when user-mode ring-3 processes are introduced.

### Interrupt Descriptor Table (IDT) & PIC

- **IDT**: 256 entries covering CPU exceptions (vectors 0-31) and hardware IRQs (vectors 32-47).
- **ISR stubs** (`isr.asm`): generated for every vector; they push a common frame and call a C dispatcher that dispatches to registered handlers.
- **PIC (8259A)**: remapped so that IRQ0-IRQ7 map to IDT vectors 32-39 and IRQ8-IRQ15 map to 40-47, avoiding conflicts with CPU exception vectors.
- **Keyboard** (IRQ1 → IDT 33): reads a scancode from I/O port `0x60`, translates it to ASCII via a lookup table, and buffers the character for the kernel to consume.

### VGA Text Mode Driver

- Writes directly to the memory-mapped frame buffer at physical address `0xB8000`.
- Each cell is 2 bytes: character byte + attribute byte (foreground/background colour nibbles).
- Cursor position is managed via I/O ports `0x3D4` (index) and `0x3D5` (data).
- Public API: `vga_putchar()`, `vga_puts()`, `vga_clear()`, `vga_set_color()`.

### Physical Memory Manager (PMM)

- Parses the `multiboot_info_t` memory map at boot to discover usable RAM regions.
- Maintains a **bitmap** of 4 KB frames: one bit per frame, `0` = free, `1` = used.
- Core functions: `pmm_alloc_frame()` → returns a physical address; `pmm_free_frame()` → returns a frame to the pool.
- The kernel image, stack, and heap regions are marked used during initialisation so they are never handed out.
- Optional VMM layer adds CR3/page-table management on top of PMM for process isolation.

### Kernel Heap (kmalloc)

- A simple slab/bump allocator built on top of `pmm_alloc_frame()`.
- Maintains a linked list of free blocks within the heap region; merges adjacent free blocks on `kfree()` to limit fragmentation.
- API mirrors the C standard: `kmalloc(size)`, `kcalloc(n, size)`, `krealloc(ptr, size)`, `kfree(ptr)`.
- The heap start pointer is set in `kmain` after the PMM is initialised; the heap grows on demand by requesting additional frames from PMM.

### Filesystem & Initrd

- GRUB loads a **TAR archive** (the initrd) as a Multiboot module into free RAM and passes a pointer to `kmain` via `multiboot_info_t.mods_addr`.
- `initrd.c` walks the TAR header chain to build an in-memory directory of file names and their byte offsets/sizes within the archive.
- A minimal **VFS shim** exposes `vfs_open()`, `vfs_read()`, `vfs_close()` so higher-level code is decoupled from the initrd implementation — swapping in a real filesystem driver later requires no changes to callers.

### System Calls

- Invoked via `int 0x80`; the IDT vector 0x80 handler saves all registers (`pusha`) and dispatches on `EAX` (syscall number).
- Arguments follow the Linux i386 ABI: `EBX`, `ECX`, `EDX`, `ESI`, `EDI` hold arguments; `EAX` receives the return value.
- Current syscall table (example):

  | EAX | Name       | Description                       |
  |-----|------------|-----------------------------------|
  | 0   | `sys_exit` | Terminate current process         |
  | 1   | `sys_write`| Write bytes to VGA / serial       |
  | 2   | `sys_read` | Read from keyboard buffer         |
  | 3   | `sys_open` | Open a file from the initrd       |

---

## System Lifecycle

```
Power on
  └─► BIOS / UEFI (CSM) POST
        └─► GRUB2 bootloader
              ├─ Reads grub.cfg
              ├─ Loads kernel.elf (ELF32) at 0x00100000
              ├─ Loads initrd.tar as a Multiboot module
              └─ Validates Multiboot header (0x1BADB002)
                    └─► _start  (boot.asm)
                          ├─ Sets ESP to top of kernel stack
                          ├─ Pushes multiboot_info_t pointer & magic
                          └─► kmain  (kmain.c)
                                ├─ vga_init()       — clear screen, set colour
                                ├─ gdt_init()       — load flat GDT
                                ├─ idt_init()       — register exception & IRQ handlers
                                ├─ pic_remap()      — remap 8259A PIC
                                ├─ pmm_init()       — build frame bitmap from memory map
                                ├─ heap_init()      — initialise kernel heap
                                ├─ initrd_init()    — parse TAR module from GRUB
                                ├─ keyboard_init()  — enable IRQ1
                                ├─ sti             — enable interrupts
                                └─ load + run first ELF application from initrd
```

---

## Application Loading

1. **Locate** the ELF binary inside the initrd by name (e.g., `"init"`).
2. **Validate** the ELF32 magic (`0x7F 'E' 'L' 'F'`), machine type (`EM_386`), and `e_type` (`ET_EXEC`).
3. **Allocate** physical frames for each `PT_LOAD` program header using `pmm_alloc_frame()`.
4. **Copy** each loadable segment from the initrd buffer into its allocated frames at the virtual address specified by `p_vaddr`.
5. **Zero** any BSS region (`p_filesz < p_memsz`).
6. **(Optional)** If paging/VMM is enabled, map the frames into a new page directory and switch CR3.
7. **Jump** to `e_entry` — the ELF entry point — either directly (ring 0 app) or via an `iret` frame (ring 3 userspace).

---

## Developer Guide

### Adding a New Driver

1. Create `src/drivers/mydriver.c` and `src/drivers/mydriver.h`.
2. Initialise the driver in `kmain.c` (call an `init` function after `idt_init()`).
3. If the driver needs an IRQ, register it via `idt_register_handler(vector, handler)`.
4. Add `src/drivers/mydriver.o` to the `Makefile` object list.

### Adding a New Syscall

1. Define a handler function `sys_myname(args...)` in `src/proc/syscall.c`.
2. Add an entry to the syscall dispatch table with the next available EAX value.
3. Document the calling convention (registers used, return value, error codes).

### Enabling Paging (VMM)

1. Build a page directory and page tables covering the kernel's virtual range.
2. Use `pmm_alloc_frame()` to back each page table.
3. Write the page directory address to CR3 and set bit 31 of CR0.
4. Update the linker script if you move the kernel to a higher-half virtual address (typically `0xC0100000`).

### Debugging Tips

- **QEMU monitor**: run with `-monitor stdio` and use `info mem`, `info registers`, `x /20i $eip`.
- **Serial output**: route `vga_puts` to port `0x3F8` (COM1) and launch QEMU with `-serial stdio` to see kernel logs without a graphical window.
- **GDB stub**: `qemu-system-i386 -s -S` starts QEMU in a halted state listening on port 1234; connect with `target remote :1234` in GDB and load the symbol file with `symbol-file kernel.elf`.
- **Page faults**: the exception 14 handler should print CR2 (faulting address) and the error code to help pinpoint bad pointer dereferences.

### Code Style

- Follow C99; keep each subsystem in its own `src/<subsystem>/` directory with a matching header.
- Name kernel-internal functions with a subsystem prefix, e.g., `pmm_alloc_frame`, `vga_putchar`.
- Keep interrupt handlers (`isr_*`) as short as possible — defer work to normal kernel context where possible.

---

## License

This project is licensed under the terms of the [LICENSE](LICENSE) file in the repository root.
