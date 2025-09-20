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
static size_t heap_used = 0;
static block_header_t *free_list = 0;

// KHEAP_VIRTUAL_START and KHEAP_SIZE are now defined in kernel.h
#define KHEAP_SIZE         (KHEAP_END - KHEAP_START + 1)  // 64MB total kernel heap size

void kheap_init(void)
{
	// Pre-map the entire kernel heap region at boot time
	heap_base = (uint8_t*)KHEAP_START;
	heap_size = KHEAP_SIZE;
	heap_used = 0;
	free_list = 0;
	
	// Map the entire kernel heap region to physical memory
	size_t pages = KHEAP_SIZE / PAGE_SIZE;
	for (size_t i = 0; i < pages; i++) {
		void *phys = pmm_alloc_page();
		if (!phys) {
			kpanic_fatal("kheap_init: failed to allocate physical page %d\n", (int)i);
		}
		uint32_t v = KHEAP_START + (i * PAGE_SIZE);
		if (vmm_map_page(v, (uint32_t)phys, PAGE_WRITE) != 0) {
			kpanic_fatal("kheap_init: failed to map page at %x\n", v);
		}
	}
	
	// Initialize the free list with the entire heap
	free_list = (block_header_t*)heap_base;
	free_list->size = KHEAP_SIZE - sizeof(block_header_t);
	free_list->free = 1;
	free_list->next = 0;
	
	kprintf("Kernel heap initialized: %x-%x (%d MB)\n", 
	        KHEAP_START, KHEAP_END, KHEAP_SIZE / (1024*1024));
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
	
	block_header_t *cur = free_list;
	while (cur) {
		if (cur->free && cur->size >= size) {
			split_block(cur, size);
			cur->free = 0;
			cur->magic = MAGIC_ALLOCATED;
			heap_used += size + sizeof(block_header_t);
			return (uint8_t*)cur + sizeof(block_header_t);
		}
		cur = cur->next;
	}
	
	// No suitable block found - heap is full
	kpanic_fatal("kmalloc: out of memory! Requested %d bytes, heap full\n", (int)size);
	return 0; // Never reached
}

void kfree(void *ptr)
{
	if (!ptr) return;
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	
	// Check for double free
    if (blk->magic == MAGIC_FREED) {
        kpanic_fatal("kfree: double free detected at %p\n", (void*)ptr);
        return;
    }
	
	// Check for invalid magic number
    if (blk->magic != MAGIC_ALLOCATED) {
        kpanic_fatal("kfree: invalid memory block at %x (magic: %x)\n", (uint32_t)ptr, blk->magic);
        return;
    }
	
	// Mark as freed
	blk->free = 1;
	blk->magic = MAGIC_FREED;
	heap_used -= blk->size + sizeof(block_header_t);
	
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
	
	// Check if block header alignment is at least 8 bytes (heap guarantees 8-byte alignment)
	if (block_addr & 7) {
		return 0; // Block header is not 8-byte aligned
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
		kprintf("[ERROR]ksize: invalid pointer %x (outside kernel heap)\n", ptr_addr);
		return 0; // Pointer is outside kernel heap region
	}
	
	block_header_t *blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	
	// Check if block is still allocated
	if (blk->magic != MAGIC_ALLOCATED) {
		kprintf("[ERROR] ksize: pointer %x refers to non-allocated block (magic=%x)\n", ptr_addr, blk->magic);
		return 0; // Block is freed or invalid
	}
	
	// Additional validation: check if block is properly allocated
	if (!is_valid_kheap_block(blk)) {
		kprintf("[ERROR] ksize: pointer %x fails allocation validation\n", ptr_addr);
		return 0; // Block is not properly allocated
	}
	
	return blk->size;
}

void *kbrk(void *new_brk)
{
	if (new_brk == 0) {
		// Return current break (end of used heap)
		return (void*)((uint32_t)heap_base + heap_used);
	}
	
	uint32_t new_addr = (uint32_t)new_brk;
	uint32_t heap_end = (uint32_t)heap_base + heap_size;
	
	// Check if new break is within heap bounds
	if (new_addr < (uint32_t)heap_base || new_addr > heap_end) {
		return (void*)-1; // Invalid break
	}
	
	// Update used heap size
	heap_used = new_addr - (uint32_t)heap_base;
	return new_brk;
}

uint32_t kheap_used_bytes(void)
{
	return heap_used;
}

uint32_t kheap_total_bytes(void)
{
	return heap_size;
}

