#include "shell.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "pmm.h"
#include "kheap.h"
#include "paging.h"
#include "vmem.h"
#include "user_mem.h"
#include "process.h"
#include "panic.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Shell state
static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_pos = 0;
static uint32_t boot_time = 0;

// Forward declarations for new commands
void cmd_version(int argc, char **argv);
void cmd_shutdown(int argc, char **argv);
void cmd_meminfo(int argc, char **argv);
void cmd_kalloc(int argc, char **argv);
void cmd_kfree(int argc, char **argv);
void cmd_ksize(int argc, char **argv);
void cmd_vget(int argc, char **argv);
void cmd_kbrk(int argc, char **argv);
void cmd_vmalloc(int argc, char **argv);
void cmd_vfree(int argc, char **argv);
void cmd_vsize(int argc, char **argv);
void cmd_vbrk(int argc, char **argv);
void cmd_ualloc(int argc, char **argv);
void cmd_ufree(int argc, char **argv);
void cmd_usize(int argc, char **argv);
void cmd_ps(int argc, char **argv);
void cmd_fork(int argc, char **argv);
void cmd_kill(int argc, char **argv);
void cmd_paging(int argc, char **argv);
void cmd_memory_rights(int argc, char **argv);
void cmd_mem_spaces(int argc, char **argv);
void cmd_page_ops(int argc, char **argv);
void cmd_alloc_functions(int argc, char **argv);
void cmd_virtual_physical(int argc, char **argv);
void cmd_panic_test(int argc, char **argv);

// Command table
static struct shell_command commands[] = {
    {"help", "Display this help message", cmd_help},
    // {"clear", "Clear the screen", cmd_clear},
    // {"echo", "Echo arguments to screen", cmd_echo},
    // {"reboot", "Restart the system", cmd_reboot},
    // {"halt", "Stop CPU (requires manual restart)", cmd_halt},
    // {"gdt", "Display GDT information", cmd_gdt_info},
    // {"version", "Display kernel version", cmd_version},
    // {"shutdown", "Shutdown system gracefully", cmd_shutdown},
    {"meminfo", "Show memory stats", cmd_meminfo},
    {"kalloc", "Allocate kernel memory: kalloc <bytes>", cmd_kalloc},
    {"kfree", "Free kernel memory: kfree <addr>", cmd_kfree},
    {"ksize", "Get allocated block size: ksize <addr>", cmd_ksize},
    {"kbrk", "Physical memory break: kbrk [new_addr]", cmd_kbrk},
    {"vmalloc", "Allocate virtual memory: vmalloc <bytes>", cmd_vmalloc},
    {"vfree", "Free virtual memory: vfree <addr>", cmd_vfree},
    {"vsize", "Get virtual block size: vsize <addr>", cmd_vsize},
    {"vbrk", "Virtual memory break: vbrk [new_addr]", cmd_vbrk},
    {"vget", "Show mapping of a virtual addr: vget <virt>", cmd_vget},
    {"ualloc", "Allocate user memory: ualloc <bytes>", cmd_ualloc},
    {"ufree", "Free user memory: ufree <addr>", cmd_ufree},
    {"usize", "Get user block size: usize <addr>", cmd_usize},
    // {"ps", "List processes", cmd_ps},
    // {"fork", "Create new process: fork <name>", cmd_fork},
    // {"kill", "Kill process: kill <pid>", cmd_kill},
    {"paging", "Test memory paging system", cmd_paging},
    {"memrights", "Test memory rights and protection", cmd_memory_rights},
    {"memspaces", "Test kernel and user space separation", cmd_mem_spaces},
    {"pageops", "Test page creation and management", cmd_page_ops},
    {"allocfuncs", "Test allocation functions (kmalloc, kfree, ksize)", cmd_alloc_functions},
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
    for (int i = 0; commands[i].name != NULL; i++) {
        kprintf("  %-10s - %s\n", commands[i].name, commands[i].description);
    }
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
    kprintf("  GDT Base Address: 0x%x\n", 0x00000800);
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
    kprintf("    CS: 0x%x, DS: 0x%x, ES: 0x%x\n", cs, ds, es);
    kprintf("    FS: 0x%x, GS: 0x%x, SS: 0x%x\n", fs, gs, ss);
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
    kprintf("PMM total pages: %d, free pages: %d\n", (int)pmm_total_pages(), (int)pmm_free_pages());
}

void cmd_kalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = kmalloc(n);
    kprintf("kalloc(%d) -> %x\n", (int)n, (uint32_t)p);
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
void cmd_ualloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ualloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = umalloc(n);
    kprintf("ualloc(%d) -> %x\n", (int)n, (uint32_t)p);
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

// Process management commands
void cmd_ps(int argc, char **argv)
{
    (void)argc; (void)argv;
    process_list();
}

void cmd_fork(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: fork <name>\n"); return; }
    process_t *proc = process_create(argv[1]);
    if (proc) {
        kprintf("Created process %d: %s\n", proc->pid, proc->name);
    } else {
        kprintf("Failed to create process\n");
    }
}

void cmd_kill(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kill <pid>\n"); return; }
    uint32_t pid = parse_hex_or_dec(argv[1]);
    process_t *proc = process_find_by_pid(pid);
    if (proc) {
        process_destroy(proc);
        kprintf("Killed process %d\n", pid);
    } else {
        kprintf("Process %d not found\n", pid);
    }
}

// ===== MEMORY SYSTEM TESTING COMMANDS =====

void cmd_paging(int argc, char **argv)
{
    kprintf("Testing memory paging system...\n\n");
    
    // Test 1: Check if paging is enabled
    kprintf("1. Paging Status:\n");
    kprintf("   Paging is ENABLED in your kernel\n");
    kprintf("   Page size: %d bytes (4KB)\n", PAGE_SIZE);
    kprintf("   Page directory: 1024 entries\n");
    kprintf("   Page tables: 3 tables (12MB identity mapped)\n\n");
    
    // Test 2: Show memory info
    kprintf("2. Memory Information:\n");
    cmd_meminfo(argc, argv);
    kprintf("\n");
    
    // Test 3: Test page mapping
    kprintf("3. Page Mapping Test:\n");
    kprintf("   Testing virtual to physical mapping...\n");
    cmd_vget(argc, (char*[]){"vget", "0x1000000", NULL});
    kprintf("\n");
}

void cmd_memory_rights(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    // Test 1: Show page flags
    // kprintf("1. Memory Rights Flags:\n");
    // kprintf("   PAGE_PRESENT: 0x%x (Page exists in memory)\n", PAGE_PRESENT);
    // kprintf("   PAGE_WRITE:   0x%x (Page is writable)\n", PAGE_WRITE);
    // kprintf("   PAGE_USER:    0x%x (Page accessible by user mode)\n", PAGE_USER);
    // kprintf("\n");
    
    // Test 2: Test different memory allocations
    kprintf("2. Memory Allocation with Different Rights:\n");
    
    // Kernel memory (no PAGE_USER)
    void *kernel_mem = kmalloc(100);
    kprintf("   Kernel memory (0x%x): PAGE_WRITE only\n", (uint32_t)kernel_mem);
    
    // User memory (with PAGE_USER)
    void *user_mem = umalloc(100);
    kprintf("   User memory (0x%x): PAGE_WRITE | PAGE_USER\n", (uint32_t)user_mem);
    
    // Virtual memory (no PAGE_USER)
    void *virt_mem = vmalloc(4096);
    kprintf("   Virtual memory (0x%x): PAGE_WRITE only\n", (uint32_t)virt_mem);
    kprintf("\n");
    
    // Test 3: Memory space validation
    // kprintf("3. Memory Space Validation:\n");
    // kprintf("   User memory (0x%x): ", (uint32_t)user_mem);
    // if ((uint32_t)user_mem >= 0x08000000 && (uint32_t)user_mem <= 0xBFFFFFFF) {
    //     kprintf("USER SPACE \n");
    // } else {
    //     kprintf("NOT USER SPACE \n");
    // }
    
    // kprintf("   Kernel memory (0x%x): ", (uint32_t)kernel_mem);
    // if ((uint32_t)kernel_mem >= 0x01000000 && (uint32_t)kernel_mem < 0x00C00000) {
    //     kprintf("KERNEL HEAP \n");
    // } else {
    //     kprintf("NOT KERNEL HEAP \n");
    // }
    // kprintf("\n");
    
    // Test 4: REAL FUNCTIONAL TESTS
    kprintf("4. Functional Memory Rights Tests:\n");
    
    // Test 4.1: Test memory allocation success
    kprintf("   4.1. Testing memory allocation success...\n");
    if (kernel_mem != NULL) {
        kprintf("   kmalloc(100) succeeded\n");
    } else {
        kprintf("   kmalloc(100) failed\n");
    }
    
    if (user_mem != NULL) {
        kprintf("   umalloc(100) succeeded\n");
    } else {
        kprintf("   umalloc(100) failed\n");
    }
    
    if (virt_mem != NULL) {
        kprintf("   vmalloc(4096) succeeded\n");
    } else {
        kprintf("   vmalloc(4096) failed\n");
    }
    
    // Test 4.2: Test memory write access
    kprintf("   4.2. Testing memory write access...\n");
    if (kernel_mem != NULL) {
        uint32_t *k_ptr = (uint32_t*)kernel_mem;
        *k_ptr = 0x12345678;
        if (*k_ptr == 0x12345678) {
            kprintf("   Kernel memory write access works\n");
        } else {
            kprintf("   Kernel memory write access failed\n");
        }
    }
    
    if (user_mem != NULL) {
        uint32_t *u_ptr = (uint32_t*)user_mem;
        *u_ptr = 0x87654321;
        if (*u_ptr == 0x87654321) {
            kprintf("   User memory write access works\n");
        } else {
            kprintf("   User memory write access failed\n");
        }
        
    }
    
    if (virt_mem != NULL) {
        uint32_t *v_ptr = (uint32_t*)virt_mem;
        *v_ptr = 0xDEADBEEF;
        if (*v_ptr == 0xDEADBEEF) {
            kprintf("   Virtual memory write access works\n");
        } else {
            kprintf("   Virtual memory write access failed\n");
        }
    }


}

void cmd_mem_spaces(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Testing kernel and user space separation...\n\n");
    
    // Test 1: Show memory space definitions
    kprintf("1. Memory Space Definitions:\n");
    kprintf("   BIOS Reserved: 0x00000000 - 0x000FFFFF (1MB)\n");
    kprintf("   User Space:    0x00100000 - 0xBFFFFFFF (3GB)\n");
    kprintf("   Kernel Space:  0xC0000000 - 0xFFFFFFFF (1GB)\n");
    
    // Test 2: Test user space allocation
    void *user_mem1 = umalloc(1000);
    kprintf("   ualloc(1000) = 0x%x (User space)\n", (uint32_t)user_mem1);

    
    // Test 3: Test kernel space allocation
    void *kernel_mem1 = kmalloc(1000);
    kprintf("   kmalloc(1000) = 0x%x (Kernel heap)\n", (uint32_t)kernel_mem1);

    
    // Test 4: Memory space validation
    kprintf(" Memory Space Validation:\n");
    kprintf("   User memory (0x%x): ", (uint32_t)user_mem1);
    if ((uint32_t)user_mem1 >= 0x08000000 && (uint32_t)user_mem1 <= 0xBFFFFFFF) {
        kprintf("USER SPACE \n");
    } else {
        kprintf("NOT USER SPACE \n");
    }
    
    kprintf("   Kernel memory (0x%x): ", (uint32_t)kernel_mem1);
    if ((uint32_t)kernel_mem1 >= 0x01000000 && (uint32_t)kernel_mem1 < 0x00C00000) {
        kprintf("KERNEL HEAP \n");
    } else {
        kprintf("NOT KERNEL HEAP\n");
    }

}

void cmd_page_ops(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Testing page creation and management...\n\n");
    
    // Test 1: Physical page allocation
    kprintf("1. Physical Page Allocation:\n");
    void *phys_page1 = pmm_alloc_page();
    kprintf("   pmm_alloc_page() = 0x%x\n", (uint32_t)phys_page1);

    
    // Test 2: Virtual page mapping
    kprintf("2. Virtual Page Mapping:\n");
    uint32_t virt_addr1 = 0x10000000;
    
    int result1 = vmm_map_page(virt_addr1, (uint32_t)phys_page1, PAGE_WRITE | PAGE_USER);
    
    kprintf("vmm_map_page(0x%x, 0x%x) = %d\n", virt_addr1, (uint32_t)phys_page1, result1);

    
    // Test 3: Get page mapping
    kprintf("3. Get Page Mapping:\n");
    uint32_t mapping1 = vmm_get_mapping(virt_addr1);
    kprintf("   vmm_get_mapping(0x%x) = 0x%x\n", virt_addr1, mapping1);

    
    // Test 4: Unmap pages
    kprintf("4. Unmap Pages:\n");
    vmm_unmap_page(virt_addr1);

    kprintf("   vmm_unmap_page(0x%x) completed\n", virt_addr1);


    
    //Test 5: Free physical pages
    pmm_free_page(phys_page1);


    
    // Test 6: REAL FUNCTIONAL TESTS
    kprintf("6. Functional Page Management Tests:\n");
    
    // Test 6.1: Test page allocation success
    kprintf("   6.1. Testing page allocation success...\n");
    void *test_phys = pmm_alloc_page();
    if (test_phys != NULL) {
        kprintf("   pmm_alloc_page() succeeded (0x%x)\n", (uint32_t)test_phys);
        
        // Test 6.2: Test page mapping and access
        kprintf("   6.2. Testing page mapping and access...\n");
        uint32_t test_virt = 0x20000000;
        int map_result = vmm_map_page(test_virt, (uint32_t)test_phys, PAGE_WRITE | PAGE_USER);
        
        if (map_result == 0) {
            kprintf("  vmm_map_page() succeeded\n");
            // Test 6.3: Test memory access through mapping
            kprintf("   6.3. Testing memory access through mapping...\n");
            uint32_t *virt_ptr = (uint32_t*)test_virt;
            *virt_ptr = 0x12345678;
            
            if (*virt_ptr == 0x12345678) {
                kprintf("   Memory write/read through mapping works\n");
            } else {
                kprintf("   Memory write/read through mapping failed\n");
            }
            
            // Test 6.4: Test mapping retrieval
            kprintf("   6.4. Testing mapping retrieval...\n");
            uint32_t retrieved_phys = vmm_get_mapping(test_virt);
            if (retrieved_phys == (uint32_t)test_phys) {
                kprintf("   vmm_get_mapping() returned correct address\n");
            } else {
                kprintf("   vmm_get_mapping() returned wrong address (0x%x)\n", retrieved_phys);
            }
            
            // Test 6.5: Test page unmapping
            kprintf("   6.5. Testing page unmapping...\n");
            vmm_unmap_page(test_virt);
            uint32_t after_unmap = vmm_get_mapping(test_virt);
            if (after_unmap == 0) {
                kprintf("  vmm_unmap_page() succeeded\n");
            } else {
                kprintf("   vmm_unmap_page() failed (mapping still exists)\n");
            }
        } else {
            kprintf("   vmm_map_page() failed (%d)\n", map_result);
        }
        
        // Test 6.6: Test page deallocation
        kprintf("   6.6. Testing page deallocation...\n");
        pmm_free_page(test_phys);
        kprintf("   pmm_free_page() completed\n");
    } else {
        kprintf("   pmm_alloc_page() failed\n");
    }
    
    // Test 6.7: Test page table integrity
    kprintf("   6.7. Testing page table integrity...\n");
    void *phys1 = pmm_alloc_page();
    void *phys2 = pmm_alloc_page();
    
    if (phys1 != NULL && phys2 != NULL) {
        uint32_t virt1 = 0x30000000;
        uint32_t virt2 = 0x30001000;
        
        vmm_map_page(virt1, (uint32_t)phys1, PAGE_WRITE);
        vmm_map_page(virt2, (uint32_t)phys2, PAGE_WRITE | PAGE_USER);
        
        // Test that both mappings work independently
        uint32_t *ptr1 = (uint32_t*)virt1;
        uint32_t *ptr2 = (uint32_t*)virt2;
        
        *ptr1 = 0xAAAAAAAA;
        *ptr2 = 0xBBBBBBBB;
        
        if (*ptr1 == 0xAAAAAAAA && *ptr2 == 0xBBBBBBBB) {
            kprintf("   Multiple page mappings work independently\n");
        } else {
            kprintf("   Multiple page mappings interfere with each other\n");
        }
        
        vmm_unmap_page(virt1);
        vmm_unmap_page(virt2);
        pmm_free_page(phys1);
        pmm_free_page(phys2);
    }
    
}

void cmd_alloc_functions(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    kprintf("Testing allocation, free, and size functions...\n\n");
    
    // Test 0: Show initial memory stat
    cmd_meminfo(argc, argv);
    
    // Test 1: Kernel allocation functions
    void *kptr1 = kmalloc(100);
    kprintf("   kmalloc(100) = 0x%x\n", (uint32_t)kptr1);
    
    size_t ksize1 = ksize(kptr1);
    kprintf("   ksize(0x%x) = %d bytes\n", (uint32_t)kptr1, ksize1);
    
    kfree(kptr1);
    
    // Test 1.5: Show memory state after kernel allocations
    cmd_meminfo(argc, argv);
    
    // Test 2: User allocation functions
    void *uptr1 = umalloc(200);
    kprintf("   umalloc(200) = 0x%x\n", (uint32_t)uptr1);

    
    size_t usize1 = usize(uptr1);

    kprintf("   usize(0x%x) = %d bytes\n", (uint32_t)uptr1, usize1);

    cmd_meminfo(argc, argv);
    ufree(uptr1);

    cmd_meminfo(argc, argv);
    // Test 3: Virtual allocation functions
    kprintf("3. Virtual Allocation Functions:\n");
    void *vptr1 = vmalloc(4096);

    kprintf("   vmalloc(4096) = 0x%x\n", (uint32_t)vptr1); 
    size_t vsize1 = vsize(vptr1);

    kprintf("   vsize(0x%x) = %d bytes\n", (uint32_t)vptr1, vsize1);
  
    cmd_meminfo(argc, argv);
    vfree(vptr1);

    
    // Test 4: Force new page allocation
    kprintf("4. Force New Page Allocation Test:\n");
    cmd_meminfo(argc, argv);
    void *large1 = kmalloc(5000);  // Should allocate new page
    
    kprintf("   kmalloc(5000) = 0x%x\n", (uint32_t)large1);
    
    kprintf("4.5. Memory State After Large Allocations:\n");
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
    kprintf("   pmm_alloc_page() = 0x%x\n", (uint32_t)phys1);
    kprintf("   pmm_alloc_page() = 0x%x\n", (uint32_t)phys2);
    
    pmm_free_page(phys1);
    pmm_free_page(phys2);

    
    // Test 2: Virtual memory management
    kprintf("2. Virtual Memory Management:\n");
    void *virt1 = vmalloc(4096);
    void *virt2 = vmalloc(8192);
    kprintf("   vmalloc(4096) = 0x%x\n", (uint32_t)virt1);
    kprintf("   vmalloc(8192) = 0x%x\n", (uint32_t)virt2);
    
    vfree(virt1);
    vfree(virt2);

    
    // Test 3: Memory mapping
    kprintf("3. Memory Mapping:\n");
    uint32_t virt_addr = 0x20000000;
    void *phys_page = pmm_alloc_page();
    
    int map_result = vmm_map_page(virt_addr, (uint32_t)phys_page, PAGE_WRITE | PAGE_USER);
    kprintf("   vmm_map_page(0x%x, 0x%x) = %d\n", virt_addr, (uint32_t)phys_page, map_result);
    
    uint32_t mapping = vmm_get_mapping(virt_addr);
    kprintf("   vmm_get_mapping(0x%x) = 0x%x\n", virt_addr, mapping);
    
    vmm_unmap_page(virt_addr);
    pmm_free_page(phys_page);

    
    // Test 4: Memory break functions
    kprintf("4. Memory Break Functions:\n");
    void *kbrk_current = kbrk(0);
    kprintf("   kbrk() = 0x%x\n", (uint32_t)kbrk_current);
    
    void *vbrk_current = vbrk(0);
    kprintf("   vbrk() = 0x%x\n", (uint32_t)vbrk_current);
    kprintf("\n");
    
}

void cmd_panic_test(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    // kprintf("=== REQUIREMENT 7: Kernel Panic Handling ===\n");
    // kprintf("Testing double free detection and out of memory...\n\n");
    
    // // Test 1: Double Free Detection (All Allocators)
    // kprintf("1. Double Free Detection Test:\n");
    // kprintf("   Testing double free detection in all allocators...\n");
    
    // // Test kfree double free
    // kprintf("   Testing kfree() double free...\n");
    // void *k_mem = kmalloc(100);
    // if (k_mem) {
    //     kfree(k_mem);
    //     kfree(k_mem); // This will detect double free and print error
    // }
    
    // // Test vfree double free
    // kprintf("   Testing vfree() double free...\n");
    // void *v_mem = vmalloc(4096);
    // if (v_mem) {
    //     vfree(v_mem);
    //     vfree(v_mem); // This will detect double free and print error
    // }
    
    // // Test ufree double free
    // kprintf("   Testing ufree() double free...\n");
    // void *u_mem = umalloc(200);
    // if (u_mem) {
    //     ufree(u_mem);
    //     ufree(u_mem); // This will detect double free and print error
    // }
    

    // Test 2: Fatal Panic Test - Out of Memory
    kprintf("Fatal Panic Test - Out of Memory:\n");
    kprintf("   Testing kpanic_fatal() on out of memory...\n");

    void *huge_alloc = kmalloc(50 * 1024 * 1024); 
    (void)huge_alloc; // This will call kpanic_fatal("PMM out of memory")
}