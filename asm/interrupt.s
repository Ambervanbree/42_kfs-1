section .text
    global idt_load
    global keyboard_handler_asm
    global page_fault_handler_asm

; Load the IDT
idt_load:
    extern idtp
    lidt [idtp]
    ret

; Keyboard interrupt handler wrapper
keyboard_handler_asm:
    ; Save registers (don't push esp as it can cause issues)
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    
    ; Call C handler
    extern keyboard_handler
    call keyboard_handler
    
    ; Restore registers
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    
    ; Return from interrupt
    iret

; Page fault handler wrapper
page_fault_handler_asm:
    ; Save registers
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    
    ; Call C handler
    extern page_fault_handler
    call page_fault_handler
    
    ; Restore registers
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    
    ; Return from interrupt
    iret 