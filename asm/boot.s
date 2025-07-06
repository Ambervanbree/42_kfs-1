section .note.GNU-stack noalloc noexec nowrite progbits

section .multiboot
    align 4
    dd 0x1BADB002          ; magic number
    dd 0x0                 ; flags
    dd -(0x1BADB002 + 0x0) ; checksum

section .text
    align 16
    global start

start:
    extern kernel_main
    mov esp, stack_top
    call kernel_main
    cli

hang:
    hlt
    jmp hang

section .bss
    align 16
stack_bottom:
    resb 4096         ; 4 KB stack
stack_top: