; Multiboot header - tells GRUB this is a valid kernel
section .multiboot
    align 4
    dd 0x1BADB002          ; Magic number that GRUB looks for to identify kernel
    dd 0x00000003          ; Flags (bit 0: load modules, bit 1: memory info)
    dd -(0x1BADB002 + 0x00000003) ; Checksum to validate header integrity

; Main code section - contains all executable instructions
section .text
    align 16                ; Align to 16-byte boundary for optimal performance
    global start            ; Make 'start' symbol visible to linker

; Uninitialized data section - contains stack space
section .bss
    align 16                ; Align stack to 16-byte boundary
stack_bottom:
    resb 4096              ; Reserve 4KB (4096 bytes) for stack space
stack_top:                  ; Top of stack (ESP will point here)

; Now all the actual code and data
section .text
start:
    ; Declare external symbols we'll use
    extern kernel_main      ; Main kernel function written in C
    extern gdt_load         ; Function to load Global Descriptor Table
    extern gdt_flush        ; Function to activate new GDT
    extern __bss_start      ; Symbol from linker script: start of .bss section
    extern __bss_end        ; Symbol from linker script: end of .bss section
    
    ; GRUB passes multiboot info in EBX register
    ; We need to save it before calling C functions
    push ebx                ; Save multiboot info pointer
    push eax                ; Save multiboot magic number
    
    ; ------------------------------------------------------
    ; Zero the .bss section (all uninitialized globals/statics)
    ; This is CRITICAL: without this, global variables would have random values!
    ; ------------------------------------------------------
    mov edi, __bss_start    ; EDI = destination pointer (start of .bss)
    mov ecx, __bss_end      ; ECX = end address of .bss
    sub ecx, __bss_start    ; ECX = size of .bss section in bytes
    shr ecx, 2              ; ECX = size in dwords (divide by 4, since we store 4 bytes at a time)
    xor eax, eax            ; EAX = 0 (we'll use this to zero memory)
    rep stosd               ; Repeat: store EAX (0) at [EDI], increment EDI, decrement ECX
    ; ------------------------------------------------------
    
    ; Set up stack - ESP points to top of stack (stacks grow downward), load GDT need to use stack
    mov esp, stack_top      ; Set stack pointer to top of our 4KB stack area
    
    ; Load and activate Global Descriptor Table (GDT)
    ; GDT defines memory segments and their permissions for protected mode
    call gdt_load           ; Load GDT into CPU's GDT register
    call gdt_flush          ; Flush CPU caches and activate new GDT
    
    ; Call the main kernel function written in C
    ; Pass multiboot magic and info pointer as parameters
    ; From this point on, we're running in the C kernel code
    call kernel_main
    
    ; If kernel_main ever returns (it shouldn't), disable interrupts and halt
    cli                     ; Clear interrupt flag (disable interrupts)

; Infinite loop - kernel should never reach here
hang:
    hlt                     ; Halt CPU (wait for interrupt)
    jmp hang               ; Jump back to halt (in case of spurious wake-up)