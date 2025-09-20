#include "paging.h"
#include "panic.h"
#include "kprintf.h"

// Simple identity-mapped page directory + tables for first 10MB
static uint32_t __attribute__((aligned(4096))) page_directory[1024];
static uint32_t __attribute__((aligned(4096))) page_tables[3][1024]; // 3 * 4MB = 12MB

static inline void load_cr3(uint32_t phys) { asm volatile("mov %0, %%cr3" : : "r"(phys) : "memory"); }
static inline uint32_t read_cr0(void) { uint32_t v; asm volatile("mov %%cr0, %0" : "=r"(v)); return v; }
static inline void write_cr0(uint32_t v) { asm volatile("mov %0, %%cr0" : : "r"(v) : "memory"); }

void paging_init(void)
{
	// 0x00000000 - 0x00BFFFFF: Virtual = Physical (identity mapped)
	// 0x00C000000+: Virtual ≠ Physical (non-identity mapped)
	// Zero PD
	for (int i = 0; i < 1024; i++) page_directory[i] = 0;
	// Identity-map first ~12MB using present|write
	// Map kernel code/data but not BIOS data area
	for (int t = 0; t < 3; t++) {
		for (int i = 0; i < 1024; i++) {
			uint32_t phys = (t * 1024 + i) * PAGE_SIZE;
			// Map everything for now - we'll handle BIOS protection in page fault handler
			page_tables[t][i] = (phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_WRITE;
		}
		page_directory[t] = ((uint32_t)page_tables[t]) | PAGE_PRESENT | PAGE_WRITE; // supervisor RW
	}
	// Kernel space/user space notion: addresses >= 0xC0000000 can later be kernel
	// For now, we only identity map low memory.
	load_cr3((uint32_t)page_directory);
	kprintf("Paging structures initialized.\n");
}

void paging_enable(void)
{
	uint32_t cr0 = read_cr0();
	cr0 |= 0x80000000u; // set PG bit
	write_cr0(cr0);
	kprintf("Paging enabled.\n");
}

uint32_t *virt_to_pte(uint32_t virt, int create)
{
	uint32_t pd_idx = (virt >> 22) & 0x3FF;
	uint32_t pt_idx = (virt >> 12) & 0x3FF;
	uint32_t pde = page_directory[pd_idx];
	if (!(pde & PAGE_PRESENT)) {
		if (!create) return NULL;
		// allocate a new table
		uint32_t *new_table = (uint32_t*)pmm_alloc_page();
		for (int i = 0; i < 1024; i++) new_table[i] = 0;
		page_directory[pd_idx] = ((uint32_t)new_table) | PAGE_PRESENT | PAGE_WRITE | ((virt >= USER_ZONE_START) ? PAGE_USER : 0);
		pde = page_directory[pd_idx];
	}
	uint32_t *pt = (uint32_t*)(pde & 0xFFFFF000);
	return &pt[pt_idx];
}

int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
	uint32_t *pte = virt_to_pte(virt, 1);
	if (!pte) return -1;
	*pte = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
	return 0;
}

void vmm_unmap_page(uint32_t virt)
{
	uint32_t *pte = virt_to_pte(virt, 0);
	if (!pte) return;
	*pte = 0;
}

uint32_t vmm_get_mapping(uint32_t virt)
{
	uint32_t *pte = virt_to_pte(virt, 0);
	if (!pte) return 0;
	return *pte;
}

// Page fault handler - handles permission violations and missing pages
void page_fault_handler(void)
{
	uint32_t fault_addr;
	uint32_t error_code;
	
	// Get fault address from CR2 register
	asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
	
	// Get error code from stack (pushed by CPU before our wrapper)
	// The error code is at offset 28 from ESP (7 registers * 4 bytes = 28)
	asm volatile("mov 28(%%esp), %0" : "=r"(error_code));
	
	// Check if trying to access BIOS addresses (0x00000000-0x000FFFFF)
	if (fault_addr < 0x00100000) {
		kpanic_fatal("Access to BIOS memory region denied\n");
	}
	
	// Check if it's a permission violation (user trying to access kernel space)
	if ((error_code & 0x4) && (fault_addr < USER_ZONE_START)) {
		kpanic_fatal("User access to kernel space denied\n");
	}
	
	// Check if it's a user space permission violation
	if ((error_code & 0x4) && (fault_addr >= USER_ZONE_START)) {
		uint32_t *pte = virt_to_pte(fault_addr, 0);
		if (pte && (*pte & PAGE_PRESENT)) {
			kpanic_fatal("User access to supervisor-only page denied\n");
		}
	}
	
	// All other page faults
	kpanic_fatal("Page fault\n");
}

void setup_page_fault_handler(void)
{
	// Set up page fault handler in IDT (interrupt 14)
	// This will be called by the interrupt system
	kprintf("Page fault handler registered (interrupt 14)\n");
}

