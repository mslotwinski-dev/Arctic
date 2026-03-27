#include "syscall.h"

#include "../drivers/keyboard.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../io/print.h"

/**
 * @file syscall.c
 * @brief Minimal syscall table and dispatcher implementation.
 * @ingroup proc_subsystem
 */

/**
 * @brief Internal fixed-size file descriptor table entry.
 */
typedef struct {
  vfs_file_t file;
  uint8_t used;
} fd_entry_t;

static fd_entry_t fd_table[8];

static uint32_t copy_cstr_to_user(char* dst, uint32_t dst_len, const char* src) {
  uint32_t i = 0;

  if (dst == 0 || src == 0 || dst_len == 0U) {
    return (uint32_t)-1;
  }

  while (i + 1U < dst_len && src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }

  dst[i] = '\0';
  return i;
}

static uint32_t sys_write(uint32_t fd, const char* buffer, uint32_t len) {
  if (fd != 1U || buffer == 0) {
    return 0;
  }

  for (uint32_t i = 0; i < len; i++) {
    printc(buffer[i]);
  }

  return len;
}

static uint32_t sys_exit(uint32_t code) {
  (void)code;
  println("\n[sys_exit]");
  return 0;
}

static uint32_t sys_open(const char* path) {
  vfs_file_t file;

  if (path == 0) {
    return (uint32_t)-1;
  }

  file = vfs_open(path);
  if (file.valid == 0U) {
    return (uint32_t)-1;
  }

  for (uint32_t i = 0; i < (uint32_t)(sizeof(fd_table) / sizeof(fd_table[0])); i++) {
    if (fd_table[i].used == 0U) {
      fd_table[i].used = 1;
      fd_table[i].file = file;
      return 3U + i;
    }
  }

  return (uint32_t)-1;
}

static uint32_t sys_read(uint32_t fd, char* buffer, uint32_t len) {
  uint32_t read_count = 0;

  if (buffer == 0 || len == 0U) {
    return 0;
  }

  if (fd == 0U) {
    while (read_count < len) {
      char c;
      if (keyboard_pop_char(&c) == 0) {
        break;
      }
      buffer[read_count++] = c;
    }
    return read_count;
  }

  if (fd < 3U) {
    return (uint32_t)-1;
  }

  {
    uint32_t idx = fd - 3U;
    if (idx >= (uint32_t)(sizeof(fd_table) / sizeof(fd_table[0])) || fd_table[idx].used == 0U) {
      return (uint32_t)-1;
    }

    return vfs_read(&fd_table[idx].file, buffer, len);
  }
}

static uint32_t sys_close(uint32_t fd) {
  uint32_t idx;

  if (fd < 3U) {
    return (uint32_t)-1;
  }

  idx = fd - 3U;
  if (idx >= (uint32_t)(sizeof(fd_table) / sizeof(fd_table[0])) || fd_table[idx].used == 0U) {
    return (uint32_t)-1;
  }

  vfs_close(&fd_table[idx].file);
  fd_table[idx].used = 0;
  return 0;
}

static uint32_t sys_seek(uint32_t fd, int32_t offset, uint32_t whence) {
  uint32_t idx;

  if (fd < 3U) {
    return (uint32_t)-1;
  }

  idx = fd - 3U;
  if (idx >= (uint32_t)(sizeof(fd_table) / sizeof(fd_table[0])) || fd_table[idx].used == 0U) {
    return (uint32_t)-1;
  }

  return vfs_seek(&fd_table[idx].file, offset, whence);
}

static uint32_t sys_file_count(void) {
  if (initrd_is_ready() == 0U) {
    return 0;
  }

  return initrd_file_count();
}

static uint32_t sys_file_name(uint32_t index, char* buffer, uint32_t buffer_len) {
  const char* name;

  if (initrd_is_ready() == 0U) {
    return (uint32_t)-1;
  }

  name = initrd_get_file_name(index);
  if (name == 0) {
    return (uint32_t)-1;
  }

  return copy_cstr_to_user(buffer, buffer_len, name);
}

static uint32_t sys_file_size(uint32_t index) {
  uint32_t size;

  if (initrd_get_file_size(index, &size) == 0U) {
    return (uint32_t)-1;
  }

  return size;
}

static uint32_t sys_file_size_by_name(const char* name) {
  uint32_t size;

  if (initrd_get_file_size_by_name(name, &size) == 0U) {
    return (uint32_t)-1;
  }

  return size;
}

static uint32_t sys_file_count_prefix(const char* prefix) { return initrd_count_prefix(prefix); }

static uint32_t sys_file_name_prefix(const char* prefix, uint32_t index, char* buffer, uint32_t buffer_len) {
  const char* name = initrd_get_file_name_by_prefix(prefix, index);

  if (name == 0) {
    return (uint32_t)-1;
  }

  return copy_cstr_to_user(buffer, buffer_len, name);
}

uint32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
  (void)arg4;
  (void)arg5;

  switch (number) {
    case SYS_EXIT: return sys_exit(arg1);
    case SYS_WRITE: return sys_write(arg1, (const char*)arg2, arg3);
    case SYS_READ: return sys_read(arg1, (char*)arg2, arg3);
    case SYS_OPEN: return sys_open((const char*)arg1);
    case SYS_CLOSE: return sys_close(arg1);
    case SYS_SEEK: return sys_seek(arg1, (int32_t)arg2, arg3);
    case SYS_FILE_COUNT: return sys_file_count();
    case SYS_FILE_NAME: return sys_file_name(arg1, (char*)arg2, arg3);
    case SYS_FILE_SIZE: return sys_file_size(arg1);
    case SYS_FILE_SIZE_BY_NAME: return sys_file_size_by_name((const char*)arg1);
    case SYS_FILE_COUNT_PREFIX: return sys_file_count_prefix((const char*)arg1);
    case SYS_FILE_NAME_PREFIX: return sys_file_name_prefix((const char*)arg1, arg2, (char*)arg3, arg4);
    default: return (uint32_t)-1;
  }
}
