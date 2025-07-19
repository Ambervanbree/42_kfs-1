section .text
    global gdt_load
    global gdt_flush

; GDT structure (in text section for simplicity)
align 4
gdt:
    ; Null descriptor (required)
    dd 0x00000000
    dd 0x00000000
    
    ; Code segment descriptor (0x08)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access byte (present, ring 0, code, readable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)
    
    ; Data segment descriptor (0x10)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010010b    ; Access byte (present, ring 0, data, writable)
    db 11001111b    ; Flags + Limit (bits 16-19) (4KB granularity, 32-bit)
    db 0x00         ; Base (bits 24-31)

gdt_end:

; GDT pointer
gdt_ptr:
    dw gdt_end - gdt - 1  ; GDT limit
    dd gdt                ; GDT base

; Load GDT and reload segments
gdt_load:
    lgdt [gdt_ptr]
    ret

; Flush segments after loading GDT
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
    mov ss, ax
    ret 