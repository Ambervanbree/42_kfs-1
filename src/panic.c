#include "panic.h"
#include <stdarg.h>

extern void kprintf(const char *fmt, ...);
extern void kprintfv(const char *fmt, va_list args);

static void vprint(const char *fmt, va_list ap)
{
    // Properly forward arguments to the varargs-aware printer
    kprintfv(fmt, ap);
}

void kpanic_fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprint(fmt, ap);
	va_end(ap);

	kprintf("\nKernel halted.\n");
	asm volatile("cli; hlt");
	for(;;) { asm volatile("hlt"); }
}

