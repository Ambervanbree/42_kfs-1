#include "process.h"
#include "user_mem.h"
#include "paging.h"
#include "pmm.h"
#include "panic.h"
#include "kprintf.h"
#include "string.h"

// Process management
static process_t processes[MAX_PROCESSES];
static uint32_t next_pid = 1;
static uint32_t num_processes = 0;
process_t *current_process = 0;

void process_init(void)
{
    // Initialize process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = PROCESS_TERMINATED;
        memset(processes[i].name, 0, MAX_PROCESS_NAME);
    }
    
    // Create kernel process (PID 0)
    current_process = &processes[0];
    current_process->pid = 0;
    strcpy(current_process->name, "kernel");
    current_process->state = PROCESS_RUNNING;
    current_process->page_directory = 0;  // Kernel uses current page directory
    current_process->heap_start = (void*)0x01000000;  // Kernel heap
    current_process->heap_end = (void*)0x02000000;
    current_process->stack_start = (void*)0x00000000;  // Kernel stack
    current_process->stack_end = (void*)0x00100000;
    current_process->memory_used = 0;
    current_process->pages_allocated = 0;
    
    num_processes = 1;
    kprintf("Process management initialized.\n");
}

process_t *process_create(const char *name)
{
    if (num_processes >= MAX_PROCESSES) {
        kprintf("Error: Maximum number of processes reached\n");
        return 0;
    }
    
    // Find free process slot
    process_t *proc = 0;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_TERMINATED) {
            proc = &processes[i];
            break;
        }
    }
    
    if (!proc) {
        kprintf("Error: No free process slots\n");
        return 0;
    }
    
    // Initialize process
    proc->pid = next_pid++;
    strncpy(proc->name, name, MAX_PROCESS_NAME - 1);
    proc->name[MAX_PROCESS_NAME - 1] = '\0';
    proc->state = PROCESS_READY;
    
    // Allocate page directory for user process
    proc->page_directory = (uint32_t)pmm_alloc_page();
    if (!proc->page_directory) {
        kprintf("Error: Failed to allocate page directory for process %d\n", proc->pid);
        return 0;
    }
    
    // Initialize user memory regions
    proc->heap_start = (void*)USER_HEAP_START;
    proc->heap_end = (void*)(USER_HEAP_START + 0x10000000);  // 256MB heap
    proc->stack_start = (void*)USER_STACK_START;
    proc->stack_end = (void*)(USER_STACK_START + USER_STACK_SIZE);
    proc->memory_used = 0;
    proc->pages_allocated = 0;
    
    // Initialize registers for user mode
    proc->cs = 0x1B;  // User code segment (Ring 3)
    proc->ds = 0x23;  // User data segment (Ring 3)
    proc->es = 0x23;  // User data segment (Ring 3)
    proc->fs = 0x23;  // User data segment (Ring 3)
    proc->gs = 0x23;  // User data segment (Ring 3)
    proc->ss = 0x23;  // User stack segment (Ring 3)
    proc->eflags = 0x202;  // Interrupts enabled, IOPL=0
    
    num_processes++;
    kprintf("Created process %d: %s\n", proc->pid, proc->name);
    return proc;
}

void process_destroy(process_t *proc)
{
    if (!proc || proc->pid == 0) {
        kprintf("Error: Cannot destroy kernel process\n");
        return;
    }
    
    // Free page directory
    if (proc->page_directory) {
        pmm_free_page((void*)proc->page_directory);
    }
    
    // Mark process as terminated
    proc->state = PROCESS_TERMINATED;
    proc->pid = 0;
    memset(proc->name, 0, MAX_PROCESS_NAME);
    
    num_processes--;
    kprintf("Destroyed process %d\n", proc->pid);
}

process_t *process_find_by_pid(uint32_t pid)
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return 0;
}

void process_list(void)
{
    kprintf("Process List:\n");
    kprintf("PID  Name                State      Memory    Pages\n");
    kprintf("---- ------------------- ---------- --------  -----\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROCESS_TERMINATED) {
            const char *state_str = "Unknown";
            switch (processes[i].state) {
                case PROCESS_RUNNING:  state_str = "Running";  break;
                case PROCESS_READY:    state_str = "Ready";    break;
                case PROCESS_BLOCKED:  state_str = "Blocked";  break;
                case PROCESS_TERMINATED: state_str = "Terminated"; break;
            }
            
            kprintf("%-4d %-18s %-10s %-8d %-5d\n", 
                   processes[i].pid, 
                   processes[i].name, 
                   state_str,
                   processes[i].memory_used,
                   processes[i].pages_allocated);
        }
    }
}

// Process memory management
int process_alloc_memory(process_t *proc, size_t size)
{
    if (!proc) return -1;
    
    // Use user memory allocator
    void *ptr = umalloc(size);
    if (!ptr) return -1;
    
    // Update process statistics
    proc->memory_used += size;
    proc->pages_allocated += (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    return 0;
}

void process_free_memory(process_t *proc, void *ptr)
{
    if (!proc || !ptr) return;
    
    // Get size before freeing
    size_t size = usize(ptr);
    
    // Free memory
    ufree(ptr);
    
    // Update process statistics
    if (proc->memory_used >= size) {
        proc->memory_used -= size;
    }
    if (proc->pages_allocated > 0) {
        proc->pages_allocated -= (size + PAGE_SIZE - 1) / PAGE_SIZE;
    }
}

size_t process_get_memory_size(process_t *proc, void *ptr)
{
    if (!proc || !ptr) return 0;
    return usize(ptr);
}

// System calls
void syscall_exit(int status)
{
    if (current_process && current_process->pid != 0) {
        kprintf("Process %d exiting with status %d\n", current_process->pid, status);
        process_destroy(current_process);
        current_process = &processes[0];  // Return to kernel process
    }
}

void *syscall_malloc(size_t size)
{
    if (!current_process) return 0;
    return umalloc(size);
}

void syscall_free(void *ptr)
{
    if (!current_process) return;
    ufree(ptr);
}

size_t syscall_size(void *ptr)
{
    if (!current_process) return 0;
    return usize(ptr);
}