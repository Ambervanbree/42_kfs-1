#include "screen.h"
#include "string.h"
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
        kprintf("This is screen %d.\n\n", i + 1);
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
    kprintf("The answer to everything: 42\n");
    
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
    } else if (c == '\t') {
        current_screen->cursor_x = (current_screen->cursor_x + TAB_WIDTH) & ~(TAB_WIDTH - 1);
    } else if (c == '\r') {
        current_screen->cursor_x = 0;
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

    // line overflow
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
