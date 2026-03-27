#pragma once

#include <stdint.h>

/**
 * @file syscall.h
 * @brief System call dispatcher entry point for int 0x80 handling.
 * @ingroup proc_subsystem
 */

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
