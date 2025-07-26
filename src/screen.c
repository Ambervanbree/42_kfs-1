#include "screen.h"
#include "string.h"
#include "kprintf.h"

struct screen_state states[MAX_SCREENS];
static int current_screen;

/* VGA text buffer address */
static volatile uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;

/* Current cursor position */
static size_t cursor_x = 0;
static size_t cursor_y = 0;

/* Current color */
static uint8_t current_color = VGA_COLOR_LIGHT_GREY | VGA_COLOR_BLACK << 4;

/* Helper function to create VGA entry */
static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t)uc | (uint16_t)color << 8;
}

/* Helper function to create color attribute */
static inline uint8_t vga_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

/* Update the hardware cursor position */
static void update_hardware_cursor(void)
{
    uint16_t pos = (uint16_t)(cursor_y * SCREEN_WIDTH + cursor_x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Initialize screen */
void screen_init(void)
{
    cursor_x = 0;
    cursor_y = 0;
    current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    screen_clear();
    init_screen_if_needed(0);
    current_screen = 0;
    update_hardware_cursor();
}

void init_screen_if_needed(int n){
    if (*(uint16_t*)states[n].buffer != 0) return;

    if (n == 0) {
        current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    } else if (n==1){
        current_color = vga_color(VGA_COLOR_BLUE, VGA_COLOR_LIGHT_GREY);
    } else if (n == 2) {
        current_color = vga_color(VGA_COLOR_RED, VGA_COLOR_WHITE);
    }

    screen_clear();
    if (n == 0){
        load_home_screen();
    }
    kprintf("This is screen %d.\n\n", n + 1);
    
    memcpy(states[n].buffer, (void*)VGA_BUFFER, SCREEN_SIZE);
    states[n].cursor_x = cursor_x;
    states[n].cursor_y = cursor_y;
    states[n].color = current_color;
}

void load_home_screen() {
    kprintf("Welcome to %s!\n", KERNEL_NAME);
    kprintf("Kernel from Scratch - KFS_1\n\n");
    
    screen_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("The answer to everything: %d\n", 42);
    
    screen_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
    kprintf("System Information:\n");
    screen_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("- Architecture: %s\n", ARCHITECTURE);
    kprintf("- Boot loader: %s\n", BOOTLOADER);
    kprintf("- %s successfully loaded!\n\n", KERNEL_NAME);
    
    screen_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("This kernel supports up to 3 screens. Press Alt+F1, Alt+F2 or Alt+F3 to switch between them.\n\n");
}

/* Clear screen */
void screen_clear(void)
{
    const size_t size = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (size_t i = 0; i < size; i++) {
        VGA_BUFFER[i] = vga_entry(' ', current_color);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor();
}

void screen_scroll(void)
{
    /* Move all lines up */
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t src_index = (y + 1) * SCREEN_WIDTH + x;
            const size_t dst_index = y * SCREEN_WIDTH + x;
            VGA_BUFFER[dst_index] = VGA_BUFFER[src_index];
        }
    }
    
    /* Clear the bottom line */
    for (size_t x = 0; x < SCREEN_WIDTH; x++) {
        const size_t index = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x;
        VGA_BUFFER[index] = vga_entry(' ', current_color);
    }
    
    cursor_y = SCREEN_HEIGHT - 1;
    update_hardware_cursor();
}

void screen_get_cursor(size_t* x, size_t* y)
{
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

/* Put character at current cursor position */
void screen_putchar(char c)
{
    if (c == '\n') {
        /* Newline */
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        /* Tab - align to next 4-character boundary */
        cursor_x = (cursor_x + TAB_WIDTH) & ~(TAB_WIDTH - 1);
    } else if (c == '\r') {
        /* Carriage return */
        cursor_x = 0;
    } else if (c == '\b') {
        /* Backspace */
        if (cursor_x > 0) {
            cursor_x--;
            const size_t index = cursor_y * SCREEN_WIDTH + cursor_x;
            VGA_BUFFER[index] = vga_entry(' ', current_color);
        }
    } else {
        /* Regular character */
        const size_t index = cursor_y * SCREEN_WIDTH + cursor_x;
        VGA_BUFFER[index] = vga_entry(c, current_color);
        cursor_x++;
    }
    
    /* Handle line wrap */
    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    /* Handle screen overflow */
    if (cursor_y >= SCREEN_HEIGHT) {
        screen_scroll();
    }
    update_hardware_cursor();
}

/* Put string on screen */
void screen_putstring(const char* str)
{
    const size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        screen_putchar(str[i]);
    }
}

/* Set text color */
void screen_set_color(enum vga_color fg, enum vga_color bg)
{
    current_color = vga_color(fg, bg);
}

/* Set cursor position */
void screen_set_cursor(size_t x, size_t y)
{
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_hardware_cursor();
    }
}

void switch_screen(int n) {
    if ((n < 0 || n > MAX_SCREENS) || n == current_screen) return;

    memcpy(states[current_screen].buffer, (void*)VGA_BUFFER, SCREEN_SIZE);
    states[current_screen].cursor_x = cursor_x;
    states[current_screen].cursor_y = cursor_y;
    states[current_screen].color = current_color;

    init_screen_if_needed(n);

    memcpy((void*)VGA_BUFFER, states[n].buffer, SCREEN_SIZE);
    cursor_x = states[n].cursor_x;
    cursor_y = states[n].cursor_y;
    current_color = states[n].color;
    current_screen = n;

    update_hardware_cursor();
}