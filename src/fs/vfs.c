#include "vfs.h"

#include "initrd.h"

vfs_file_t vfs_open(const char* path) {
  vfs_file_t file;
  uint32_t size = 0;
  const void* data = initrd_get_file_data(path, &size);

  file.data = 0;
  file.size = 0;
  file.position = 0;
  file.valid = 0;

  if (data != 0) {
    file.data = (const uint8_t*)data;
    file.size = size;
    file.valid = 1;
  }

  return file;
}

uint32_t vfs_read(vfs_file_t* file, void* buffer, uint32_t size) {
  uint8_t* dst = (uint8_t*)buffer;
  uint32_t remaining;
  uint32_t to_read;

  if (file == 0 || buffer == 0 || file->valid == 0 || size == 0U) {
    return 0;
  }

  if (file->position >= file->size) {
    return 0;
  }

  remaining = file->size - file->position;
  to_read = size < remaining ? size : remaining;

  for (uint32_t i = 0; i < to_read; i++) {
    dst[i] = file->data[file->position + i];
  }

  file->position += to_read;
  return to_read;
}

uint32_t vfs_seek(vfs_file_t* file, int32_t offset, uint32_t whence) {
  int64_t base;
  int64_t target;

  if (file == 0 || file->valid == 0U) {
    return (uint32_t)-1;
  }

  switch (whence) {
    case VFS_SEEK_SET:
      base = 0;
      break;
    case VFS_SEEK_CUR:
      base = (int64_t)file->position;
      break;
    case VFS_SEEK_END:
      base = (int64_t)file->size;
      break;
    default:
      return (uint32_t)-1;
  }

  target = base + (int64_t)offset;
  if (target < 0 || target > (int64_t)file->size) {
    return (uint32_t)-1;
  }

  file->position = (uint32_t)target;
  return file->position;
}

void vfs_close(vfs_file_t* file) {
  if (file == 0) {
    return;
  }

  file->data = 0;
  file->size = 0;
  file->position = 0;
  file->valid = 0;
}
