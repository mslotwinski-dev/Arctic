#pragma once

#include <stdint.h>

/**
 * @file vfs.h
 * @brief Minimal virtual file API built over initrd data.
 * @ingroup fs_subsystem
 */

/**
 * @brief Opaque state for an opened VFS file handle.
 */
typedef struct {
  const uint8_t* data;
  uint32_t size;
  uint32_t position;
  uint8_t valid;
} vfs_file_t;

/**
 * @brief Seek origin constants used by vfs_seek.
 */
#define VFS_SEEK_SET 0U
#define VFS_SEEK_CUR 1U
#define VFS_SEEK_END 2U

/**
 * @brief Open a file path and create a read handle.
 * @param path File path/name.
 * @return File handle with valid=0 on failure.
 */
vfs_file_t vfs_open(const char* path);

/**
 * @brief Read bytes from an opened file and advance file position.
 * @param file Pointer to an opened file handle.
 * @param buffer Destination buffer.
 * @param size Maximum bytes to read.
 * @return Number of bytes actually read.
 */
uint32_t vfs_read(vfs_file_t* file, void* buffer, uint32_t size);

/**
 * @brief Reposition file cursor inside opened file.
 * @param file Pointer to an opened file handle.
 * @param offset Signed byte offset.
 * @param whence One of VFS_SEEK_SET, VFS_SEEK_CUR, VFS_SEEK_END.
 * @return New absolute file position, or (uint32_t)-1 on failure.
 */
uint32_t vfs_seek(vfs_file_t* file, int32_t offset, uint32_t whence);

/**
 * @brief Close file handle and invalidate it.
 * @param file Pointer to an opened file handle.
 */
void vfs_close(vfs_file_t* file);
