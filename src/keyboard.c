#include "keyboard.h"
#include "screen.h"

/* Color definitions for debug output */
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_LIGHT_RED 12

/* Forward declaration */
static char scancode_to_ascii(uint8_t scancode);

/* Global keyboard state */
struct keyboard_state keyboard_state = {0, 0, 0, 0, 0};

/* Enhanced scancode tables */
static const char ascii_table[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char ascii_table_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

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
    /* ICW1: start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    /* ICW2: remap IRQ table */
    outb(PIC1_DATA, IRQ0);     /* IRQ 0-7 -> interrupts 32-39 */
    outb(PIC2_DATA, IRQ8);     /* IRQ 8-15 -> interrupts 40-47 */
    
    /* ICW3: tell PICs how they're cascaded */
    outb(PIC1_DATA, 4);        /* IRQ2 connects to slave PIC */
    outb(PIC2_DATA, 2);        /* Slave PIC is connected to IRQ2 */
    
    /* ICW4: set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    /* Mask all interrupts except keyboard (IRQ1) */
    outb(PIC1_DATA, 0xFD);     /* Enable IRQ1 (keyboard) */
    outb(PIC2_DATA, 0xFF);     /* Disable all IRQs on PIC2 */
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
    /* Set up IDT pointer */
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    
    /* Clear the IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    /* Set up keyboard interrupt handler */
    idt_set_gate(IRQ1, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);
    
    /* Load the IDT */
    idt_load();
    
    /* Initialize PIC */
    pic_init();
    
    /* Enable interrupts */
    asm volatile("sti");
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
    uint8_t scancode = inb(0x60);
    
    /* Handle key press (bit 7 not set) */
    if (!(scancode & 0x80)) {
        /* Handle modifier keys */
        switch (scancode) {
            case KEY_LEFT_SHIFT:
            case KEY_RIGHT_SHIFT:
                keyboard_state.shift_pressed = 1;
                break;
            case KEY_LEFT_CTRL:
                keyboard_state.ctrl_pressed = 1;
                break;
            case KEY_CAPS_LOCK:
                keyboard_state.caps_lock = !keyboard_state.caps_lock;
                break;
            case KEY_F1:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F1]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F1]");
                } else {
                    screen_putstring("[F1]");
                }
                break;
            case KEY_F2:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F2]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F2]");
                } else {
                    screen_putstring("[F2]");
                }
                break;
            case KEY_F3:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F3]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F3]");
                } else {
                    screen_putstring("[F3]");
                }
                break;
            case KEY_F4:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F4]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F4]");
                } else {
                    screen_putstring("[F4]");
                }
                break;
            case KEY_F5:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F5]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F5]");
                } else {
                    screen_putstring("[F5]");
                }
                break;
            case KEY_F6:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F6]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F6]");
                } else {
                    screen_putstring("[F6]");
                }
                break;
            case KEY_F7:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F7]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F7]");
                } else {
                    screen_putstring("[F7]");
                }
                break;
            case KEY_F8:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F8]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F8]");
                } else {
                    screen_putstring("[F8]");
                }
                break;
            case KEY_F9:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F9]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F9]");
                } else {
                    screen_putstring("[F9]");
                }
                break;
            case KEY_F10:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F10]");
                } else if (keyboard_state.alt_pressed) {
                    screen_putstring("[Alt+F10]");
                } else {
                    screen_putstring("[F10]");
                }
                break;
            default:
                /* Convert scancode to ASCII and display */
                char c = scancode_to_ascii(scancode);
                if (c != 0) {
                    screen_putchar(c);
                }
                break;
        }
    } else {
        /* Handle key release */
        uint8_t key_code = scancode & 0x7F;
        switch (key_code) {
            case KEY_LEFT_SHIFT:
            case KEY_RIGHT_SHIFT:
                keyboard_state.shift_pressed = 0;
                break;
            case KEY_LEFT_CTRL:
                keyboard_state.ctrl_pressed = 0;
                break;
        }
    }
    
    /* Send EOI */
    pic_send_eoi(IRQ1);
}

/* Enhanced scancode to ASCII conversion */
static char scancode_to_ascii(uint8_t scancode)
{
    char c;
    
    /* Choose the appropriate table based on shift state */
    if (keyboard_state.shift_pressed) {
        c = ascii_table_shift[scancode];
    } else {
        c = ascii_table[scancode];
    }
    
    /* Handle caps lock for letters */
    if (keyboard_state.caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (keyboard_state.caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    return c;
}

/* Initialize keyboard */
void keyboard_init(void)
{
    /* Clear any pending input */
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    /* Enable keyboard */
    outb(0x64, 0xAE);  /* Enable keyboard interface */
    
    /* Wait for keyboard to be ready */
    while (inb(0x64) & 0x02);  /* Wait for input buffer to be empty */
} 