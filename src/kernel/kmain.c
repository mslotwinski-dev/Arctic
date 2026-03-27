#include <stdint.h>

#include "../arch/gdt.h"
#include "../arch/idt.h"
#include "../arch/pic.h"
#include "../arch/pit.h"
#include "../drivers/keyboard.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../io/print.h"
#include "../memory/heap.h"
#include "../memory/paging.h"
#include "../memory/pmm.h"
#include "../proc/elf.h"

/**
 * @file kmain.c
 * @brief Kernel boot-time orchestration and subsystem smoke tests.
 * @ingroup kernel_core
 */

#ifndef ELF_EXECUTE_ON_BOOT
/** @brief Compile-time flag enabling execution of loaded ELF entry. */
#define ELF_EXECUTE_ON_BOOT 0
#endif

#ifndef PAGING_ENABLE_ON_BOOT
/** @brief Compile-time flag enabling paging during boot sequence. */
#define PAGING_ENABLE_ON_BOOT 0
#endif

#ifndef ELF_VADDR_LOAD_ON_BOOT
/** @brief Compile-time flag enabling identity-virtual ELF loading diagnostics. */
#define ELF_VADDR_LOAD_ON_BOOT 0
#endif

/** @brief Assembly helper that executes STI to enable CPU interrupts. */
extern void enable_interrupts(void);
/** @brief Linker symbol: kernel image start address. */
extern uint8_t _kernel_start;
/** @brief Linker symbol: kernel image end address. */
extern uint8_t _kernel_end;

static void print_uint(uint32_t value) {
  char buffer[11];
  int pos = 0;

  if (value == 0) {
    printc('0');
    return;
  }

  while (value > 0 && pos < 10) {
    buffer[pos++] = (char)('0' + (value % 10));
    value /= 10;
  }

  for (int i = pos - 1; i >= 0; i--) {
    printc(buffer[i]);
  }
}

void kernel_main(uint32_t magic, uint32_t multiboot_info) __asm__("kernel_main");

/**
 * @brief Render startup ASCII banner.
 */
void hello_world(void) {
  println("  ,---.  ,------.  ,-----.,--------.,--. ,-----.                   ");
  println(" /  O  \\ |  .--. ''  .--./'--.  .--'|  |'  .--./     ,---.  ,---.  ");
  println("|  .-.  ||  '--'.'|  |       |  |   |  ||  |        | .-. |(  .-'  ");
  println("|  | |  ||  |\\  \\ '  '--'\\   |  |   |  |'  '--'\\    ' '-' '.-'  `) ");
  println("`--' `--'`--' '--' `-----'   `--'   `--' `-----'     `---' `----'  ");
}

static void run_memory_smoke_tests(void) {
  uint32_t before = pmm_get_free_frames();
  uint32_t frame = pmm_alloc_frame();
  uint32_t after_alloc = pmm_get_free_frames();
  void* a;
  void* b;
  void* c;

  print("PMM test: ");
  if (frame == 0 || after_alloc + 1U != before) {
    println("FAIL");
  } else {
    pmm_free_frame(frame);
    if (pmm_get_free_frames() != before) {
      println("FAIL");
    } else {
      println("OK");
    }
  }

  a = kmalloc(64);
  b = kmalloc(128);
  if (a == 0 || b == 0) {
    println("HEAP test: FAIL");
    return;
  }

  kfree(a);
  c = kmalloc(32);

  print("HEAP test: ");
  if (c == 0) {
    println("FAIL");
  } else {
    println("OK");
  }

  kfree(b);
  kfree(c);
}

static void run_vfs_smoke_test(void) {
  uint32_t file_count = initrd_file_count();

  if (initrd_is_ready() == 0U || file_count == 0U) {
    println("VFS test: SKIP");
    return;
  }

  {
    const char* name = initrd_get_file_name(0);
    vfs_file_t file;
    char preview[33];
    uint32_t bytes_read;

    if (name == 0) {
      println("VFS test: FAIL");
      return;
    }

    file = vfs_open(name);
    if (file.valid == 0U) {
      println("VFS test: FAIL");
      return;
    }

    bytes_read = vfs_read(&file, preview, 32);
    vfs_close(&file);

    print("VFS test: OK ");
    print(name);
    print(" [");
    print_uint(bytes_read);
    print("] ");

    for (uint32_t i = 0; i < bytes_read; i++) {
      char c = preview[i];
      if (c >= 32 && c <= 126) {
        printc(c);
      } else {
        printc('.');
      }
    }
    printc('\n');
  }
}

static void run_syscall_smoke_test(void) {
  static const char msg[] = "[sys_write via int 0x80]\n";
  uint32_t written;

  __asm__ __volatile__("int $0x80" : "=a"(written) : "a"(1U), "b"(1U), "c"(msg), "d"((uint32_t)(sizeof(msg) - 1U)) : "memory");

  print("sys_write ret=");
  print_uint(written);
  printc('\n');

  if (initrd_is_ready() != 0U && initrd_file_count() > 0U) {
    const char* name = initrd_get_file_name(0);
    uint32_t fd;
    char buffer_a[9];
    char buffer_b[9];
    uint32_t read_a;
    uint32_t seek_ret;
    uint32_t read_b;
    uint32_t close_ret;
    uint32_t same = 1U;

    if (name == 0) {
      return;
    }

    __asm__ __volatile__("int $0x80" : "=a"(fd) : "a"(3U), "b"((uint32_t)name) : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(read_a) : "a"(2U), "b"(fd), "c"((uint32_t)buffer_a), "d"(8U) : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(seek_ret) : "a"(5U), "b"(fd), "c"(0U), "d"(0U) : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(read_b) : "a"(2U), "b"(fd), "c"((uint32_t)buffer_b), "d"(8U) : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(close_ret) : "a"(4U), "b"(fd) : "memory");

    if (read_a != read_b) {
      same = 0U;
    }

    for (uint32_t i = 0; i < read_a && i < read_b; i++) {
      if (buffer_a[i] != buffer_b[i]) {
        same = 0U;
        break;
      }
    }

    print("sys_open fd=");
    print_uint(fd);
    print(" sys_read_a=");
    print_uint(read_a);
    print(" sys_seek=");
    print_uint(seek_ret);
    print(" sys_read_b=");
    print_uint(read_b);
    print(" sys_close=");
    print_uint(close_ret);
    print(" seek_match=");
    print_uint(same);
    print(" ");

    for (uint32_t i = 0; i < read_a && i < 8U; i++) {
      char c = buffer_a[i];
      if (c >= 32 && c <= 126) {
        printc(c);
      } else {
        printc('.');
      }
    }
    printc('\n');
  }
}

static void run_elf_smoke_test(void) {
  uint32_t file_count = initrd_file_count();
  uint32_t entry = 0;
  uint32_t entry_loaded = 0;
  uint32_t segments = 0;
  uint32_t loaded_bytes = 0;
#if ELF_VADDR_LOAD_ON_BOOT
  uint32_t vaddr_entry = 0;
  uint32_t vaddr_segments = 0;
  uint32_t vaddr_bytes = 0;
#endif

  if (initrd_is_ready() == 0U || file_count == 0U) {
    println("ELF test: SKIP");
    return;
  }

  if (elf_load_image_from_vfs("init", &entry, &entry_loaded, &segments, &loaded_bytes) != 0U) {
    print("ELF test: LOAD init entry=");
    print_uint(entry);
    print(" loaded=");
    print_uint(entry_loaded);
    print(" seg=");
    print_uint(segments);
    print(" bytes=");
    print_uint(loaded_bytes);
    printc('\n');

#if ELF_EXECUTE_ON_BOOT
    if (elf_try_execute_loaded_entry(entry_loaded) != 0U) {
      println("ELF exec: OK");
    } else {
      println("ELF exec: FAIL");
    }
#else
    println("ELF exec: disabled");
#endif

#if ELF_VADDR_LOAD_ON_BOOT
    if (elf_load_virtual_identity_from_vfs("init", &vaddr_entry, &vaddr_segments, &vaddr_bytes) != 0U) {
      print("ELF vaddr: OK entry=");
      print_uint(vaddr_entry);
      print(" seg=");
      print_uint(vaddr_segments);
      print(" bytes=");
      print_uint(vaddr_bytes);
      printc('\n');
    } else {
      println("ELF vaddr: FAIL");
    }
#else
    println("ELF vaddr: disabled");
#endif
    return;
  }

  for (uint32_t i = 0; i < file_count; i++) {
    const char* name = initrd_get_file_name(i);
    if (name == 0) {
      continue;
    }

    if (elf_load_image_from_vfs(name, &entry, &entry_loaded, &segments, &loaded_bytes) != 0U) {
      print("ELF test: LOAD ");
      print(name);
      print(" entry=");
      print_uint(entry);
      print(" loaded=");
      print_uint(entry_loaded);
      print(" seg=");
      print_uint(segments);
      print(" bytes=");
      print_uint(loaded_bytes);
      printc('\n');

#if ELF_EXECUTE_ON_BOOT
      if (elf_try_execute_loaded_entry(entry_loaded) != 0U) {
        println("ELF exec: OK");
      } else {
        println("ELF exec: FAIL");
      }
#else
      println("ELF exec: disabled");
#endif

#if ELF_VADDR_LOAD_ON_BOOT
  if (elf_load_virtual_identity_from_vfs(name, &vaddr_entry, &vaddr_segments, &vaddr_bytes) != 0U) {
    print("ELF vaddr: OK entry=");
    print_uint(vaddr_entry);
    print(" seg=");
    print_uint(vaddr_segments);
    print(" bytes=");
    print_uint(vaddr_bytes);
    printc('\n');
  } else {
    println("ELF vaddr: FAIL");
  }
#else
  println("ELF vaddr: disabled");
#endif
      return;
    }
  }

  println("ELF test: SKIP");
}

/**
 * @brief Main C entry point called from early boot assembly.
 *
 * Brings up core architecture and runtime subsystems, initializes memory,
 * optional initrd/VFS, and executes basic diagnostics.
 *
 * @param magic Multiboot magic value provided by bootloader.
 * @param multiboot_info Physical address of multiboot_info_t.
 */
void kernel_main(uint32_t magic, uint32_t multiboot_info) {
  (void)magic;
  (void)multiboot_info;

  clear();

  hello_world();

  print("GDT: ");
  gdt_init();
  println("OK");

  print("IDT: ");
  idt_init();
  println("OK");

  print("PIC: ");
  pic_init();
  println("OK");

  print("PIT: ");
  pit_init();
  println("OK");

  print("PMM: ");
  pmm_init(multiboot_info, (uint32_t)&_kernel_start, (uint32_t)&_kernel_end);
  println("OK");

  print("PMM free frames: ");
  print_uint(pmm_get_free_frames());
  print("/");
  print_uint(pmm_get_total_frames());
  printc('\n');

  print("HEAP: ");
  heap_init();
  println("OK");

  print("PAGING: ");
  paging_prepare_identity(16U * 1024U * 1024U);
  if (paging_is_prepared() != 0U) {
    println("prepared");
    print("PAGING pages: ");
    print_uint(paging_get_mapped_pages());
    print(" dir=");
    print_uint(paging_get_directory_physical());
    printc('\n');
  } else {
    println("FAIL");
  }

#if PAGING_ENABLE_ON_BOOT
  paging_enable();
  println("PAGING enabled");
#else
  println("PAGING disabled");
#endif

  run_memory_smoke_tests();

  print("INITRD: ");
  initrd_init(multiboot_info);
  if (initrd_is_ready() != 0U) {
    println("OK");
    print("INITRD files: ");
    print_uint(initrd_file_count());
    printc('\n');
  } else {
    println("none");
  }

  run_vfs_smoke_test();
  run_elf_smoke_test();

  print("KBD: ");
  keyboard_init();
  println("OK");

  print("IRQ: ");
  enable_interrupts();
  println("OK");

  run_syscall_smoke_test();

  println("Ready! Type something:");

  while (1) {
  }
}