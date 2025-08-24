; Place GDT at required address 0x00000800
section .gdt_section
align 8
gdt:
    ; 0x00: Null descriptor (required)
    dd 0x00000000
    dd 0x00000000
    
    ; 0x08: Kernel Code Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access byte (present, ring 0, code, readable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)
    
    ; 0x10: Kernel Data Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010010b    ; Access byte (present, ring 0, data, writable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

    ; 0x18: Kernel Stack Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010110b    ; Access byte (present, ring 0, data, writable, expand-down)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

    ; 0x20: User Code Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 11111010b    ; Access byte (present, ring 3, code, readable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

    ; 0x28: User Data Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 11110010b    ; Access byte (present, ring 3, data, writable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

    ; 0x30: User Stack Segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 11110110b    ; Access byte (present, ring 3, data, writable, expand-down)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

gdt_end:

section .text
    global gdt_load
    global gdt_flush
    global gdt_setup_at_required_address

; GDT pointer (will be updated to point to 0x00000800)
gdt_ptr:
    dw gdt_end - gdt - 1  ; GDT limit
    dd gdt                ; GDT base address

; GDT pointer for required address
gdt_ptr_800:
    dw gdt_end - gdt - 1  ; GDT limit
    dd 0x00000800         ; Required GDT base address

; Copy GDT to required address 0x00000800 and load it
gdt_setup_at_required_address:
    push eax
    push ecx
    push esi
    push edi
    
    ; Calculate size of GDT
    mov ecx, gdt_end
    sub ecx, gdt
    
    ; Copy GDT from current location to 0x00000800
    mov esi, gdt          ; Source
    mov edi, 0x00000800   ; Destination
    rep movsb             ; Copy ECX bytes
    
    ; Load the new GDT at 0x00000800
    lgdt [gdt_ptr_800]
    
    ; Flush segments
    jmp 0x08:.flush_new
.flush_new:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Load stack segment separately
    mov ax, 0x18
    mov ss, ax
    
    pop edi
    pop esi
    pop ecx
    pop eax
    ret

; Load GDT and reload segments (legacy function)
gdt_load:
    lgdt [gdt_ptr]
    ret

; Flush segments after loading GDT (legacy function)
gdt_flush:
    ; Reload code segment (far jump to flush CS)
    jmp 0x08:.flush
.flush:
    ; Reload data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Load stack segment separately
    mov ax, 0x18
    mov ss, ax
    ret 