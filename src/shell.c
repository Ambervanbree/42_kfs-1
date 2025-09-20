#include "shell.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "pmm.h"
#include "kheap.h"
#include "paging.h"
#include "vmem.h"
#include "user_mem.h"
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
    {"umalloc", "Allocate user memory: umalloc <bytes>", cmd_umalloc},
    {"ufree", "Free user memory: ufree <addr>", cmd_ufree},
    {"usize", "Get user block size: usize <addr>", cmd_usize},
    {"present", "Map, unmap, then access to trigger not-present fault", cmd_present},
    {"pageops", "Test page creation and management", cmd_page_ops},
    {"allocfuncs", "Test allocation functions (kmalloc, kfree, ksize)", cmd_alloc_functions},
    {"ktest", "Allocate, write, verify, free: ktest <bytes> <value>", cmd_ktest},
    {"write", "Write int to any allocator addr: write <addr> <value> <zone>", cmd_write},
    {"read", "Read int from any allocator addr: read <addr> <zone>", cmd_read},
    {"rotest", "Test read-only page protection", cmd_rotest},
    {"pftest", "Test page fault handler by accessing invalid memory", cmd_pftest},
    {"pftest3", "Test page fault with simple unmapped access", cmd_pftest3},
    {"pftest4", "Test page fault by accessing NULL pointer", cmd_pftest4},
    {"biostest", "Test BIOS memory access protection", cmd_biostest},
    {"virmem", "Test virtual and physical memory functions", cmd_virtual_physical},
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
    
    kprintf("Available commands:\n");
    
    // Memory management commands
    kprintf("Memory Management:\n");
    kprintf("  meminfo     - Show memory stats\n");
    kprintf("  kbrk        - Physical memory break: kbrk [new_addr]\n");
    kprintf("  vbrk        - Virtual memory break: vbrk [new_addr]\n");
    kprintf("  vget        - Show mapping of a virtual addr: vget <virt>\n\n");
    
    // Kernel heap commands
    kprintf("Kernel Heap (kmalloc):\n");
    kprintf("  kmalloc     - Allocate kernel memory: kmalloc <bytes>\n");
    kprintf("  kfree       - Free kernel memory: kfree <addr>\n");
    kprintf("  ksize       - Get allocated block size: ksize <addr>\n");
    kprintf("  ktest       - Allocate, write, verify, free: ktest <bytes> <value>\n\n");
    
    // Virtual memory commands
    kprintf("Virtual Memory (vmalloc):\n");
    kprintf("  vmalloc vfree vsize\n");
    
    // User memory commands
    kprintf("User Memory (umalloc):\n");
    kprintf("  umalloc ufree usize\n");
    
    // Test commands
    kprintf("Memory Tests:\n");
    kprintf("  write       - Write int to any allocator addr: write <addr> <value>\n");
    kprintf("  read        - Read int from any allocator addr: read <addr>\n");
    kprintf("  pageops     - Test page creation and management\n");
    kprintf("  allocfuncs  - Test allocation functions (kmalloc, kfree, ksize)\n");
    kprintf("  rotest      - Test read-only page protection\n");
    kprintf("  virmem      - Test virtual and physical memory functions\n");
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

void cmd_meminfo(int argc, char **argv)
{
    (void)argc; (void)argv;
    kprintf("=== Memory Information ===\n");
    kprintf("PMM total pages: %d \n", (int)pmm_total_pages());
    kprintf("PMM free pages: %d \n", (int)pmm_free_pages());
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
    void *result = kbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("kbrk: invalid address %x\n", new_brk);
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
    void *result = vbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("vbrk: invalid address %x\n", new_brk);
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

// User space memory commands
void cmd_umalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: umalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = umalloc(n);
    kprintf("umalloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_ufree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ufree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    ufree((void*)a);
    kprintf("ufree(%x)\n", a);
}

void cmd_usize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: usize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = usize((void*)a);
    kprintf("usize(%x) -> %d\n", a, (int)s);
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
    kprintf("present test: map → unmap → fault\n");

    // Pick a test virtual address
    uint32_t virt = 0x40000000; // high user-ish/test area

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
    vmm_unmap_page(virt);
    pmm_free_page(phys);
    kprintf("unmapped\n");

    // Now deliberately access the unmapped address to trigger a not-present fault
    kprintf("about to access unmapped address; expect panic/page fault...\n");
    volatile uint32_t x = *(volatile uint32_t*)virt; // should fault
    (void)x;
}
void cmd_alloc_functions(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Testing allocation, free, and size functions...\n\n");
    
    // Test 0: Show initial memory stat
    cmd_meminfo(argc, argv);
    
    // Test 1: Kernel allocation functions
    void *kptr1 = kmalloc(100);
    kprintf("   kmalloc(100) = %x\n", (uint32_t)kptr1);
    size_t ksize1 = ksize(kptr1);
    kprintf("   ksize(%x) = %d bytes\n", (uint32_t)kptr1, ksize1);
    cmd_meminfo(argc, argv);
    kfree(kptr1);
    
    // Test 1.5: Show memory state after kernel allocations
    
    
    // Test 2: User allocation functions
    cmd_meminfo(argc, argv);
    void *uptr1 = umalloc(200);
    kprintf("   umalloc(200) = %x\n", (uint32_t)uptr1);
    size_t usize1 = usize(uptr1);
    kprintf("   usize(%x) = %d bytes\n", (uint32_t)uptr1, usize1);
    cmd_meminfo(argc, argv);
    ufree(uptr1);

    
    // Test 3: Virtual allocation functions
    cmd_meminfo(argc, argv);
    void *vptr1 = vmalloc(4096);
    kprintf("   vmalloc(4096) = %x\n", (uint32_t)vptr1); 
    size_t vsize1 = vsize(vptr1);
    kprintf("   vsize(%x) = %d bytes\n", (uint32_t)vptr1, vsize1);
    cmd_meminfo(argc, argv);
    vfree(vptr1);

    
    // Test 4: Force new page allocation

    kprintf(" Force New Page Allocation Test:\n");
    cmd_meminfo(argc, argv);
    void *large1 = kmalloc(5000);  // Should allocate new page
    kprintf("   kmalloc(5000) = %x\n", (uint32_t)large1);
    kprintf("     Memory State After Large Allocations:\n");
    cmd_meminfo(argc, argv);
    kfree(large1);

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

    // Write the value across the buffer as 32-bit words
    uint32_t *w = (uint32_t*)ptr;
    size_t words = nbytes / sizeof(uint32_t);
    for (size_t i = 0; i < words; i++) {
        w[i] = value;
    }
    // If trailing bytes exist, write them too byte-wise
    uint8_t *b = (uint8_t*)ptr;
    for (size_t i = words * sizeof(uint32_t); i < nbytes; i++) {
        b[i] = (uint8_t)(value & 0xFF);
    }

    // Print what we wrote (decimal) and read back to confirm
    kprintf("ktest: wrote value %d to %d bytes\n", (int)value, (int)nbytes);
    if (nbytes >= sizeof(uint32_t)) {
        kprintf("ktest: read back first int = %d\n", (int)w[0]);
        if (words > 1) {
            kprintf("ktest: read back last  int = %d\n", (int)w[words - 1]);
        }
    } else {
        kprintf("ktest: buffer smaller than 4 bytes, first byte = %d\n", (int)b[0]);
    }

    // Verify
    size_t errors = 0;
    for (size_t i = 0; i < words; i++) {
        if (w[i] != value) { errors++; break; }
    }
    for (size_t i = words * sizeof(uint32_t); i < nbytes && errors == 0; i++) {
        if (b[i] != (uint8_t)(value & 0xFF)) { errors++; break; }
    }

    size_t got = ksize(ptr);
    kprintf("ktest: ksize(%x) -> %d\n", (uint32_t)ptr, (int)got);
    if (errors == 0) {
        kprintf("ktest: verify OK\n");
    } else {
        kprintf("ktest: verify FAILED\n");
    }

    kfree(ptr);
    kprintf("ktest: kfree(%x)\n", (uint32_t)ptr);
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
    
    // Kernel heap range: 0x01000000 - 0x02000000 (16MB)
    if (addr >= 0x01000000 && addr < 0x02000000) {
        alloc_size = ksize((void*)addr);
        alloc_type = "kmalloc";
    }
    // Virtual memory range: 0x02000000 - 0x03000000 (16MB) 
    else if (addr >= 0x02000000 && addr < 0x03000000) {
        alloc_size = vsize((void*)addr);
        alloc_type = "vmalloc";
    }
    // User heap range: 0x08000000 - 0x09000000 (16MB)
    else if (addr >= 0x08000000 && addr < 0x09000000) {
        alloc_size = usize((void*)addr);
        alloc_type = "umalloc";
    }
    
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
    
    // Kernel heap range: 0x01000000 - 0x02000000 (16MB)
    if (addr >= 0x01000000 && addr < 0x02000000) {
        alloc_size = ksize((void*)addr);
        alloc_type = "kmalloc";
    }
    // Virtual memory range: 0x02000000 - 0x03000000 (16MB) 
    else if (addr >= 0x02000000 && addr < 0x03000000) {
        alloc_size = vsize((void*)addr);
        alloc_type = "vmalloc";
    }
    // User heap range: 0x08000000 - 0x09000000 (16MB)
    else if (addr >= 0x08000000 && addr < 0x09000000) {
        alloc_size = usize((void*)addr);
        alloc_type = "umalloc";
    }
    
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

// Test page fault handler by accessing kernel space (simulating user mode)
void cmd_pftest2(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== Page Fault Handler Test 2 ===\n");
    kprintf("Testing kernel space access protection...\n\n");
    
    // Test 2: Access kernel space (should trigger protection violation)
    kprintf("Test 2: Accessing kernel space (0xC0000000+)\n");
    kprintf("pftest2: About to access 0xC0000000 (kernel space)...\n");
    kprintf("pftest2: This should trigger a protection violation!\n");
    
    // This should trigger a page fault with protection violation
    volatile uint32_t *kernel_space = (volatile uint32_t*)0xC0000000;
    uint32_t value = *kernel_space;  // This will cause a page fault
    (void)value;  // Suppress unused variable warning
    
    // If we get here, something went wrong
    kprintf("pftest2: ERROR - Page fault handler not working!\n");
}

// Simple page fault test - just access unmapped memory
void cmd_pftest3(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== Simple Page Fault Test ===\n");
    kprintf("pftest3: About to access unmapped memory at 0x99999999\n");
    kprintf("pftest3: This should trigger a page fault!\n");
    kprintf("pftest3: If you see this message after the access, the handler isn't working.\n");
    
    // This should definitely cause a page fault
    volatile uint32_t *ptr = (volatile uint32_t*)0x99999999;
    uint32_t value = *ptr;  // This will cause a page fault
    (void)value;
    
    // If we get here, something is wrong
    kprintf("pftest3: ERROR - Page fault handler not working!\n");
    kprintf("pftest3: The access succeeded when it should have failed!\n");
}

// Test page fault with NULL pointer access
void cmd_pftest4(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== NULL Pointer Page Fault Test ===\n");
    kprintf("pftest4: About to access NULL pointer (0x00000000)\n");
    kprintf("pftest4: This should trigger a page fault!\n");
    
    // This should cause a page fault (accessing address 0)
    volatile uint32_t *ptr = (volatile uint32_t*)0x00000000;
    uint32_t value = *ptr;  // This will cause a page fault
    (void)value;
    
    // If we get here, something is wrong
    kprintf("pftest4: ERROR - Page fault handler not working!\n");
}

// Test BIOS memory access protection
void cmd_biostest(int argc, char **argv)
{
    (void)argc; (void)argv;
    
    kprintf("=== BIOS Memory Access Protection Test ===\n");
    kprintf("biostest: Testing access to BIOS memory regions...\n\n");
    
    kprintf("biostest: BIOS memory region: 0x00000000 - 0x000FFFFF (1MB)\n");
    kprintf("biostest: This region contains:\n");
    kprintf("biostest: - Interrupt Vector Table (0x0000-0x03FF)\n");
    kprintf("biostest: - BIOS Data Area (0x0400-0x04FF)\n");
    kprintf("biostest: - BIOS Code (0xF0000-0xFFFFF)\n");
    kprintf("biostest: - Video Memory (0xA0000-0xBFFFF)\n\n");
    
    kprintf("biostest: This should trigger a page fault!\n\n");
    
    // Test different BIOS addresses
    //volatile uint32_t *bios_ivt = (volatile uint32_t*)0x00000000;  // Interrupt Vector Table
    volatile uint32_t *bios_data = (volatile uint32_t*)0x00000400; // BIOS Data Area
    //volatile uint32_t *bios_code = (volatile uint32_t*)0x000F0000; // BIOS Code
    
    kprintf("biostest: Accessing Interrupt Vector Table (0x00000000)...\n");
    uint32_t value = *bios_data;  // This should cause page fault
    (void)value;
    
    kprintf("biostest: ERROR - Page fault handler not working!\n");
    kprintf("biostest: BIOS access should have been blocked!\n");
}

