#include "interrupt.h"
#include "screen.h"

/* Color definitions for debug output */
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_LIGHT_RED 12

/* Forward declaration */
static char scancode_to_ascii(uint8_t scancode);

/* IDT and interrupt handlers */
struct idt_entry idt[256];
struct idt_ptr idtp;

/* I/O functions */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Initialize PIC */
void pic_init(void)
{
    screen_putstring("PIC: Starting init...\n");
    
    /* ICW1: start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    screen_putstring("PIC: ICW1 sent\n");
    
    /* ICW2: remap IRQ table */
    outb(PIC1_DATA, IRQ0);     /* IRQ 0-7 -> interrupts 32-39 */
    outb(PIC2_DATA, IRQ8);     /* IRQ 8-15 -> interrupts 40-47 */
    screen_putstring("PIC: ICW2 sent\n");
    
    /* ICW3: tell PICs how they're cascaded */
    outb(PIC1_DATA, 4);        /* IRQ2 connects to slave PIC */
    outb(PIC2_DATA, 2);        /* Slave PIC is connected to IRQ2 */
    screen_putstring("PIC: ICW3 sent\n");
    
    /* ICW4: set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    screen_putstring("PIC: ICW4 sent\n");
    
    /* Mask all interrupts except keyboard (IRQ1) */
    outb(PIC1_DATA, 0xFD);     /* Enable IRQ1 (keyboard) */
    outb(PIC2_DATA, 0xFF);     /* Disable all IRQs on PIC2 */
    screen_putstring("PIC: Masks set\n");
}

/* Set up an IDT entry */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/* Initialize interrupts */
void interrupt_init(void)
{
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    screen_putstring("Setting up IDT...\n");
    
    /* Set up IDT pointer */
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    
    /* Clear the IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    screen_putstring("Setting up IRQ1 handler...\n");
    /* Set up keyboard interrupt handler */
    idt_set_gate(IRQ1, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);
    
    /* Debug: Show handler address */
    screen_putstring("Handler addr: 0x");
    uint32_t addr = (uint32_t)keyboard_handler_asm;
    for (int i = 7; i >= 0; i--) {
        uint8_t digit = (addr >> (i * 4)) & 0xF;
        screen_putchar(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    screen_putstring("\n");
    
    screen_putstring("Loading IDT...\n");
    /* Load the IDT */
    idt_load();
    
    screen_putstring("Initializing PIC...\n");
    /* Initialize PIC */
    pic_init();
    
    screen_putstring("Enabling interrupts...\n");
    /* Enable interrupts */
    asm volatile("sti");
    
    screen_putstring("Interrupt system ready!\n");
    
    /* Test: Try to trigger a software interrupt */
    screen_putstring("Testing interrupt system...\n");
    asm volatile("int $33");  /* Trigger IRQ1 manually */
    screen_putstring("Manual interrupt test complete!\n");
}

/* Send EOI to PIC */
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}

/* Keyboard interrupt handler */
void keyboard_handler(void)
{
    /* Debug: Show we entered the handler */
    screen_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    screen_putstring("IRQ1!");
    
    uint8_t scancode = inb(0x60);
    
    /* Debug: Show scancode in hex */
    screen_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    screen_putstring("SC:0x");
    screen_putchar(scancode < 16 ? '0' : '0' + (scancode / 16));
    screen_putchar(scancode % 16 < 10 ? '0' + (scancode % 16) : 'A' + (scancode % 16) - 10);
    screen_putchar(' ');
    
    /* Handle key press (bit 7 not set) */
    if (!(scancode & 0x80)) {
        /* Convert scancode to ASCII and display */
        char c = scancode_to_ascii(scancode);
        if (c != 0) {
            screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            screen_putchar(c);
        }
    } else {
        /* Key release - just show it for debugging */
        screen_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        screen_putstring("REL ");
    }
    
    /* Send EOI */
    pic_send_eoi(IRQ1);
    
    /* Debug: Show we're leaving */
    screen_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    screen_putstring("OK ");
}

/* Simple scancode to ASCII conversion */
static char scancode_to_ascii(uint8_t scancode)
{
    static const char ascii_table[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };
    
    if (scancode < sizeof(ascii_table)) {
        return ascii_table[scancode];
    }
    return 0;
}

/* Initialize keyboard */
void keyboard_init(void)
{
    screen_putstring("Keyboard: Starting init...\n");
    
    /* Clear any pending input */
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    /* Enable keyboard */
    outb(0x64, 0xAE);  /* Enable keyboard interface */
    
    /* Wait for keyboard to be ready */
    while (inb(0x64) & 0x02);  /* Wait for input buffer to be empty */
    
    screen_putstring("Keyboard: Ready!\n");
} 