#include "shell.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "pmm.h"
#include "kheap.h"
#include "paging.h"
#include "vmem.h"
#include "panic.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Shell state
static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_pos = 0;
static uint32_t boot_time = 0;

// Command table
static struct shell_command commands[] = {
    {"help", "Display this help message", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"echo", "Echo arguments to screen", cmd_echo},
    {"reboot", "Restart the system", cmd_reboot},
    {"halt", "Stop CPU (requires manual restart)", cmd_halt},
    {"gdt", "Display GDT information", cmd_gdt_info},
    {"version", "Display kernel version", cmd_version},
    {"shutdown", "Shutdown system gracefully", cmd_shutdown},
    {"meminfo", "Show memory stats", cmd_meminfo},
    {"kmalloc", "Allocate kernel memory: kmalloc <bytes>", cmd_kmalloc},
    {"kfree", "Free kernel memory: kfree <addr>", cmd_kfree},
    {"ksize", "Get allocated block size: ksize <addr>", cmd_ksize},
    {"kbrk", "Physical memory break: kbrk [new_addr]", cmd_kbrk},
    {"vmalloc", "Allocate virtual memory: vmalloc <bytes>", cmd_vmalloc},
    {"vfree", "Free virtual memory: vfree <addr>", cmd_vfree},
    {"vsize", "Get virtual block size: vsize <addr>", cmd_vsize},
    {"vbrk", "Virtual memory break: vbrk [new_addr]", cmd_vbrk},
    {"vget", "Show mapping of a virtual addr: vget <virt>", cmd_vget},
    {"present", "Map, unmap, then access to trigger not-present fault", cmd_present},
    {"pageops", "Test page creation and management", cmd_page_ops},
    {"kmalloctest", "Test allocation functions (kmalloc, kfree, ksize)", cmd_kmalloc_test},
    {"vmalloctest", "Test allocation functions (vmalloc, vfree, vsize)", cmd_vmalloc_test},
    {"ktest", "Allocate, write, verify, free: ktest <bytes> <value>", cmd_ktest},
    {"vtest", "Allocate, write, verify, free: vtest <bytes> <value>", cmd_vtest},
    {"write", "Write int to any allocator addr: write <addr> <value>", cmd_write},
    {"read", "Read int from any allocator addr: read <addr>", cmd_read},
    {"rotest", "Test read-only page protection", cmd_rotest},
    {"pftest", "Test page fault handler by accessing invalid memory", cmd_pftest},
    {"pftest2", "Simple page fault test - access unmapped memory", cmd_pftest2},
    {"panictest", "Test kernel panic handling", cmd_panic_test},
    {NULL, NULL, NULL} // Sentinel
};

void shell_init(void)
{
    boot_time = 0; // We'll implement a timer later
    shell_buffer_pos = 0;
    memset(shell_buffer, 0, SHELL_BUFFER_SIZE);
    
    kprintf("KFS Debug Shell v1.0\n");
    kprintf("Type 'help' for available commands.\n\n");
    shell_print_prompt();
}

void shell_print_prompt(void)
{
    kprintf(SHELL_PROMPT);
    // Set input start position to current cursor position (after prompt)
    input_set_start_position();
}

void shell_process_input(const char *input)
{
    size_t len = strlen(input);
    
    if (len == 0) {
        shell_print_prompt();
        return;
    }
    
    // Copy input to buffer for processing
    if (len < SHELL_BUFFER_SIZE - 1) {
        strcpy(shell_buffer, input);
        shell_execute_command(shell_buffer);
    } else {
        kprintf("Command too long!\n");
    }
    
    shell_print_prompt();
}

void shell_execute_command(const char *command_line)
{
    char *argv[SHELL_MAX_ARGS];
    int argc;
    
    shell_parse_args(command_line, argv, &argc);
    
    if (argc == 0) {
        return;
    }
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            kprintf("Executing command: %s\n", commands[i].name);
            commands[i].function(argc, argv);
            return;
        }
    }
    
    kprintf("Unknown command: %s\n", argv[0]);
    kprintf("Type 'help' for available commands.\n");
}

void shell_parse_args(const char *input, char **argv, int *argc)
{
    static char args_buffer[SHELL_BUFFER_SIZE];
    strcpy(args_buffer, input);
    
    *argc = 0;
    char *token = args_buffer;
    char *end = args_buffer + strlen(args_buffer);
    
    while (token < end && *argc < SHELL_MAX_ARGS - 1) {
        // Skip leading spaces
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        
        if (*token == '\0') {
            break;
        }
        
        argv[*argc] = token;
        (*argc)++;
        
        // Find end of current argument
        while (*token != ' ' && *token != '\t' && *token != '\0') {
            token++;
        }
        
        if (*token != '\0') {
            *token = '\0';
            token++;
        }
    }
    
    argv[*argc] = NULL;
}

// Built-in commands implementation

void cmd_help(int argc, char **argv)
{
    (void)argc; // Suppress unused parameter warning
    (void)argv;
    
    // System commands
    kprintf("System commands:\n");
    kprintf("  help clear echo reboot halt shutdown version gdt\n");

    // Kernel heap commands
    kprintf("Kernel Heap commands (kmalloc):\n");
    kprintf("  kmalloc     - Allocate kernel memory: kmalloc <bytes>\n");
    kprintf("  kfree       - Free kernel memory: kfree <addr>\n");
    kprintf("  ksize       - Get allocated block size: ksize <addr>\n");
    kprintf("  ktest       - Allocate, write, verify, free: ktest <bytes> <value>\n");
    kprintf("  kbrk        - Physical memory break: kbrk [new_addr]\n");
    
    // Virtual memory commands
    kprintf("Virtual Memory commands (vmalloc):\n");
    kprintf("  vmalloc vfree vsize vbrk\n");
    kprintf("  vget        - Show mapping of a virtual addr: vget <virt>\n");
    
    // Test commands
    kprintf("Memory Tests:\n");
    kprintf("  meminfo     - Show memory stats\n");
    kprintf("  present     - Map, unmap, then access to trigger not-present fault\n");
    kprintf("  pageops     - Test page creation and management\n");
    kprintf("  kmalloctest - Test allocation functions (kmalloc, kfree, ksize)\n");
    kprintf("  vmalloctest - Test allocation functions (vmalloc, vfree, vsize)\n");
    kprintf("  write       - Write int to any allocator addr: write <addr> <value>\n");
    kprintf("  read        - Read int from any allocator addr: read <addr>\n");
    kprintf("  rotest      - Test read-only page protection\n");
    kprintf("  pftest      - Test page fault handler by accessing invalid memory\n");
    kprintf("  panictest   - Test kernel panic handling\n");
}

void cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    screen_clear();
}

void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        kprintf("%s", argv[i]);
        if (i < argc - 1) {
            kprintf(" ");
        }
    }
    kprintf("\n");
}

void cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Rebooting system...\n");
    kprintf("Trying multiple reboot methods...\n");
    
    // Method 1: Use keyboard controller to reboot (most common)
    kprintf("Method 1: Keyboard controller reset...\n");
    outb(0x64, 0xFE);
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 2: Try ACPI reset (if available)
    kprintf("Method 2: ACPI reset...\n");
    outb(0xCF9, 0x06);
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 3: Triple fault (force CPU reset)
    kprintf("Method 3: Triple fault...\n");
    asm volatile("cli");           // Disable interrupts
    asm volatile("lidt %0" : : "m"((struct {uint16_t limit; uint32_t base;}){0, 0})); // Load invalid IDT
    asm volatile("int $0x00");     // Trigger interrupt with invalid IDT
    
    // If we get here, nothing worked
    kprintf("All reboot methods failed. Please use:\n");
    kprintf("- Physical reset button on computer case\n");
    kprintf("- VM reset function (Ctrl+R in QEMU, or VM menu)\n");
    kprintf("- Power cycle the machine\n");
}

void cmd_halt(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("System halted. To restart:\n");
    kprintf("- Physical computer: Press reset button on case\n");
    kprintf("- QEMU: Ctrl+A then X, or close window\n");
    kprintf("- VirtualBox: Machine menu -> Reset\n");
    kprintf("- VMware: VM menu -> Power -> Reset\n");
    kprintf("\nSystem is now halted...\n");
    
    // Disable interrupts and halt
    asm volatile("cli; hlt");
    
    // In case we somehow continue, infinite loop
    while (1) {
        asm volatile("hlt");
    }
}

void cmd_gdt_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Global Descriptor Table Information:\n");
    kprintf("  GDT Base Address: %x\n", 0x00000800);
    kprintf("  Segment Layout:\n");
    kprintf("    0x00: Null Segment\n");
    kprintf("    0x08: Kernel Code Segment (Ring 0)\n");
    kprintf("    0x10: Kernel Data Segment (Ring 0)\n");
    kprintf("    0x18: Kernel Stack Segment (Ring 0)\n");
    kprintf("    0x20: User Code Segment (Ring 3)\n");
    kprintf("    0x28: User Data Segment (Ring 3)\n");
    kprintf("    0x30: User Stack Segment (Ring 3)\n");
    
    // Display current segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%es, %0" : "=r"(es));
    asm volatile("mov %%fs, %0" : "=r"(fs));
    asm volatile("mov %%gs, %0" : "=r"(gs));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    
    kprintf("  Current Segment Registers:\n");
    kprintf("    CS: %x, DS: %x, ES: %x\n", cs, ds, es);
    kprintf("    FS: %x, GS: %x, SS: %x\n", fs, gs, ss);
}

void cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("KFS (Kernel From Scratch) v1.0\n");
    kprintf("Built: %s %s\n", __DATE__, __TIME__);
    kprintf("Architecture: i386 (32-bit)\n");
    kprintf("Features: GDT, Interrupts, Keyboard, VGA Text Mode, Shell\n");
}

void cmd_shutdown(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Shutting down system gracefully...\n");
    kprintf("This will try to power off the system.\n\n");
    
    // Method 1: ACPI shutdown (works in most VMs)
    kprintf("Attempting ACPI shutdown...\n");
    // QEMU ACPI shutdown - write 16-bit value to port
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 2: APM shutdown (older systems)
    kprintf("Attempting APM shutdown...\n");
    asm volatile(
        "mov $0x5307, %%ax\n\t"  // APM function: set power state
        "mov $0x0001, %%bx\n\t"  // Device: all devices  
        "mov $0x0003, %%cx\n\t"  // Power state: off
        "int $0x15"              // APM BIOS interrupt
        :
        :
        : "ax", "bx", "cx"
    );
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 3: Try alternative ACPI methods
    kprintf("Trying alternative shutdown methods...\n");
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x3400), "Nd"((uint16_t)0x4004));
    
    // If we get here, shutdown failed
    kprintf("\nShutdown failed. The system is still running.\n");
    kprintf("You can:\n");
    kprintf("- Use 'halt' to stop the CPU (requires restart)\n");
    kprintf("- Use 'reboot' to restart the system\n");
    kprintf("- Close the VM window manually\n");
    kprintf("- Power off physical machine manually\n");
}

static uint32_t parse_hex_or_dec(const char *s)
{
    uint32_t val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++;
            uint32_t n;
            if (c >= '0' && c <= '9') n = c - '0';
            else if (c >= 'a' && c <= 'f') n = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') n = 10 + (c - 'A');
            else break;
            val = (val << 4) | n;
        }
        return val;
    }
    while (*s && *s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

void cmd_meminfo(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("=== Memory Information ===\n");
    cmd_pmminfo(0, NULL);

    kprintf("\nAllocator Regions:\n");
    kprintf("  kmalloc: %x - %x (64MB) - Physical memory\n", KHEAP_START, KHEAP_END);
    kprintf("  vmalloc: %x - %x (32MB) - Kernel virtual memory\n", KVMEM_START, KVMEM_END);
    kprintf("  vmalloc: %x - %x (64MB) - User virtual memory\n", VMEM_START, VMEM_END);
    
}

void cmd_pmminfo(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    // Get PMM information
    uint32_t total_pages = pmm_total_pages();
    uint32_t free_pages = pmm_free_pages();
    
    kprintf("Physical Memory Manager (PMM):\n");
    kprintf("  Total pages: %d (%d MB)\n", total_pages, (total_pages * PAGE_SIZE) / (1024 * 1024));
    kprintf("  Free pages: %d (%d MB)\n", free_pages, (free_pages * PAGE_SIZE) / (1024 * 1024));
}

void cmd_kmalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kmalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = kmalloc(n);
    kprintf("kmalloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_kfree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kfree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    kfree((void*)a);
    kprintf("kfree(%x)\n", a);
}

void cmd_ksize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ksize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = ksize((void*)a);
    kprintf("ksize(%x) -> %d\n", a, (int)s);
}

void cmd_kbrk(int argc, char **argv)
{
    if (argc < 2) {
        void *brk = kbrk(0);
        kprintf("kbrk: %x\n", (uint32_t)brk);
        return;
    }
    uint32_t new_brk = parse_hex_or_dec(argv[1]);
    if (new_brk == 0) {
        kprintf("[ERROR] vbrk: invalid address %x\n", new_brk);
        return;
    }
    void *result = kbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("[ERROR] kbrk: invalid address %x\n", new_brk);
    } else {
        kprintf("kbrk: %x\n", (uint32_t)result);
    }
}

void cmd_vmalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vmalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = vmalloc(n);
    kprintf("vmalloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_vfree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vfree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    vfree((void*)a);
    kprintf("vfree(%x)\n", a);
}

void cmd_vsize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vsize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = vsize((void*)a);
    kprintf("vsize(%x) -> %d\n", a, (int)s);
}

void cmd_vbrk(int argc, char **argv)
{
    if (argc < 2) {
        void *brk = vbrk(0);
        kprintf("vbrk: %x\n", (uint32_t)brk);
        return;
    }
    uint32_t new_brk = parse_hex_or_dec(argv[1]);
    if (new_brk == 0) {
        kprintf("[ERROR] vbrk: address %x not allocatable\n", new_brk);
        return;
    }
    void *result = vbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("[ERROR] vbrk: address %x not allocatable\n", new_brk);
    } else {
        kprintf("vbrk: %x\n", (uint32_t)result);
    }
}

void cmd_vget(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vget <virt>\n"); return; }
    uint32_t virt = parse_hex_or_dec(argv[1]);
    uint32_t pte = vmm_get_mapping(virt);
    uint32_t pd_idx = (virt >> 22) & 0x3FF;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    uint32_t offset = virt & 0xFFF;
    kprintf("logical: %x (flat seg)\n", virt);
    kprintf("virtual: %x  pd=%d pt=%d off=%d\n", virt, (int)pd_idx, (int)pt_idx, (int)offset);
    if (pte == 0) {
        kprintf("mapping: (not present)\n");
        return;
    }
    uint32_t phys = (pte & 0xFFFFF000) | offset;
    uint32_t flags = pte & 0xFFF;
    kprintf("physical: %x  flags: %x\n", phys, flags);
}



void cmd_page_ops(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Page ops test...\n");
    // Single happy-path exercise: alloc → map → RW test → get → unmap → free
    void *phys = pmm_alloc_page();
    if (!phys) {
        kprintf("alloc failed\n");
        return;
    }

    uint32_t virt = 0x10000000;
    if (vmm_map_page(virt, (uint32_t)phys, PAGE_WRITE) != 0) {
        kprintf("map failed\n");
        pmm_free_page(phys);
        return;
    }

    uint32_t *ptr = (uint32_t*)virt;
    *ptr = 0x12345678;
    kprintf("map %x -> %x, rw=%s\n", virt, (uint32_t)phys, (*ptr == 0x12345678) ? "ok" : "bad");

    uint32_t got = vmm_get_mapping(virt);
    kprintf("get %x => %x\n", virt, got);

    vmm_unmap_page(virt);
    uint32_t after = vmm_get_mapping(virt);
    kprintf("unmap ok=%s\n", (after == 0) ? "yes" : "no");

    pmm_free_page(phys);
}

void cmd_present(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("present test: 1. map 2. unmap 3. fault\n");

    // Pick a test virtual address - use an address that's definitely unmapped
    uint32_t virt = 0x50000000; // This should be unmapped (outside first 12MB)

    // Allocate and map one page
    void *phys = pmm_alloc_page();
    if (!phys) {
        kprintf("alloc failed\n");
        return;
    }
    if (vmm_map_page(virt, (uint32_t)phys, PAGE_WRITE | PAGE_USER) != 0) {
        kprintf("map failed\n");
        pmm_free_page(phys);
        return;
    }

    // Touch it to ensure mapping works
    volatile uint32_t *p = (uint32_t*)virt;
    *p = 0xCAFEBABE;
    kprintf("mapped and wrote ok\n");

    // Unmap and free
    kprintf("unmapping page at %x...\n", virt);
    vmm_unmap_page(virt);
    pmm_free_page(phys);
    kprintf("unmapped\n");

    // Flush TLB to ensure cached translation is cleared
    kprintf("flushing TLB...\n");
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax");

    // Check if the page is really unmapped
    uint32_t mapping = vmm_get_mapping(virt);
    kprintf("mapping check: %x -> %x (should be 0)\n", virt, mapping);

    // Now deliberately access the unmapped address to trigger a not-present fault
    kprintf("about to access unmapped address; expect panic/page fault...\n");
    volatile uint32_t x = *(volatile uint32_t*)virt; // should fault
    (void)x;
}
void cmd_kmalloc_test(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{   
    // Show initial memory stat
    cmd_pmminfo(argc, argv);
    
    // Test 1: Pages are equal size after small allocation
    void *kptr1 = kmalloc(100);
    kprintf("\n  kmalloc(100) = %x\n", (uint32_t)kptr1);
    size_t ksize1 = ksize(kptr1);
    kprintf("  ksize(%x) = %d bytes\n\n", (uint32_t)kptr1, ksize1);
    cmd_pmminfo(argc, argv);
    kfree(kptr1);
    
    // Test 2: Pages are equal size after big allocation
    void *large1 = kmalloc(5000);
    kprintf("\n  kmalloc(5000) = %x\n", (uint32_t)large1);
    size_t ksize2 = ksize(large1);
    kprintf("  ksize(%x) = %d bytes\n\n", (uint32_t)large1, ksize2);
    cmd_pmminfo(argc, argv);
    kfree(large1);
}

void cmd_vmalloc_test(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{   
    // Show initial memory stat
    cmd_pmminfo(argc, argv);
    
    // Small virtual allocation
    void *vptr1 = vmalloc(200);
    kprintf("\n  vmalloc(200) = %x\n", (uint32_t)vptr1);
    size_t vsize1 = vsize(vptr1);
    kprintf("  vsize(%x) = %d bytes\n\n", (uint32_t)vptr1, vsize1);
    cmd_pmminfo(argc, argv);
    vfree(vptr1);

    
    // Virtual allocation of multiple pages
    void *vptr2 = vmalloc(5000);
    kprintf("\n  vmalloc(5000) = %x\n", (uint32_t)vptr2); 
    size_t vsize2 = vsize(vptr2);
    kprintf("  vsize(%x) = %d bytes\n\n", (uint32_t)vptr2, vsize2);
    cmd_pmminfo(argc, argv);
    vfree(vptr2);
}

void cmd_virtual_physical(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Testing virtual and physical memory functions...\n\n");
    
    // Test 1: Physical memory management
    kprintf("1. Physical Memory Management:\n");
    void *phys1 = pmm_alloc_page();
    void *phys2 = pmm_alloc_page();
    kprintf("   pmm_alloc_page() = %x\n", (uint32_t)phys1);
    kprintf("   pmm_alloc_page() = %x\n", (uint32_t)phys2);
    
    pmm_free_page(phys1);
    pmm_free_page(phys2);

    // Test 2: Virtual memory management
    kprintf("2. Virtual Memory Management:\n");
    void *virt1 = vmalloc(4096);
    void *virt2 = vmalloc(8192);
    kprintf("   vmalloc(4096) = %x\n", (uint32_t)virt1);
    kprintf("   vmalloc(8192) = %x\n", (uint32_t)virt2);
    
    vfree(virt1);
    vfree(virt2);
    
    // Test 3: Memory mapping
    kprintf("3. Memory Mapping:\n");
    uint32_t virt_addr = 0x20000000;
    void *phys_page = pmm_alloc_page();
    
    int map_result = vmm_map_page(virt_addr, (uint32_t)phys_page, PAGE_WRITE | PAGE_USER);
    kprintf("   vmm_map_page(%x, %x) = %d\n", virt_addr, (uint32_t)phys_page, map_result);
    
    uint32_t mapping = vmm_get_mapping(virt_addr);
    kprintf("   vmm_get_mapping(%x) = %x\n", virt_addr, mapping);
    
    vmm_unmap_page(virt_addr);
    pmm_free_page(phys_page);

    // Test 4: Memory break functions
    kprintf("4. Memory Break Functions:\n");
    void *kbrk_current = kbrk(0);
    kprintf("   kbrk() = %x\n", (uint32_t)kbrk_current);
    
    void *vbrk_current = vbrk(0);
    kprintf("   vbrk() = %x\n", (uint32_t)vbrk_current);
    kprintf("\n");
    
}

void cmd_panic_test(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    // Test 1: Fatal Panic Test - Out of Memory
    kprintf("Fatal Panic Test - Out of Memory:\n");
    kprintf("   Testing kpanic_fatal() on out of memory...\n");

    void *huge_alloc = kmalloc(50 * 1024 * 1024); 
    (void)huge_alloc; // This will call kpanic_fatal("PMM out of memory")
}

// Simple end-to-end kernel heap test: allocate, write, verify, free
void cmd_ktest(int argc, char **argv)
{
    if (argc < 3) {
        kprintf("Usage: ktest <bytes> <value>\n");
        return;
    }
    uint32_t nbytes = parse_hex_or_dec(argv[1]);
    uint32_t value = parse_hex_or_dec(argv[2]);

    void *ptr = kmalloc(nbytes);
    if (!ptr) {
        kprintf("ktest: kmalloc(%d) failed\n", (int)nbytes);
        return;
    }
    kprintf("ktest: kmalloc(%d) -> %x\n", (int)nbytes, (uint32_t)ptr);

    // Write value using existing write function
    kprintf("ktest: writing value %d to %d bytes\n", (int)value, (int)nbytes);
    int *p = (int*)ptr;
    *p = (int)value;
    
    // Read back using existing read function logic
    kprintf("ktest: read back first int = %d\n", *p);

    size_t got = ksize(ptr);
    kprintf("ktest: ksize(%x) -> %d\n", (uint32_t)ptr, (int)got);
    kprintf("ktest: verify OK\n");

    kfree(ptr);
    kprintf("ktest: kfree(%x)\n", (uint32_t)ptr);
}

// Simple end-to-end virtual memory test: allocate, write, verify, free
void cmd_vtest(int argc, char **argv)
{
    if (argc < 3) {
        kprintf("Usage: vtest <bytes> <value>\n");
        return;
    }
    uint32_t nbytes = parse_hex_or_dec(argv[1]);
    uint32_t value = parse_hex_or_dec(argv[2]);

    void *ptr = vmalloc(nbytes);
    if (!ptr) {
        kprintf("vtest: vmalloc(%d) failed\n", (int)nbytes);
        return;
    }
    kprintf("vtest: vmalloc(%d) -> %x\n", (int)nbytes, (uint32_t)ptr);

    // Write value using simple approach
    kprintf("vtest: writing value %d to %d bytes\n", (int)value, (int)nbytes);
    int *p = (int*)ptr;
    *p = (int)value;
    
    // Read back to verify
    kprintf("vtest: read back first int = %d\n", *p);

    size_t got = vsize(ptr);
    kprintf("vtest: vsize(%x) -> %d\n", (uint32_t)ptr, (int)got);
    kprintf("vtest: verify OK\n");

    vfree(ptr);
    kprintf("vtest: vfree(%x)\n", (uint32_t)ptr);
}

// Generic write command that works with any allocator
void cmd_write(int argc, char **argv)
{
    if (argc < 3) {
        kprintf("Usage: write <addr> <value>\n");
        return;
    }
    uint32_t addr = parse_hex_or_dec(argv[1]);
    int val = (int)parse_hex_or_dec(argv[2]);

    // Reject values outside 32-bit signed range
    if ((uint32_t)val > 0x7FFFFFFF) {
        kprintf("write: value too big for int: %x\n", (uint32_t)val);
        return;
    }
    if ( addr < 0x00100000) {
        kpanic_fatal("write: attempt to write to BIOS memory at %p\n", (void*)addr);
        return;
    }

    // Check address range and call appropriate size function
    size_t alloc_size = 0;
    const char *alloc_type = "unknown";
    
    // Kernel heap range: 0x04000000 - 0x07FFFFFF (64MB)
    if (addr >= KHEAP_START && addr < KHEAP_END) {
        alloc_size = ksize((void*)addr);
        alloc_type = "kmalloc";
    }
    // Kernel vmalloc range: 0x02000000 - 0x03FFFFFF (32MB) 
    else if (addr >= KVMEM_START && addr < KVMEM_END) {
        alloc_size = vsize((void*)addr);
        alloc_type = "kvmalloc";
    }
    // User vmalloc range: 0x40000000 - 0x43FFFFFF (64MB)
    else if (addr >= VMEM_START && addr < VMEM_END) {
        alloc_size = vsize((void*)addr);
        alloc_type = "vmalloc";
    }
    // Removed umalloc - using vmalloc for user space
    
    if (alloc_size > 0) {
        // Writing 4 bytes to a valid allocation
        if (4 > alloc_size) {
            kpanic_fatal("write: buffer overflow detected! Writing 4 bytes to %s allocation of %d bytes at %p\n", 
                        alloc_type, (int)alloc_size, (void*)addr);
            return;
        }
        kprintf("write: writing to %s allocation (%d bytes) at %p\n", alloc_type, (int)alloc_size, addr);
    } else {
        kprintf("write: WARNING - address %p not recognized as valid allocation\n", (void*)addr);
        return;
    }

    int *p = (int*)addr;
    *p = val;
    kprintf("write: *(int*)%x = %d\n", addr, val);
}

// Generic read command that works with any allocator
void cmd_read(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("Usage: read <addr>\n");
        return;
    }
    uint32_t addr = parse_hex_or_dec(argv[1]);

    // Note: allocators guarantee proper alignment for user data

    // Check address range and call appropriate size function
    size_t alloc_size = 0;
    const char *alloc_type = "unknown";
    
    // Kernel heap range: 0x04000000 - 0x07FFFFFF (64MB)
    if (addr >= KHEAP_START && addr < KHEAP_END) {
        alloc_size = ksize((void*)addr);
        alloc_type = "kmalloc";
    }
    // Kernel vmalloc range: 0x02000000 - 0x03FFFFFF (32MB) 
    else if (addr >= KVMEM_START && addr < KVMEM_END) {
        alloc_size = vsize((void*)addr);
        alloc_type = "kvmalloc";
    }
    // User vmalloc range: 0x40000000 - 0x43FFFFFF (64MB)
    else if (addr >= VMEM_START && addr < VMEM_END) {
        alloc_size = vsize((void*)addr);
        alloc_type = "vmalloc";
    }
    // Removed umalloc - using vmalloc for user space
    
    if (alloc_size > 0) {
        // Reading 4 bytes from a valid allocation
        if (4 > alloc_size) {
            kpanic_fatal("read: buffer overflow detected! Reading 4 bytes from %s allocation of %d bytes at %p\n", 
                        alloc_type, (int)alloc_size, (void*)addr);
            return;
        }
        kprintf("read: reading from %s allocation (%d bytes) at %p\n", alloc_type, (int)alloc_size, addr);
    } else {
        kprintf("read: WARNING - address %p not recognized as valid allocation\n", (void*)addr);
        return;
    }

    int *p = (int*)addr;
    kprintf("read: *(int*)%x = %d\n", addr, *p);
}

// Test read-only page protection by mapping a page as read-only and trying to write
void cmd_rotest(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("Testing read-only page protection...\n");
    
    // Allocate a physical page
    void *phys_page = pmm_alloc_page();
    if (!phys_page) {
        kprintf("rotest: failed to allocate physical page\n");
        return;
    }
    kprintf("rotest: allocated physical page at %x\n", (uint32_t)phys_page);
    
    // Map it as read-only (PAGE_PRESENT only, no PAGE_WRITE)
    uint32_t virt_addr = 0x50000000;  // Use a high virtual address
    int result = vmm_map_page(virt_addr, (uint32_t)phys_page, PAGE_PRESENT);
    if (result != 0) {
        kprintf("rotest: failed to map read-only page\n");
        pmm_free_page(phys_page);
        return;
    }
    kprintf("rotest: mapped read-only page at virtual %x\n", virt_addr);
    
    // Try to read from it (should work)
    uint32_t *ptr = (uint32_t*)virt_addr;
    uint32_t original_value = *ptr;
    kprintf("rotest: read from page: %x\n", original_value);
    
    // Try to write to it (should trigger page fault)
    kprintf("rotest: attempting to write to read-only page...\n");
    kprintf("rotest: WARNING - this may cause a page fault!\n");
    
    // This write should trigger a page fault
    *ptr = 0xDEADBEEF;
    
    // If we get here, the write succeeded (protection not working)
    kprintf("rotest: write succeeded - protection not working!\n");
    kprintf("rotest: new value: %x\n", *ptr);
    
    // Clean up
    vmm_unmap_page(virt_addr);
    pmm_free_page(phys_page);
    kprintf("rotest: cleaned up\n");
}

// Test page fault handler by deliberately accessing invalid memory
void cmd_pftest(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== Page Fault Handler Test ===\n");
    kprintf("Testing page fault detection and handling...\n\n");
    
    // Test 1: Access unmapped memory (should trigger not-present fault)
    kprintf("Test 1: Accessing unmapped memory\n");
    kprintf("pftest: About to access 0x12345678 (unmapped)...\n");
    kprintf("pftest: This should trigger a page fault!\n");
    
    // This should trigger a page fault
    volatile uint32_t *unmapped = (volatile uint32_t*)0x12345678;
    uint32_t value = *unmapped;  // This will cause a page fault
    (void)value;  // Suppress unused variable warning
    
    // If we get here, something went wrong
    kprintf("pftest: ERROR - Page fault handler not working!\n");
}

// Simple page fault test - just access unmapped memory
void cmd_pftest2(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== Simple Page Fault Test ===\n");
    kprintf("pftest2: About to access unmapped memory at 0x20000000\n");
    kprintf("pftest2: This should trigger a page fault!\n");
    kprintf("pftest2: If you see this message after the access, the handler isn't working.\n");
    
    // This should definitely cause a page fault
    volatile uint32_t *ptr = (volatile uint32_t*)0x20000000;
    uint32_t value = *ptr;  // This will cause a page fault
    (void)value;
    
    // If we get here, something is wrong
    kprintf("pftest2: ERROR - Page fault handler not working!\n");
    kprintf("pftest2: The access succeeded when it should have failed!\n");
}

