#include "screen.h"
#include "string.h"
#include "shell.h"
#include "kprintf.h"

static struct screen_state states[MAX_SCREENS];
struct screen_state* current_screen;

static volatile uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;

// helper to create VGA entry
static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t)uc | (uint16_t)color << 8;
}

// helper to create color attribute
static inline uint8_t vga_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

static void update_hardware_cursor()
{
    uint16_t pos = (uint16_t)(current_screen->cursor_y * SCREEN_WIDTH + current_screen->cursor_x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void screen_init(void)
{
   for (int i = 0; i < MAX_SCREENS; i++) {
        memset(states[i].buffer, 0, SCREEN_SIZE);
        states[i].cursor_x = 0;
        states[i].cursor_y = 0;
        states[i].input_length = 0;
        states[i].input_cursor = 0;
        states[i].input_start_x = 0;
        states[i].input_start_y = 0;
        if (i == 0) {
            states[i].color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        } else if (i == 1) {
            states[i].color = vga_color(VGA_COLOR_BLUE, VGA_COLOR_LIGHT_GREY);
        } else if (i == 2) {
            states[i].color = vga_color(VGA_COLOR_MAGENTA, VGA_COLOR_WHITE);
        } else {
            states[i].color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }

        current_screen = &states[i];
        screen_clear();

        if (i == 0)
            load_home_screen();
        shell_print_prompt();
        memcpy(current_screen->buffer, (void*)VGA_BUFFER, SCREEN_SIZE);
        update_hardware_cursor();
    }
    current_screen = &states[0];
    memcpy((void*)VGA_BUFFER, current_screen->buffer, SCREEN_SIZE);
    update_hardware_cursor();
}

void load_home_screen() {
    kprintf("Welcome to KrnL!\n");
    kprintf("Kernel from Scratch - 1\n\n");
    
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    // kprintf("The answer to everything: 42\n");
    
    screen_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
    kprintf("System Information:\n");
    screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    kprintf("- Architecture: %s\n", ARCHITECTURE);

    kprintf("Boot loader: %s\n", BOOTLOADER);

    kprintf("- KrnL successfully loaded!\n\n");
    
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("This kernel supports up to 3 screens. Press F1, F2 or F3 to switch between them.\n\n");
}

void screen_clear()
{
    const size_t size = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (size_t i = 0; i < size; i++) {
        VGA_BUFFER[i] = vga_entry(' ', current_screen->color);
    }
    current_screen->cursor_x = 0;
    current_screen->cursor_y = 0;
    update_hardware_cursor();
}

void screen_scroll()
{
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t src_index = (y + 1) * SCREEN_WIDTH + x;
            const size_t dst_index = y * SCREEN_WIDTH + x;
            VGA_BUFFER[dst_index] = VGA_BUFFER[src_index];
        }
    }
    
    for (size_t x = 0; x < SCREEN_WIDTH; x++) {
        const size_t index = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x;
        VGA_BUFFER[index] = vga_entry(' ', current_screen->color);
    }
    
    current_screen->cursor_y = SCREEN_HEIGHT - 1;
    update_hardware_cursor();
}

void screen_get_cursor(size_t* x, size_t* y)
{
    if (x) *x = current_screen->cursor_x;
    if (y) *y = current_screen->cursor_y;
}

void screen_set_color(enum vga_color fg, enum vga_color bg)
{
    current_screen->color = vga_color(fg, bg);
    update_hardware_cursor();
}

void screen_putchar(char c)
{
    if (c == '\n') {
        current_screen->cursor_x = 0;
        current_screen->cursor_y++;
    } else if (c == '\b') {
        if (current_screen->cursor_x > 0) {
            current_screen->cursor_x--;
            const size_t index = current_screen->cursor_y * SCREEN_WIDTH + current_screen->cursor_x;
            VGA_BUFFER[index] = vga_entry(' ', current_screen->color);
        }
    } else {
        const size_t index = current_screen->cursor_y * SCREEN_WIDTH + current_screen->cursor_x;
        VGA_BUFFER[index] = vga_entry(c, current_screen->color);
        current_screen->cursor_x++;
    }

    // line overflow //
    if (current_screen->cursor_x > SCREEN_WIDTH) {
        current_screen->cursor_x = 0;
        current_screen->cursor_y++;
    }
    
    // screen overflow
    if (current_screen->cursor_y >= SCREEN_HEIGHT) {
        screen_scroll();
    }
    
    update_hardware_cursor();
}

void screen_putstring(const char* str)
{
    const size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        screen_putchar(str[i]);
    }
}

void screen_set_cursor(size_t x, size_t y)
{
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        current_screen->cursor_x = x;
        current_screen->cursor_y = y;
        update_hardware_cursor();
    }
}

void switch_screen(int n) {
    if ((n < 0 || n >= MAX_SCREENS) || current_screen == &states[n]) return;

    memcpy(current_screen->buffer, (void*)VGA_BUFFER, SCREEN_SIZE);
    current_screen = &states[n];
    memcpy((void*)VGA_BUFFER, current_screen->buffer, SCREEN_SIZE);
    update_hardware_cursor();
}

// Unified input line management functions

void input_insert_char_at_cursor(char c) {
    if (c >= 32 && c <= 126) { // printable ASCII characters
        if (current_screen->input_length < SCREEN_SIZE - 1) {
            // Allow visual wrapping without resetting input state
            
            // shift buffer right from cursor to make space
            for (size_t i = current_screen->input_length; i > current_screen->input_cursor; i--) {
                current_screen->buffer[i] = current_screen->buffer[i - 1];
            }
            current_screen->buffer[current_screen->input_cursor] = c;
            current_screen->input_length++;
            current_screen->buffer[current_screen->input_length] = '\0';
            
            // Redraw the input line from the insertion point
            // Calculate actual screen positions relative to input start
            for (size_t i = current_screen->input_cursor; i < current_screen->input_length; i++) {
                size_t screen_x = current_screen->input_start_x + i;
                size_t screen_y = current_screen->input_start_y;
                
                // Handle line wrapping
                while (screen_x >= SCREEN_WIDTH) {
                    screen_x -= SCREEN_WIDTH;
                    screen_y++;
                }
                // If we wrapped beyond the bottom, scroll and adjust anchor
                if (screen_y >= SCREEN_HEIGHT) {
                    screen_scroll();
                    if (current_screen->input_start_y > 0) {
                        current_screen->input_start_y--;
                    }
                    screen_y = SCREEN_HEIGHT - 1;
                }
                
                if (screen_y < SCREEN_HEIGHT) {
                    const size_t index = screen_y * SCREEN_WIDTH + screen_x;
                    VGA_BUFFER[index] = vga_entry(current_screen->buffer[i], current_screen->color);
                }
            }
            
            // Erase the character after the new end if needed
            size_t end_x = current_screen->input_start_x + current_screen->input_length;
            size_t end_y = current_screen->input_start_y;
            while (end_x >= SCREEN_WIDTH) {
                end_x -= SCREEN_WIDTH;
                end_y++;
            }
            if (end_y >= SCREEN_HEIGHT) {
                screen_scroll();
                if (current_screen->input_start_y > 0) {
                    current_screen->input_start_y--;
                }
                end_y = SCREEN_HEIGHT - 1;
            }
            if (end_y < SCREEN_HEIGHT) {
                const size_t index = end_y * SCREEN_WIDTH + end_x;
                VGA_BUFFER[index] = vga_entry(' ', current_screen->color);
            }
            
            // Move cursor to after inserted character
            current_screen->input_cursor++;
            size_t cursor_x = current_screen->input_start_x + current_screen->input_cursor;
            size_t cursor_y = current_screen->input_start_y;
            while (cursor_x >= SCREEN_WIDTH) {
                cursor_x -= SCREEN_WIDTH;
                cursor_y++;
            }
            if (cursor_y >= SCREEN_HEIGHT) {
                screen_scroll();
                if (current_screen->input_start_y > 0) {
                    current_screen->input_start_y--;
                }
                cursor_y = SCREEN_HEIGHT - 1;
            }
            
            current_screen->cursor_x = cursor_x;
            current_screen->cursor_y = cursor_y;
            update_hardware_cursor();
        }
    }
}

void input_delete_char_at_cursor(void) {
    if (current_screen->input_cursor > 0 && current_screen->input_length > 0) {
        // shift buffer left from cursor
        for (size_t i = current_screen->input_cursor - 1; i < current_screen->input_length - 1; i++) {
            current_screen->buffer[i] = current_screen->buffer[i + 1];
        }
        current_screen->input_length--;
        current_screen->buffer[current_screen->input_length] = '\0';
        current_screen->input_cursor--;
        
        // Redraw the input line from deletion point
        for (size_t i = current_screen->input_cursor; i < current_screen->input_length; i++) {
            size_t screen_x = current_screen->input_start_x + i;
            size_t screen_y = current_screen->input_start_y;
            
            while (screen_x >= SCREEN_WIDTH) {
                screen_x -= SCREEN_WIDTH;
                screen_y++;
            }
            
            if (screen_y < SCREEN_HEIGHT) {
                const size_t index = screen_y * SCREEN_WIDTH + screen_x;
                VGA_BUFFER[index] = vga_entry(current_screen->buffer[i], current_screen->color);
            }
        }
        
        // Erase the character after the new end
        size_t end_x = current_screen->input_start_x + current_screen->input_length;
        size_t end_y = current_screen->input_start_y;
        while (end_x >= SCREEN_WIDTH) {
            end_x -= SCREEN_WIDTH;
            end_y++;
        }
        if (end_y < SCREEN_HEIGHT) {
            const size_t index = end_y * SCREEN_WIDTH + end_x;
            VGA_BUFFER[index] = vga_entry(' ', current_screen->color);
        }
        
        // Position cursor after deletion
        size_t cursor_x = current_screen->input_start_x + current_screen->input_cursor;
        size_t cursor_y = current_screen->input_start_y;
        while (cursor_x >= SCREEN_WIDTH) {
            cursor_x -= SCREEN_WIDTH;
            cursor_y++;
        }
        
        current_screen->cursor_x = cursor_x;
        current_screen->cursor_y = cursor_y;
        update_hardware_cursor();
    }
}

void input_move_cursor_left(void) {
    if (current_screen->input_cursor > 0) {
        current_screen->input_cursor--;
        
        // Calculate new cursor position relative to input start
        size_t cursor_x = current_screen->input_start_x + current_screen->input_cursor;
        size_t cursor_y = current_screen->input_start_y;
        while (cursor_x >= SCREEN_WIDTH) {
            cursor_x -= SCREEN_WIDTH;
            cursor_y++;
        }
        
        current_screen->cursor_x = cursor_x;
        current_screen->cursor_y = cursor_y;
        update_hardware_cursor();
    }
}

void input_move_cursor_right(void) {
    if (current_screen->input_cursor < current_screen->input_length) {
        current_screen->input_cursor++;
        
        // Calculate new cursor position relative to input start
        size_t cursor_x = current_screen->input_start_x + current_screen->input_cursor;
        size_t cursor_y = current_screen->input_start_y;
        while (cursor_x >= SCREEN_WIDTH) {
            cursor_x -= SCREEN_WIDTH;
            cursor_y++;
        }
        
        current_screen->cursor_x = cursor_x;
        current_screen->cursor_y = cursor_y;
        update_hardware_cursor();
    }
}

void input_set_start_position(void) {
    // Set input start position to current cursor position
    current_screen->input_start_x = current_screen->cursor_x;
    current_screen->input_start_y = current_screen->cursor_y;
    current_screen->input_length = 0;
    current_screen->input_cursor = 0;
}

void input_newline(void) {
    // Null-terminate the current input buffer
    current_screen->buffer[current_screen->input_length] = '\0';
    
    // Move to new line first
    screen_putchar('\n');
    
    // Process the command if there's input
    if (current_screen->input_length > 0) {
        shell_process_input((char*)current_screen->buffer);
    } else {
        shell_print_prompt();
    }
    
    // Reset input buffer
    current_screen->input_length = 0;
    current_screen->input_cursor = 0;
}
