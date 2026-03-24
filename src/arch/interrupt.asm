; Macro do tworzenia handler'ów przerwań bez error code
%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    cli
    push 0
    push %1
    jmp isr_common_stub
%endmacro

; IRQ handlers
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push 0
    push %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32    ; Timer
IRQ 1, 33    ; Keyboard
IRQ 4, 36    ; Serial port
IRQ 12, 44   ; Mouse

extern isr_handler
extern irq_handler

isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    sti
    iret

irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call irq_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    sti
    iret

global idt_flush
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush_cs

flush_cs:
    ret

global enable_interrupts
enable_interrupts:
    sti
    ret

global disable_interrupts
disable_interrupts:
    cli
    ret
