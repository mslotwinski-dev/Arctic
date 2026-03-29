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
#include "../proc/syscall.h"

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

static uint32_t syscall0(uint32_t number) {
  uint32_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(number) : "memory");
  return ret;
}

static uint32_t syscall1(uint32_t number, uint32_t arg1) {
  uint32_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(number), "b"(arg1) : "memory");
  return ret;
}

static uint32_t syscall3(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
  uint32_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3) : "memory");
  return ret;
}

static uint32_t syscall4(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
  uint32_t ret;
  __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4) : "memory");
  return ret;
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

  __asm__ __volatile__("int $0x80"
                       : "=a"(written)
                       : "a"(SYS_WRITE), "b"(1U), "c"(msg), "d"((uint32_t)(sizeof(msg) - 1U))
                       : "memory");

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
    uint32_t files_total;
    uint32_t files_pref;
    uint32_t name_len;
    uint32_t pref_name_len;
    uint32_t file_size;
    uint32_t file_size_by_name;
    uint32_t same = 1U;
    char name_buf[64];
    char pref_buf[64];
    static const char prefix_all[] = "";

    if (name == 0) {
      return;
    }

    __asm__ __volatile__("int $0x80" : "=a"(fd) : "a"(SYS_OPEN), "b"((uint32_t)name) : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(read_a)
                         : "a"(SYS_READ), "b"(fd), "c"((uint32_t)buffer_a), "d"(8U)
                         : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(seek_ret) : "a"(SYS_SEEK), "b"(fd), "c"(0U), "d"(SYS_SEEK_SET) : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(read_b)
                         : "a"(SYS_READ), "b"(fd), "c"((uint32_t)buffer_b), "d"(8U)
                         : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(close_ret) : "a"(SYS_CLOSE), "b"(fd) : "memory");

    __asm__ __volatile__("int $0x80" : "=a"(files_total) : "a"(SYS_FILE_COUNT) : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(name_len)
                         : "a"(SYS_FILE_NAME), "b"(0U), "c"((uint32_t)name_buf), "d"((uint32_t)sizeof(name_buf))
                         : "memory");
    __asm__ __volatile__("int $0x80" : "=a"(file_size) : "a"(SYS_FILE_SIZE), "b"(0U) : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(file_size_by_name)
                         : "a"(SYS_FILE_SIZE_BY_NAME), "b"((uint32_t)name)
                         : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(files_pref)
                         : "a"(SYS_FILE_COUNT_PREFIX), "b"((uint32_t)prefix_all)
                         : "memory");
    __asm__ __volatile__("int $0x80"
                         : "=a"(pref_name_len)
                         : "a"(SYS_FILE_NAME_PREFIX),
                           "b"((uint32_t)prefix_all),
                           "c"(0U),
                           "d"((uint32_t)pref_buf),
                           "S"((uint32_t)sizeof(pref_buf))
                         : "memory");

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
    print(" files=");
    print_uint(files_total);
    print(" first_name_len=");
    print_uint(name_len);
    print(" first_size=");
    print_uint(file_size);
    print(" first_size_by_name=");
    print_uint(file_size_by_name);
    print(" pref_count=");
    print_uint(files_pref);
    print(" pref_name_len=");
    print_uint(pref_name_len);
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

static void run_fs_listing_demo(void) {
  static const char prefix_all[] = "";
  static const char prefix_bin[] = "bin/";
  static const char prefix_etc[] = "etc/";
  uint32_t total;
  uint32_t max_items;
  uint32_t bin_count;
  uint32_t etc_count;

  if (initrd_is_ready() == 0U) {
    println("LS demo: SKIP");
    return;
  }

  total = syscall0(SYS_FILE_COUNT);
  max_items = total < 6U ? total : 6U;

  print("LS demo total=");
  print_uint(total);
  print(" showing=");
  print_uint(max_items);
  printc('\n');

  for (uint32_t i = 0; i < max_items; i++) {
    char name[64];
    uint32_t name_len = syscall3(SYS_FILE_NAME, i, (uint32_t)name, (uint32_t)sizeof(name));
    uint32_t file_size = syscall1(SYS_FILE_SIZE, i);

    print(" - ");
    print_uint(i);
    print(": ");
    if (name_len == (uint32_t)-1) {
      print("<invalid>");
    } else {
      print(name);
    }
    print(" (");
    print_uint(file_size);
    print(")");
    printc('\n');
  }

  bin_count = syscall1(SYS_FILE_COUNT_PREFIX, (uint32_t)prefix_bin);
  etc_count = syscall1(SYS_FILE_COUNT_PREFIX, (uint32_t)prefix_etc);

  print("LS demo prefix bin/=");
  print_uint(bin_count);
  print(" etc/=");
  print_uint(etc_count);
  printc('\n');

  if (total > 0U) {
    char first_any[64];
    uint32_t any_len =
        syscall4(SYS_FILE_NAME_PREFIX, (uint32_t)prefix_all, 0U, (uint32_t)first_any, (uint32_t)sizeof(first_any));

    print("LS demo first(any)=");
    if (any_len == (uint32_t)-1) {
      print("<invalid>");
    } else {
      print(first_any);
    }
    printc('\n');
  }
}

static int cstr_eq_local(const char* a, const char* b) {
  uint32_t i = 0;

  if (a == 0 || b == 0) {
    return 0;
  }

  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return 0;
    }
    i++;
  }

  return a[i] == b[i];
}

static int cstr_starts_with_local(const char* str, const char* prefix) {
  uint32_t i = 0;

  if (str == 0 || prefix == 0) {
    return 0;
  }

  while (prefix[i] != '\0') {
    if (str[i] == '\0' || str[i] != prefix[i]) {
      return 0;
    }
    i++;
  }

  return 1;
}

static const char* skip_spaces(const char* str) {
  const char* p = str;
  while (p != 0 && (*p == ' ' || *p == '\t')) {
    p++;
  }
  return p;
}

/* Global current drive (disk A) */
static char current_drive = 'A';
static uint32_t prompt_len = 0;

/**
 * @brief Shell input buffer tracking for proper cursor positioning
 * (In VGA text mode, cursor is automatically displayed at current write position)
 */

static void print_prompt(void) {
  printc(current_drive);
  print(":> ");
  prompt_len = 3; /* drive letter, colon, >, space = 3 chars */
}

static void cmd_ls(const char* prefix) {
  uint32_t count = syscall1(SYS_FILE_COUNT_PREFIX, (uint32_t)prefix);
  uint32_t show = count < 32U ? count : 32U;

  print("ls count=");
  print_uint(count);
  printc('\n');

  for (uint32_t i = 0; i < show; i++) {
    char name[96];
    uint32_t len = syscall4(SYS_FILE_NAME_PREFIX, (uint32_t)prefix, i, (uint32_t)name, (uint32_t)sizeof(name));

    if (len == (uint32_t)-1) {
      continue;
    }

    print(" - ");
    print(name);
    print(" (");
    print_uint(syscall1(SYS_FILE_SIZE_BY_NAME, (uint32_t)name));
    print(")");
    printc('\n');
  }

  if (count > show) {
    print("... and ");
    print_uint(count - show);
    println(" more");
  }
}

static void cmd_cat(const char* path) {
  uint32_t fd;
  char buf[64];

  if (path == 0 || path[0] == '\0') {
    println("cat: missing path");
    return;
  }

  fd = syscall1(SYS_OPEN, (uint32_t)path);
  if (fd == (uint32_t)-1) {
    println("cat: open failed");
    return;
  }

  while (1) {
    uint32_t read_count = syscall3(SYS_READ, fd, (uint32_t)buf, (uint32_t)sizeof(buf));

    if (read_count == (uint32_t)-1 || read_count == 0U) {
      break;
    }

    (void)syscall3(SYS_WRITE, 1U, (uint32_t)buf, read_count);
  }

  (void)syscall1(SYS_CLOSE, fd);
  printc('\n');
}

static void cmd_cd(const char* arg) {
  if (arg == 0 || arg[0] == '\0') {
    print("Current drive: ");
    printc(current_drive);
    println(":");
    return;
  }

  /* Simple drive switching: A:, B:, C:, etc. */
  if (arg[1] == ':' && arg[2] == '\0' && arg[0] >= 'A' && arg[0] <= 'Z') {
    current_drive = arg[0];
    print("Switched to drive: ");
    printc(current_drive);
    println(":");
  } else if (arg[0] >= 'a' && arg[0] <= 'z' && arg[1] == ':' && arg[2] == '\0') {
    current_drive = (char)(arg[0] - 32); /* Convert to uppercase */
    print("Switched to drive: ");
    printc(current_drive);
    println(":");
  } else {
    println("cd: only single drive letter supported (e.g., A:, B:, C:)");
  }
}

static void run_builtin_shell(void) {
  char line[128];
  uint32_t line_len = 0;

  println("Shell: help, ls [prefix], cat <path>, cd <drive>, clear");
  print_prompt();

  while (1) {
    char c;

    if (keyboard_pop_char(&c) == 0) {
      continue;
    }

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      const char* cmd;

      line[line_len] = '\0';
      cmd = skip_spaces(line);

      if (cmd[0] == '\0') {
        print_prompt();
        line_len = 0;
        continue;
      }

      if (cstr_eq_local(cmd, "help") != 0) {
        println("help: show commands");
        println("ls [prefix]: list initrd files (optional prefix)");
        println("cat <path>: print file content");
        println("cd <drive>: change disk (e.g., A:, B:, C:)");
        println("clear: clear screen");
      } else if (cstr_eq_local(cmd, "clear") != 0) {
        clear();
      } else if (cstr_starts_with_local(cmd, "ls") != 0 && (cmd[2] == '\0' || cmd[2] == ' ' || cmd[2] == '\t')) {
        const char* arg = skip_spaces(cmd + 2);
        cmd_ls(arg);
      } else if (cstr_starts_with_local(cmd, "cat") != 0 && (cmd[3] == ' ' || cmd[3] == '\t')) {
        const char* arg = skip_spaces(cmd + 3);
        cmd_cat(arg);
      } else if (cstr_starts_with_local(cmd, "cd") != 0 && (cmd[2] == ' ' || cmd[2] == '\t' || cmd[2] == '\0')) {
        const char* arg = skip_spaces(cmd + 2);
        cmd_cd(arg);
      } else {
        print("unknown command: ");
        print(cmd);
        printc('\n');
      }

      line_len = 0;
      print_prompt();
      continue;
    }

    if (c == '\b') {
      /* Backspace: erase last character only if text exists (not the prompt) */
      if (line_len > 0U) {
        line_len--;
        /* printc('\b') handles cursor movement and character erasure automatically */
        printc('\b');
      }
      /* Prompt itself (A:>) cannot be erased */
      continue;
    }

    if (c >= 32 && c <= 126 && line_len + 1U < (uint32_t)sizeof(line)) {
      line[line_len++] = c;
      /* Echo visible character to terminal */
      printc(c);
    }
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
  run_fs_listing_demo();

  println("Ready! Type something:");
  run_builtin_shell();
}