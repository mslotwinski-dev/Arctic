#include "initrd.h"

#include "../memory/heap.h"

typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms0;
  uint32_t syms1;
  uint32_t syms2;
  uint32_t syms3;
  uint32_t mmap_length;
  uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
  uint32_t mod_start;
  uint32_t mod_end;
  uint32_t cmdline;
  uint32_t pad;
} __attribute__((packed)) multiboot_module_t;

/**
 * @brief Internal initrd index entry for one discovered TAR member.
 */
typedef struct {
  char name[100];
  uint32_t offset;
  uint32_t size;
} initrd_file_t;

typedef struct {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char padding[12];
} __attribute__((packed)) tar_header_t;

static uint8_t initrd_ready = 0;
static const uint8_t* initrd_base = 0;
static uint32_t initrd_size = 0;
static initrd_file_t* initrd_files = 0;
static uint32_t initrd_files_count = 0;

static uint32_t octal_to_u32(const char* str, uint32_t len) {
  uint32_t value = 0;

  for (uint32_t i = 0; i < len; i++) {
    char c = str[i];
    if (c == '\0' || c == ' ') {
      break;
    }
    if (c < '0' || c > '7') {
      break;
    }
    value = (value << 3) + (uint32_t)(c - '0');
  }

  return value;
}

static uint32_t cstr_len(const char* str) {
  uint32_t len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

static int cstr_eq(const char* a, const char* b) {
  uint32_t i = 0;
  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return 0;
    }
    i++;
  }
  return a[i] == b[i];
}

static int tar_header_is_empty(const tar_header_t* header) {
  const uint8_t* raw = (const uint8_t*)header;
  for (uint32_t i = 0; i < 512; i++) {
    if (raw[i] != 0U) {
      return 0;
    }
  }
  return 1;
}

void initrd_init(uint32_t multiboot_info_addr) {
  multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info_addr;
  uint32_t offset = 0;
  uint32_t count = 0;
  uint32_t index = 0;

  initrd_ready = 0;
  initrd_base = 0;
  initrd_size = 0;
  initrd_files = 0;
  initrd_files_count = 0;

  if ((mbi->flags & (1U << 3)) == 0U || mbi->mods_count == 0U) {
    return;
  }

  {
    multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
    initrd_base = (const uint8_t*)mods[0].mod_start;
    initrd_size = mods[0].mod_end - mods[0].mod_start;
  }

  while (offset + 512U <= initrd_size) {
    const tar_header_t* header = (const tar_header_t*)(initrd_base + offset);
    uint32_t file_size;
    uint32_t jump;

    if (tar_header_is_empty(header)) {
      break;
    }

    file_size = octal_to_u32(header->size, 12);
    if (header->typeflag == '0' || header->typeflag == '\0') {
      count++;
    }

    jump = 512U + ((file_size + 511U) & ~511U);
    if (jump == 0U) {
      break;
    }
    offset += jump;
  }

  if (count == 0U) {
    initrd_ready = 1;
    return;
  }

  initrd_files = (initrd_file_t*)kcalloc(count, (uint32_t)sizeof(initrd_file_t));
  if (initrd_files == 0) {
    return;
  }

  offset = 0;
  while (offset + 512U <= initrd_size && index < count) {
    const tar_header_t* header = (const tar_header_t*)(initrd_base + offset);
    uint32_t file_size;
    uint32_t payload_offset;
    uint32_t jump;

    if (tar_header_is_empty(header)) {
      break;
    }

    file_size = octal_to_u32(header->size, 12);
    payload_offset = offset + 512U;

    if ((header->typeflag == '0' || header->typeflag == '\0') && payload_offset + file_size <= initrd_size) {
      uint32_t name_len = cstr_len(header->name);
      if (name_len >= sizeof(initrd_files[index].name)) {
        name_len = (uint32_t)sizeof(initrd_files[index].name) - 1U;
      }

      for (uint32_t i = 0; i < name_len; i++) {
        initrd_files[index].name[i] = header->name[i];
      }
      initrd_files[index].name[name_len] = '\0';
      initrd_files[index].offset = payload_offset;
      initrd_files[index].size = file_size;
      index++;
    }

    jump = 512U + ((file_size + 511U) & ~511U);
    if (jump == 0U) {
      break;
    }
    offset += jump;
  }

  initrd_files_count = index;
  initrd_ready = 1;
}

uint8_t initrd_is_ready(void) { return initrd_ready; }

uint32_t initrd_file_count(void) { return initrd_files_count; }

const void* initrd_get_file_data(const char* name, uint32_t* size) {
  if (initrd_ready == 0 || initrd_files == 0 || name == 0) {
    return 0;
  }

  for (uint32_t i = 0; i < initrd_files_count; i++) {
    if (cstr_eq(initrd_files[i].name, name)) {
      if (size != 0) {
        *size = initrd_files[i].size;
      }
      return (const void*)(initrd_base + initrd_files[i].offset);
    }
  }

  return 0;
}

const char* initrd_get_file_name(uint32_t index) {
  if (initrd_ready == 0 || initrd_files == 0 || index >= initrd_files_count) {
    return 0;
  }

  return initrd_files[index].name;
}
