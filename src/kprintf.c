#include <stdarg.h>
#include "screen.h"

// Helper to print an integer in decimal
static void print_decimal(int value) {
    char buffer[12];
    int i = 0, is_negative = 0;

    if (value == 0) {
        screen_putchar('0');
        return;
    }
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    if (is_negative)
        buffer[i++] = '-';
    while (i--)
        screen_putchar(buffer[i]);
}

static void print_hex(unsigned int value) {
    int i = 0;
    if (value == 0) {
        screen_putstring("0x0");
        return;
    }
    screen_putstring("0x");
    for (i = 7; i >= 0; i--) {
        int nibble = (value >> (i * 4)) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        screen_putchar(c);
    }
}

void kprintfv(const char *fmt, va_list args) {
    for (; *fmt; ++fmt) {
        if (*fmt == '%') {
            ++fmt;
            if (*fmt == 's') {
                char *str = va_arg(args, char*);
                screen_putstring(str);
            } else if (*fmt == 'c') {
                char c = (char)va_arg(args, int);
                screen_putchar(c);
            } else if (*fmt == 'd') {
                int val = va_arg(args, int);
                print_decimal(val);
            } else if (*fmt == 'x') {
                unsigned int val = va_arg(args, unsigned int);
                print_hex(val);
            } else if (*fmt == 'p') {
                void *ptr = va_arg(args, void*);
                print_hex((unsigned int)ptr);
            } else if (*fmt == '%') {
                screen_putchar('%');
            }
        } else {
            screen_putchar(*fmt);
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kprintfv(fmt, args);
    va_end(args);
}