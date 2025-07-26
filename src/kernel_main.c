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
    
    /* Kernel main loop - interrupts will handle keyboard input */
    while (1) {
        /* Halt and wait for interrupts */
        asm volatile("hlt");
    }
}
    