#include "kheap.h"
#include "paging.h"
#include "pmm.h"
#include "panic.h"
#include "kprintf.h"

// Very simple bump + free-list allocator backed by pages

typedef struct block_header {
	size_t size;
	int free;
	struct block_header *next;
} block_header_t;

static uint8_t *heap_base = 0;
static size_t heap_size = 0;
static block_header_t *free_list = 0;

static void *heap_expand(size_t min_bytes)
{
	// Allocate whole pages
	size_t pages = (min_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
	if (!heap_base) {
		heap_base = (uint8_t*)0x01000000; // 16MB virtual start for heap (example)
		heap_size = 0;
		free_list = 0;
	}
	for (size_t i = 0; i < pages; i++) {
		void *phys = pmm_alloc_page();
		uint32_t v = (uint32_t)heap_base + heap_size;
		if (vmm_map_page(v, (uint32_t)phys, PAGE_WRITE) != 0) {
			kpanic_fatal("Heap map failed\n");
		}
		heap_size += PAGE_SIZE;
	}
	return heap_base + heap_size - pages * PAGE_SIZE;
}

void kheap_init(void)
{
	// pre-expand by one page to seed free list
	uint8_t *chunk = (uint8_t*)heap_expand(PAGE_SIZE);
	free_list = (block_header_t*)chunk;
	free_list->size = PAGE_SIZE - sizeof(block_header_t);
	free_list->free = 1;
	free_list->next = 0;
	kprintf("Heap initialized at %x size %u\n", (uint32_t)heap_base, (uint32_t)heap_size);
}

static void split_block(block_header_t *blk, size_t size)
{
	if (blk->size >= size + sizeof(block_header_t) + 16) {
		block_header_t *n = (block_header_t*)((uint8_t*)blk + sizeof(block_header_t) + size);
		n->size = blk->size - size - sizeof(block_header_t);
		n->free = 1;
		n->next = blk->next;
		blk->size = size;
		blk->next = n;
	}
}

void *kmalloc(size_t size)
{
	if (size == 0) return 0;
	// Align
	if (size & 7) size = (size + 7) & ~7u;
	block_header_t *prev = 0;
	block_header_t *cur = free_list;
	while (cur) {
		if (cur->free && cur->size >= size) {
			split_block(cur, size);
			cur->free = 0;
			return (uint8_t*)cur + sizeof(block_header_t);
		}
		prev = cur;
		cur = cur->next;
	}
	// Need to expand heap
	uint8_t *chunk = (uint8_t*)heap_expand(size + sizeof(block_header_t));
	block_header_t *blk = (block_header_t*)chunk;
	blk->size = (size + sizeof(block_header_t) <= PAGE_SIZE) ? (PAGE_SIZE - sizeof(block_header_t)) : (size);
	blk->free = 0;
	blk->next = 0;
	if (prev) prev->next = blk; else free_list = blk;
	return (uint8_t*)blk + sizeof(block_header_t);
}

void kfree(void *ptr)
{
	if (!ptr) return;
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	blk->free = 1;
	// Coalesce naive
	block_header_t *cur = free_list;
	while (cur && cur->next) {
		uint8_t *end_cur = (uint8_t*)cur + sizeof(block_header_t) + cur->size;
		if (cur->free && cur->next->free && end_cur == (uint8_t*)cur->next) {
			cur->size += sizeof(block_header_t) + cur->next->size;
			cur->next = cur->next->next;
		} else {
			cur = cur->next;
		}
	}
}

size_t ksize(void *ptr)
{
	if (!ptr) return 0;
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	return blk->size;
}

