#include "keyboard.h"
#include "screen.h"


static char scancode_to_ascii(uint8_t scancode);

struct keyboard_state keyboard_state = {0, 0, 0, 0};

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

// IDT and interrupt handlers
struct idt_entry idt[256];
struct idt_ptr idtp;

// I/O functions
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void pic_init(void)
{
    // ICW1: start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    // ICW2: remap IRQ table
    outb(PIC1_DATA, IRQ0);     // IRQ 0-7 -> interrupts 32-39
    outb(PIC2_DATA, IRQ8);     // IRQ 8-15 -> interrupts 40-47
    
    // ICW3: tell PICs how they're cascaded
    outb(PIC1_DATA, 4);        // IRQ2 connects to slave PIC
    outb(PIC2_DATA, 2);        // slave PIC is connected to IRQ2
    
    // ICW4: set 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    // mask all interrupts except keyboard (IRQ1)
    outb(PIC1_DATA, 0xFD);     // enable IRQ1 (keyboard)
    outb(PIC2_DATA, 0xFF);     // disable all IRQs on PIC2
}

// set up an IDT entry */
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
    // set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    
    // clear the IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // set up keyboard interrupt handler
    idt_set_gate(IRQ1, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);
    
    // load the IDT
    idt_load();
    
    // initialize PIC
    pic_init();
    
    // enable interrupts
    asm volatile("sti");
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);
    static uint8_t extended = 0;
    
    // handle extended scancode prefix
    if (scancode == 0xE0) {
        extended = 1;
        pic_send_eoi(IRQ1);
        return;
    }
    
    if (!(scancode & 0x80)) {
        // key press
        size_t x, y;
        screen_get_cursor(&x, &y);
        if (extended) {
            switch (scancode) {
                case KEY_ARROW_LEFT:
                    if (current_screen->input_cursor > 0) {
                        current_screen->input_cursor--;
                        screen_set_cursor(current_screen->input_cursor, y);
                    }
                    break;
                case KEY_ARROW_RIGHT:
                    if (current_screen->input_cursor < current_screen->input_length) {
                        current_screen->input_cursor++;
                        screen_set_cursor(current_screen->input_cursor, y);
                    }
                    break;
            }
            extended = 0;
            pic_send_eoi(IRQ1);
            return;
        }
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
                } else {
                    switch_screen(0);
                }
                break;
            case KEY_F2:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F2]");
                } else {
                    switch_screen(1);
                }
                break;
            case KEY_F3:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F3]");
                } else {
                    switch_screen(2);
                }
                break;
            case KEY_F4:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F4]");
                } else {
                    screen_putstring("[F4]");
                }
                break;
            case KEY_F5:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F5]");
                } else {
                    screen_putstring("[F5]");
                }
                break;
            case KEY_F6:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F6]");
                } else {
                    screen_putstring("[F6]");
                }
                break;
            case KEY_F7:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F7]");
                } else {
                    screen_putstring("[F7]");
                }
                break;
            case KEY_F8:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F8]");
                } else {
                    screen_putstring("[F8]");
                }
                break;
            case KEY_F9:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F9]");
                } else {
                    screen_putstring("[F9]");
                }
                break;
            case KEY_F10:
                if (keyboard_state.ctrl_pressed) {
                    screen_putstring("[Ctrl+F10]");
                } else {
                    screen_putstring("[F10]");
                }
                break;
            default: {
                char c = scancode_to_ascii(scancode);
                if (c >= 32 && c <= 126) {
                    if (current_screen->input_length < SCREEN_WIDTH) {
                        // shift buffer right from cursor
                        for (size_t i = current_screen->input_length; i > current_screen->input_cursor; i--) {
                            current_screen->buffer[i] = current_screen->buffer[i - 1];
                        }
                        current_screen->buffer[current_screen->input_cursor] = c;
                        current_screen->input_length++;
                        current_screen->buffer[current_screen->input_length] = '\0';
                        // redraw the line from cursor
                        size_t x, y;
                        screen_get_cursor(&x, &y);
                        for (size_t i = current_screen->input_cursor; i < current_screen->input_length; i++) {
                            screen_set_cursor(i, y);
                            screen_putchar(current_screen->buffer[i]);
                        }
                        // erase the char after the new end if needed
                        screen_set_cursor(current_screen->input_length, y);
                        screen_putchar(' ');
                        // move cursor to after inserted char
                        current_screen->input_cursor++;
                        screen_set_cursor(current_screen->input_cursor, y);
                    }
                    // start at new line with line overflow
                    if (current_screen->input_length >= SCREEN_WIDTH) {
                        current_screen->input_length = 0;
                        current_screen->input_cursor = 0;
                    }
                } else if (c == '\b') { 
                    if (current_screen->input_cursor > 0 && current_screen->input_length > 0) {
                        // shift buffer left from cursor
                        for (size_t i = current_screen->input_cursor - 1; i < current_screen->input_length - 1; i++) {
                            current_screen->buffer[i] = current_screen->buffer[i + 1];
                        }
                        current_screen->input_length--;
                        current_screen->buffer[current_screen->input_length] = '\0';
                        current_screen->input_cursor--;
                        // redraw the line from cursor
                        size_t x, y;
                        screen_get_cursor(&x, &y);
                        for (size_t i = current_screen->input_cursor; i < current_screen->input_length; i++) {
                            screen_set_cursor(i, y);
                            screen_putchar(current_screen->buffer[i]);
                        }
                        // erase the char after the new end
                        screen_set_cursor(current_screen->input_length, y);
                        screen_putchar(' ');
                        // move cursor to after deleted char
                        screen_set_cursor(current_screen->input_cursor, y);
                    }
                } else if (c == '\n') {
                    current_screen->input_length = 0;
                    current_screen->input_cursor = 0;
                    screen_putchar('\n');
                }
                break;
            }
        }
    } else {
        // key release
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
    
    pic_send_eoi(IRQ1);
    extended = 0;
}

static char scancode_to_ascii(uint8_t scancode)
{
    char c;
    
    if (keyboard_state.shift_pressed) {
        c = ascii_table_shift[scancode];
    } else {
        c = ascii_table[scancode];
    }
    
    if (keyboard_state.caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (keyboard_state.caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    return c;
}

void keyboard_init(void)
{
    // clear any pending input
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    // enable keyboard interface
    outb(0x64, 0xAE); 
    
    // wait for keyboard to be ready
    while (inb(0x64) & 0x02);
} 