

#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "interrupt.h"

/* I/O function for polling test */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Color definition for polling test */
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BLUE 9

void kernel_main(void) //uint32_t magic, struct multiboot_info* mbi?
{

    screen_init();
    screen_clear();
    
    /* Test GDT setup */
    screen_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    screen_putstring("Testing GDT setup...\n");
    
    /* Check segment registers */
    uint16_t cs, ds, es, fs, gs, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%es, %0" : "=r"(es));
    asm volatile("mov %%fs, %0" : "=r"(fs));
    asm volatile("mov %%gs, %0" : "=r"(gs));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    
    screen_putstring("CS: 0x");
    screen_putchar('0' + (cs >> 12));
    screen_putchar('0' + ((cs >> 8) & 0xF));
    screen_putchar('0' + ((cs >> 4) & 0xF));
    screen_putchar('0' + (cs & 0xF));
    screen_putstring(" DS: 0x");
    screen_putchar('0' + (ds >> 12));
    screen_putchar('0' + ((ds >> 8) & 0xF));
    screen_putchar('0' + ((ds >> 4) & 0xF));
    screen_putchar('0' + (ds & 0xF));
    screen_putstring("\n");
    
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
    
    /* Display interrupt test instructions */
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    screen_putstring("IRQ1 (Keyboard) interrupt enabled!\n");
    screen_putstring("Type any key to test the interrupt handler.\n");
    screen_putstring("Each keypress should trigger IRQ1 and display the character.\n\n");
    
    /* Kernel main loop - interrupts will handle keyboard input */
    while (1) {
        /* Check if keyboard has data (polling test) */
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            screen_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
            screen_putstring("POLL:");
            screen_putchar('0' + (scancode / 10));
            screen_putchar('0' + (scancode % 10));
            screen_putchar(' ');
        }
        
        /* Halt and wait for interrupts */
        asm volatile("hlt");
    }
}