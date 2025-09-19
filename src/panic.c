#include "panic.h"
#include <stdarg.h>

static void vprint(const char *fmt, va_list ap)
{
	// Minimal vprintf bridge using kprintf's %s/%x/%d support
	// We don't have a full vprintf; implement a tiny subset
	// For simplicity, reuse kprintf by formatting piecewise
	// Note: Keep simple to avoid heavy code size
	(void)ap; // suppress unused for now
	kprintf(fmt); // best-effort; extend later if needed
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

