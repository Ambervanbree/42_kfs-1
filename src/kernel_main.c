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
    
    /* Set nice colors */
    screen_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    
    /* Display welcome message */
    kprintf("Welcome to %s!\n", "KrnL");
    kprintf("Kernel from Scratch - KFS_1\n\n");
    
    /* Display the required "42" */
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("The answer to everything: %d\n", 42);
    
    /* Display some system information */
    screen_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
    kprintf("System Information:\n");
    screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("- Architecture: i386 (x86)\n");
    kprintf("- Boot loader: GRUB\n");
    kprintf("- Kernel successfully loaded!\n\n");
    
    /* Display success message */
    screen_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("Kernel initialization complete.\n");
    kprintf("System ready!\n\n");
    
    /* Display keyboard instructions */
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Keyboard input enabled!\n");
    kprintf("Type to see characters appear on screen.\n\n");

    int x = 12345;
    kprintf("Address of x: %x\n", (unsigned int)&x);
    kprintf("Value of x in hex: %x\n", x);
    
    /* Kernel main loop - interrupts will handle keyboard input */
    while (1) {
        /* Halt and wait for interrupts */
        asm volatile("hlt");
    }
}
    