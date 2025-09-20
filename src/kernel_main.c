#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "keyboard.h"
#include "shell.h"
#include "panic.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"

// External symbols from GDT
extern void *gdt;
extern void *gdt_end;
extern void gdt_setup_at_required_address(void);

void kernel_main(uint32_t magic, uint32_t *multiboot_info) 
{
    // Suppress unused parameter warnings
    (void)magic;
    (void)multiboot_info;
    
    // Copy GDT to required address 0x00000800
    gdt_setup_at_required_address();
    
    screen_init();
    keyboard_init();
    interrupt_init();

    memory_init(PMM_MAX_BYTES);  // Use shared constant

    // Display GDT info
    // kprintf("GDT relocated to %x\n", 0x00000800);
    // kprintf("Kernel segments: Code=0x08, Data=0x10, Stack=0x18\n");
    // kprintf("User segments: Code=0x20, Data=0x28, Stack=0x30\n\n");
    
    // Initialize and start the shell
    shell_init();
    
    while (1) {
        // halt and wait for interrupts
        asm volatile("hlt");
    }
}
    