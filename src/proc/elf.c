#include "elf.h"

#include "../fs/vfs.h"
#include "../memory/heap.h"
#include "../memory/paging.h"

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} __attribute__((packed)) elf32_phdr_t;

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define EM_386 3
#define ET_EXEC 2
#define ET_DYN 3
#define PT_LOAD 1

static uint8_t elf_header_basic_valid(const elf32_ehdr_t *hdr, uint32_t size) {
  uint32_t min_ph_table;

  if (size < sizeof(elf32_ehdr_t)) {
    return 0;
  }

  if (hdr->e_ident[0] != ELF_MAGIC0 || hdr->e_ident[1] != ELF_MAGIC1 || hdr->e_ident[2] != ELF_MAGIC2 || hdr->e_ident[3] != ELF_MAGIC3) {
    return 0;
  }

  if (hdr->e_ident[4] != ELFCLASS32 || hdr->e_ident[5] != ELFDATA2LSB) {
    return 0;
  }

  if (hdr->e_ident[6] != EV_CURRENT || hdr->e_version != EV_CURRENT) {
    return 0;
  }

  if (hdr->e_machine != EM_386) {
    return 0;
  }

  if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
    return 0;
  }

  if (hdr->e_phnum == 0U) {
    return 0;
  }

  if (hdr->e_phentsize != sizeof(elf32_phdr_t)) {
    return 0;
  }

  min_ph_table = hdr->e_phoff + (uint32_t)hdr->e_phnum * (uint32_t)hdr->e_phentsize;
  if (min_ph_table > size || min_ph_table < hdr->e_phoff) {
    return 0;
  }

  return 1;
}

uint8_t elf_validate_buffer(const void *data, uint32_t size) {
  const elf32_ehdr_t *hdr = (const elf32_ehdr_t *)data;
  return elf_header_basic_valid(hdr, size);
}

uint32_t elf_get_entry(const void *data) {
  const elf32_ehdr_t *hdr = (const elf32_ehdr_t *)data;
  return hdr->e_entry;
}

uint32_t elf_count_loadable_segments(const void *data) {
  const elf32_ehdr_t *hdr = (const elf32_ehdr_t *)data;
  const elf32_phdr_t *phdr;
  uint32_t count = 0;

  phdr = (const elf32_phdr_t *)((const uint8_t *)data + hdr->e_phoff);
  for (uint32_t i = 0; i < hdr->e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      count++;
    }
  }

  return count;
}

uint8_t elf_probe_from_vfs(const char *path, uint32_t *entry, uint32_t *load_segments) {
  vfs_file_t file;
  const uint8_t *all_data;

  if (path == 0) {
    return 0;
  }

  file = vfs_open(path);
  if (file.valid == 0U) {
    return 0;
  }

  all_data = file.data;
  if (all_data == 0 || elf_validate_buffer(all_data, file.size) == 0U) {
    vfs_close(&file);
    return 0;
  }

  if (entry != 0) {
    *entry = elf_get_entry(all_data);
  }

  if (load_segments != 0) {
    *load_segments = elf_count_loadable_segments(all_data);
  }

  vfs_close(&file);
  return 1;
}

uint8_t elf_load_from_vfs(const char *path, uint32_t *entry, uint32_t *load_segments, uint32_t *loaded_bytes) {
  vfs_file_t file;
  const uint8_t *all_data;
  const elf32_ehdr_t *hdr;
  const elf32_phdr_t *phdr;
  uint32_t segments = 0;
  uint32_t bytes = 0;

  if (path == 0) {
    return 0;
  }

  file = vfs_open(path);
  if (file.valid == 0U) {
    return 0;
  }

  all_data = file.data;
  if (all_data == 0 || elf_validate_buffer(all_data, file.size) == 0U) {
    vfs_close(&file);
    return 0;
  }

  hdr = (const elf32_ehdr_t *)all_data;
  phdr = (const elf32_phdr_t *)(all_data + hdr->e_phoff);

  for (uint32_t i = 0; i < hdr->e_phnum; i++) {
    const elf32_phdr_t *segment = &phdr[i];
    uint8_t *dst;

    if (segment->p_type != PT_LOAD) {
      continue;
    }

    if (segment->p_memsz == 0U) {
      continue;
    }

    if (segment->p_offset + segment->p_filesz > file.size || segment->p_filesz > segment->p_memsz) {
      vfs_close(&file);
      return 0;
    }

    dst = (uint8_t *)kmalloc(segment->p_memsz);
    if (dst == 0) {
      vfs_close(&file);
      return 0;
    }

    for (uint32_t j = 0; j < segment->p_filesz; j++) {
      dst[j] = all_data[segment->p_offset + j];
    }

    for (uint32_t j = segment->p_filesz; j < segment->p_memsz; j++) {
      dst[j] = 0;
    }

    segments++;
    bytes += segment->p_memsz;
  }

  if (entry != 0) {
    *entry = hdr->e_entry;
  }

  if (load_segments != 0) {
    *load_segments = segments;
  }

  if (loaded_bytes != 0) {
    *loaded_bytes = bytes;
  }

  vfs_close(&file);
  return 1;
}

uint8_t elf_load_image_from_vfs(const char *path, uint32_t *entry_virtual, uint32_t *entry_loaded, uint32_t *load_segments,
                                uint32_t *loaded_bytes) {
  vfs_file_t file;
  const uint8_t *all_data;
  const elf32_ehdr_t *hdr;
  const elf32_phdr_t *phdr;
  uint32_t segments = 0;
  uint32_t bytes = 0;
  uint32_t loaded_entry = 0;

  if (path == 0) {
    return 0;
  }

  file = vfs_open(path);
  if (file.valid == 0U) {
    return 0;
  }

  all_data = file.data;
  if (all_data == 0 || elf_validate_buffer(all_data, file.size) == 0U) {
    vfs_close(&file);
    return 0;
  }

  hdr = (const elf32_ehdr_t *)all_data;
  phdr = (const elf32_phdr_t *)(all_data + hdr->e_phoff);

  for (uint32_t i = 0; i < hdr->e_phnum; i++) {
    const elf32_phdr_t *segment = &phdr[i];
    uint8_t *dst;

    if (segment->p_type != PT_LOAD) {
      continue;
    }

    if (segment->p_memsz == 0U) {
      continue;
    }

    if (segment->p_offset + segment->p_filesz > file.size || segment->p_filesz > segment->p_memsz) {
      vfs_close(&file);
      return 0;
    }

    dst = (uint8_t *)kmalloc(segment->p_memsz);
    if (dst == 0) {
      vfs_close(&file);
      return 0;
    }

    for (uint32_t j = 0; j < segment->p_filesz; j++) {
      dst[j] = all_data[segment->p_offset + j];
    }

    for (uint32_t j = segment->p_filesz; j < segment->p_memsz; j++) {
      dst[j] = 0;
    }

    if (loaded_entry == 0U && hdr->e_entry >= segment->p_vaddr && hdr->e_entry < segment->p_vaddr + segment->p_memsz) {
      loaded_entry = (uint32_t)(dst + (hdr->e_entry - segment->p_vaddr));
    }

    segments++;
    bytes += segment->p_memsz;
  }

  if (entry_virtual != 0) {
    *entry_virtual = hdr->e_entry;
  }

  if (entry_loaded != 0) {
    *entry_loaded = loaded_entry;
  }

  if (load_segments != 0) {
    *load_segments = segments;
  }

  if (loaded_bytes != 0) {
    *loaded_bytes = bytes;
  }

  vfs_close(&file);
  return segments > 0U;
}

uint8_t elf_try_execute_loaded_entry(uint32_t loaded_entry) {
  void (*entry_fn)(void);

  if (loaded_entry == 0U) {
    return 0;
  }

  entry_fn = (void (*)(void))loaded_entry;
  entry_fn();
  return 1;
}

uint8_t elf_load_virtual_identity_from_vfs(const char *path, uint32_t *entry_virtual, uint32_t *load_segments,
                                           uint32_t *loaded_bytes) {
  vfs_file_t file;
  const uint8_t *all_data;
  const elf32_ehdr_t *hdr;
  const elf32_phdr_t *phdr;
  uint32_t identity_limit;
  uint32_t segments = 0;
  uint32_t bytes = 0;

  if (path == 0 || paging_is_prepared() == 0U) {
    return 0;
  }

  identity_limit = paging_get_identity_limit_bytes();
  if (identity_limit == 0U) {
    return 0;
  }

  file = vfs_open(path);
  if (file.valid == 0U) {
    return 0;
  }

  all_data = file.data;
  if (all_data == 0 || elf_validate_buffer(all_data, file.size) == 0U) {
    vfs_close(&file);
    return 0;
  }

  hdr = (const elf32_ehdr_t *)all_data;
  phdr = (const elf32_phdr_t *)(all_data + hdr->e_phoff);

  for (uint32_t i = 0; i < hdr->e_phnum; i++) {
    const elf32_phdr_t *segment = &phdr[i];
    uint8_t *dst;

    if (segment->p_type != PT_LOAD || segment->p_memsz == 0U) {
      continue;
    }

    if (segment->p_offset + segment->p_filesz > file.size || segment->p_filesz > segment->p_memsz) {
      vfs_close(&file);
      return 0;
    }

    if (segment->p_vaddr < 0x00100000U || segment->p_vaddr + segment->p_memsz < segment->p_vaddr ||
        segment->p_vaddr + segment->p_memsz > identity_limit) {
      vfs_close(&file);
      return 0;
    }

    dst = (uint8_t *)segment->p_vaddr;
    for (uint32_t j = 0; j < segment->p_filesz; j++) {
      dst[j] = all_data[segment->p_offset + j];
    }

    for (uint32_t j = segment->p_filesz; j < segment->p_memsz; j++) {
      dst[j] = 0;
    }

    segments++;
    bytes += segment->p_memsz;
  }

  if (entry_virtual != 0) {
    *entry_virtual = hdr->e_entry;
  }

  if (load_segments != 0) {
    *load_segments = segments;
  }

  if (loaded_bytes != 0) {
    *loaded_bytes = bytes;
  }

  vfs_close(&file);
  return segments > 0U;
}
