#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

// Process management for user space

#define MAX_PROCESSES 16
#define MAX_PROCESS_NAME 32

// Process states
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

// Process structure
typedef struct process {
    uint32_t pid;                    // Process ID
    char name[MAX_PROCESS_NAME];     // Process name
    process_state_t state;           // Process state
    
    // Memory management
    uint32_t page_directory;         // Page directory physical address
    void *heap_start;                // User heap start
    void *heap_end;                  // User heap end
    void *stack_start;               // User stack start
    void *stack_end;                 // User stack end
    
    // Registers (for context switching)
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    
    // Process statistics
    uint32_t memory_used;            // Memory used in bytes
    uint32_t pages_allocated;        // Number of pages allocated
} process_t;

// Process management functions
void process_init(void);
process_t *process_create(const char *name);
void process_destroy(process_t *proc);
process_t *process_find_by_pid(uint32_t pid);
void process_list(void);

// Process memory management
int process_alloc_memory(process_t *proc, size_t size);
void process_free_memory(process_t *proc, void *ptr);
size_t process_get_memory_size(process_t *proc, void *ptr);

// System calls
void syscall_exit(int status);
void *syscall_malloc(size_t size);
void syscall_free(void *ptr);
size_t syscall_size(void *ptr);

// Current process
extern process_t *current_process;

#endif