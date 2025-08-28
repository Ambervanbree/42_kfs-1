#include "kernel.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"
#include "kprintf.h"

void memory_init(uint32_t mem_bytes)
{
	// Initialize physical memory manager with a cap
	pmm_init(mem_bytes);
	// Set up paging structures and enable paging
	paging_init();
	paging_enable();
	// Initialize kernel heap
	kheap_init();
	kprintf("Memory subsystem initialized.\n");
}

