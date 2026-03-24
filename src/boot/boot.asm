MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384 
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    
    ; Test: write 'A' directly to VGA memory at 0xB8000
    mov eax, 0xB8000
    mov byte [eax], 'A'        ; Character
    mov byte [eax + 1], 0x0F   ; Color (white on black)
    
    call kernel_main  
    cli
.hang:
    hlt
    jmp .hang