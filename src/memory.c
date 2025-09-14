#include "kernel.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"
#include "user_mem.h"
#include "process.h"
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
	// Initialize user memory management
	user_mem_init();
	// Initialize process management
	process_init();
	kprintf("Memory subsystem initialized with user space support.\n");
}

