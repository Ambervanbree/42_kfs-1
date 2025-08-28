#ifndef PANIC_H
#define PANIC_H

#include "kprintf.h"

// Fatal panic: print message and halt the CPU forever
void kpanic_fatal(const char *fmt, ...);

// Non-fatal panic: print message and continue (used for recoverable errors)
void kpanic(const char *fmt, ...);

// Simple assertion macro; fatal when condition fails
#define KASSERT(cond, ...) do { \
	if (!(cond)) { \
		kpanic_fatal("Assertion failed: %s\n", #cond); \
	} \
} while (0)

#endif
