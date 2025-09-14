#include "pmm.h"
#include "panic.h"
#include "kprintf.h"

#define PMM_START 0x00100000u
#define PMM_LIMIT_BYTES (10u * 1024u * 1024u)

static uint32_t total_pages = 0;
static uint32_t free_pages = 0;

// Bitmap: 1 = used, 0 = free
static uint32_t bitmap[(PMM_LIMIT_BYTES / PAGE_SIZE) / 32 + 1];

static inline void set_bit(uint32_t idx) { bitmap[idx >> 5] |= (1u << (idx & 31)); }
static inline void clr_bit(uint32_t idx) { bitmap[idx >> 5] &= ~(1u << (idx & 31)); }
static inline int  tst_bit(uint32_t idx) { return (bitmap[idx >> 5] >> (idx & 31)) & 1u; }

void pmm_init(uint32_t mem_size_bytes)
{
	if (mem_size_bytes > PMM_LIMIT_BYTES) mem_size_bytes = PMM_LIMIT_BYTES;
	total_pages = mem_size_bytes / PAGE_SIZE;
	free_pages = 0;
	for (uint32_t i = 0; i < (sizeof(bitmap)/sizeof(bitmap[0])); i++) bitmap[i] = 0xFFFFFFFFu; // mark all used
	// Mark available pages as free starting from PMM_START
	uint32_t start_page = 0; // index 0 corresponds to PMM_START
	for (uint32_t i = start_page; i < total_pages; i++) {
		clr_bit(i);
		free_pages++;
	}
	// Reserve first few pages for kernel (roughly 1MB for now)
	uint32_t reserve_pages = (1024u * 1024u) / PAGE_SIZE;
	if (reserve_pages > total_pages) reserve_pages = total_pages;
	for (uint32_t i = 0; i < reserve_pages; i++) {
		if (!tst_bit(i)) { set_bit(i); free_pages--; }
	}
	kprintf("PMM: total=%u pages, free=%u pages\n", total_pages, free_pages);
}

void *pmm_alloc_page(void)
{
	for (uint32_t i = 0; i < total_pages; i++) {
		if (!tst_bit(i)) {
			set_bit(i);
			free_pages--;
			return (void*)(PMM_START + i * PAGE_SIZE);
		}
	}
	kpanic_fatal("PMM out of memory\n");
	return NULL;
}

void pmm_free_page(void *page)
{
	uint32_t addr = (uint32_t)page;
	if (addr < PMM_START) return; // ignore
	uint32_t idx = (addr - PMM_START) / PAGE_SIZE;
	if (idx >= total_pages) return;
	if (!tst_bit(idx)) { kpanic("Double free page %x\n", addr); return; }
	clr_bit(idx);
	free_pages++;
}

uint32_t pmm_free_pages(void) { return free_pages; }
uint32_t pmm_total_pages(void) { return total_pages; }

// Physical memory break - simple implementation
static void *current_brk = 0;

void *pmm_brk(void *new_brk)
{
	if (new_brk == 0) {
		// Get current break
		if (current_brk == 0) {
			current_brk = (void*)PMM_START;
		}
		return current_brk;
	}
	
	// Simple implementation: just track the break point
	// In a real OS, this would manage a physical heap
	uint32_t new_addr = (uint32_t)new_brk;
	uint32_t start_addr = PMM_START;
	uint32_t max_addr = PMM_START + total_pages * PAGE_SIZE;
	
	if (new_addr < start_addr || new_addr > max_addr) {
		return (void*)-1; // Invalid break
	}
	
	current_brk = new_brk;
	return current_brk;
}

