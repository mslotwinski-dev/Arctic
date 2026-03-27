#pragma once

#include <stdint.h>

/**
 * @file syscall.h
 * @brief System call dispatcher entry point for int 0x80 handling.
 * @ingroup proc_subsystem
 */

/** @brief Terminate current execution context. */
#define SYS_EXIT 0U
/** @brief Write bytes to output descriptor. */
#define SYS_WRITE 1U
/** @brief Read bytes from input descriptor. */
#define SYS_READ 2U
/** @brief Open VFS path and return descriptor. */
#define SYS_OPEN 3U
/** @brief Close descriptor. */
#define SYS_CLOSE 4U
/** @brief Seek inside file descriptor. */
#define SYS_SEEK 5U
/** @brief Return number of initrd files. */
#define SYS_FILE_COUNT 6U
/** @brief Copy initrd file name by index to caller buffer. */
#define SYS_FILE_NAME 7U
/** @brief Return initrd file size by index. */
#define SYS_FILE_SIZE 8U
/** @brief Return initrd file size by path/name. */
#define SYS_FILE_SIZE_BY_NAME 9U
/** @brief Return count of files matching prefix. */
#define SYS_FILE_COUNT_PREFIX 10U
/** @brief Copy prefix-filtered file name by filtered index. */
#define SYS_FILE_NAME_PREFIX 11U

/** @brief Seek from beginning of file. */
#define SYS_SEEK_SET 0U
/** @brief Seek from current file position. */
#define SYS_SEEK_CUR 1U
/** @brief Seek from end of file. */
#define SYS_SEEK_END 2U

/**
 * @brief Dispatch one system call by number and register-style arguments.
 *
 * Parameter order follows the register ABI used by the interrupt handler.
 *
 * @param number Syscall number (usually EAX).
 * @param arg1 First syscall argument.
 * @param arg2 Second syscall argument.
 * @param arg3 Third syscall argument.
 * @param arg4 Fourth syscall argument.
 * @param arg5 Fifth syscall argument.
 * @return Syscall result value (written back to caller EAX).
 */
uint32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
