

#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "interrupt.h"



void kernel_main(void) //uint32_t magic, struct multiboot_info* mbi?
{

    screen_init();
    screen_clear();
    
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
    screen_putstring("Welcome to My Kernel!\n");
    screen_putstring("Kernel from Scratch - KFS_1\n\n");
    
    /* Display the required "42" */
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    screen_putstring("The answer to everything: ");
    screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    screen_putstring("42\n\n");
    
    /* Display some system information */
    screen_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
    screen_putstring("System Information:\n");
    screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    screen_putstring("- Architecture: i386 (x86)\n");
    screen_putstring("- Boot loader: GRUB\n");
    screen_putstring("- Kernel successfully loaded!\n\n");
    
    /* Display success message */
    screen_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    screen_putstring("Kernel initialization complete.\n");
    screen_putstring("System ready!\n\n");
    
    /* Display keyboard instructions */
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    screen_putstring("Keyboard input enabled!\n");
    screen_putstring("Type to see characters appear on screen.\n\n");
    
    /* Kernel main loop - interrupts will handle keyboard input */
    while (1) {
        /* Halt and wait for interrupts */
        asm volatile("hlt");
    }
}