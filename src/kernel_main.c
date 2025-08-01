#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "keyboard.h"

// External symbols from GDT
extern void *gdt;
extern void *gdt_end;
extern void gdt_setup_at_required_address(void);

void kernel_main(void) 
{
    // Copy GDT to required address 0x00000800
    gdt_setup_at_required_address();
    
    screen_init();
    keyboard_init();
    interrupt_init();
    
    // Display GDT info
    kprintf("GDT relocated to 0x%x\n", 0x00000800);
    kprintf("Kernel segments: Code=0x08, Data=0x10, Stack=0x18\n");
    kprintf("User segments: Code=0x20, Data=0x28, Stack=0x30\n");
    
    while (1) {
        // halt and wait for interrupts
        asm volatile("hlt");
    }
}
    