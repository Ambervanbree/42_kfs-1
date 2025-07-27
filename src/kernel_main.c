#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "keyboard.h"


void kernel_main(void) 
{
    screen_init();
    keyboard_init();
    interrupt_init();
    
    /* Check if we were booted by a Multiboot-compliant boot loader */
    // if (magic != MULTIBOOT_MAGIC) {
    //     screen_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    //     screen_putstring("Error: Not booted by Multiboot loader!\n");
    //     return;
    // }
    
    /* Kernel main loop - interrupts will handle keyboard input */
    while (1) {
        /* Halt and wait for interrupts */
        asm volatile("hlt");
    }
}
    