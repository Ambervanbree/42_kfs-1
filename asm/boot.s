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
    extern gdt_load
    extern gdt_flush
    extern __bss_start      ; Symbol from linker script: start of .bss
    extern __bss_end        ; Symbol from linker script: end of .bss
    
    ; ------------------------------------------------------
    ; Zero the .bss section (all uninitialized globals/statics)
    ; ------------------------------------------------------
    mov edi, __bss_start    ; Destination pointer (start of .bss)
    mov ecx, __bss_end      ; End of .bss
    sub ecx, __bss_start    ; ecx = size in bytes
    shr ecx, 2              ; ecx = size in dwords (divide by 4)
    xor eax, eax            ; Zero value
    rep stosd               ; Fill ECX dwords at [EDI] with EAX (zero)
    ; ------------------------------------------------------
    
    ; Set up stack
    mov esp, stack_top
    
    ; Load GDT
    call gdt_load
    call gdt_flush
    
    ; Call kernel main
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