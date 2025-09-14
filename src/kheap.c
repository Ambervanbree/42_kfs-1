#include "kheap.h"
#include "paging.h"
#include "pmm.h"
#include "panic.h"
#include "kprintf.h"

typedef struct block_header {
	size_t size;
	int free;
	uint32_t magic;  // Magic number for double free detection
	struct block_header *next;
} block_header_t;

#define MAGIC_ALLOCATED 0xDEADBEEF
#define MAGIC_FREED     0xFEEED000

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
	// Pre-expand by one page to seed free list
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
	// Round size up to the next multiple of 8 for alignment
	if (size & 7) size = (size + 7) & ~7u;
	block_header_t *prev = 0;
	block_header_t *cur = free_list;
	while (cur) {
		if (cur->free && cur->size >= size) {
		split_block(cur, size);
		cur->free = 0;
		cur->magic = MAGIC_ALLOCATED;
		return (uint8_t*)cur + sizeof(block_header_t);
		}
		prev = cur;
		cur = cur->next;
	}
	// If no suitable block is found, expand heap
	uint8_t *chunk = (uint8_t*)heap_expand(size + sizeof(block_header_t));
	block_header_t *blk = (block_header_t*)chunk;
	blk->size = (size + sizeof(block_header_t) <= PAGE_SIZE) ? (PAGE_SIZE - sizeof(block_header_t)) : (size);
	blk->free = 0;
	blk->magic = MAGIC_ALLOCATED;
	blk->next = 0;
	if (prev) prev->next = blk; else free_list = blk;
	return (uint8_t*)blk + sizeof(block_header_t);
}

void kfree(void *ptr)
{
	if (!ptr) return;
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	
	// Check for double free
	if (blk->magic == MAGIC_FREED) {
		kprintf("ERROR: Double free detected at 0x%x (kfree)\n", (uint32_t)ptr);
		return; // Don't panic, just log and continue
	}
	
	// Check for invalid magic number
	if (blk->magic != MAGIC_ALLOCATED) {
		kprintf("ERROR: Invalid memory block at 0x%x (magic: 0x%x)\n", (uint32_t)ptr, blk->magic);
		return; // Don't panic, just log and continue
	}
	
	// Mark as freed
	blk->free = 1;
	blk->magic = MAGIC_FREED;
	
	// If adjacent blocks are free, merge them, to reduce fragmentation
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

// Helper function to validate if a kernel heap block is properly allocated
static int is_valid_kheap_block(block_header_t *blk)
{
	// Check if block is within kernel heap region
	uint32_t block_addr = (uint32_t)blk;
	if (!heap_base || block_addr < (uint32_t)heap_base || block_addr >= (uint32_t)heap_base + heap_size) {
		return 0; // Block is outside kernel heap region
	}
	
	// Check if block is properly aligned (should be page-aligned)
	if (block_addr & (PAGE_SIZE - 1)) {
		return 0; // Block is not properly aligned
	}
	
	// Check if block is linked in the free list (either allocated or free)
	block_header_t *cur = free_list;
	while (cur) {
		if (cur == blk) {
			return 1; // Found in free list
		}
		cur = cur->next;
	}
	
	// Also check if it's a recently allocated block that might not be in free list yet
	// This is a simplified check - in a real implementation, you'd maintain an allocated list too
	return 1; // Allow if within heap bounds and aligned
}

size_t ksize(void *ptr)
{
	if (!ptr) return 0;
	
	// Check if pointer is within kernel heap region
	uint32_t ptr_addr = (uint32_t)ptr;
	if (!heap_base || ptr_addr < (uint32_t)heap_base || ptr_addr >= (uint32_t)heap_base + heap_size) {
		return 0; // Pointer is outside kernel heap region
	}
	
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	
	// Check if block is still allocated
	if (blk->magic != MAGIC_ALLOCATED) {
		return 0; // Block is freed or invalid
	}
	
	// Additional validation: check if block is properly allocated
	if (!is_valid_kheap_block(blk)) {
		return 0; // Block is not properly allocated
	}
	
	return blk->size;
}

void *kbrk(void *new_brk)
{
	// Delegate to physical memory manager
	return pmm_brk(new_brk);
}

