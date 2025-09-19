# KFS

This is the first project in the "Kernel From Scratch"-branch of 42 Paris.

Our kernel currently includes:
- an ASM base that is bootable with GRUB
- a basic kernel library
- keyboard interrupt support that prints to the screen
- a kprintf function
- scroll and cursor support
- color support
- different screens, with shortcuts to switch between them

## Commands:

```
# compile all source files
make kfs

# compile all source files and build iso
make build

# compile all source files, build iso, and run the kernel
make run

# remove object folder and iso folder
make clean

# remove object folder, iso folder, executable, and iso image
make fclean

```

# Project Notes

## Tools

- **QEMU**
QEMU is a machine emulator and virtualizer. It allows you to run operating systems and programs for one machine on a different machine. In the context of kernel development, it's used to emulate an x86 PC so you can test your kernel without needing real hardware.
Emulation means simulating a system (hardware or software) so that code written for that system can run in a controlled environment. QEMU emulates an x86 computer to run your kernel.

- **GRUB**
GRUB is a bootloader: a small program that runs right after the BIOS/UEFI when a computer starts. Its job is to load an operating system kernel into memory and start executing it. GRUB supports various filesystems and OS kernels. It's especially useful in kernel development because it supports the Multiboot Specification, which allows it to load kernels in a standard way.
GRUB looks for a multiboot header in your kernel binary. Once found, it uses it to know how and where to load your kernel into memory, then passes control to it.

## Concepts

- **Virtual Image**
A virtual image is a file that simulates a hard drive. GRUB and your kernel are installed on this image. QEMU can boot from this image as if it were a real disk.

- **Multiboot**
The Multiboot Specification is a standard for bootloaders and kernels. It defines:
	-   How the kernel should be structured
	-   How the bootloader passes information (like memory map, boot device info) to the kernel
	-   A specific header (placed in your ASM file) that the bootloader searches for

	To be compatible, your kernel must:
	-   Include a multiboot header in the first 8 KB of the file
    -   Align the header properly
    -   Declare things like the entry point (where execution starts)

- **ASM Boot Code**
Initial boot code written in assembly. This code sets up the CPU and passes control to your kernel's entry point (usually a function like  `main`). It must contain the multiboot header.

- **Boot**
Booting is the process of starting a computer and loading the operating system. The steps generally are:
	1.  **BIOS/UEFI**: Runs firmware-level code from hardware.
  2.  **Bootloader (like GRUB)**: Loaded by the firmware, GRUB reads its configuration, finds your kernel's multiboot header, loads the kernel binary into memory, and then transfers execution control to your kernel's entry point.
  3.  **Kernel**: Your code takes over once GRUB loads it, initializes the system (memory, drivers, etc.), and starts execution from your entry point (e.g.,  `_start`  or  `main`).
    
- **Linker**
The linker is a program that combines multiple object files (.o) into a single binary. For kernel development, it also ensures that code is loaded into the correct memory addresses.

- **Linker Script**
A file (usually an `.ld`-file) that tells the linker how to place code and data in memory. Required because kernels must be loaded at specific physical addresses.

- **Host vs Kernel**
Your host machine runs the compiler and linker, but the resulting binary must not use host libraries or linker scripts. It must be self-contained and multiboot-compliant.

- **Memory and the VGA-buffer**
The kernel will initially run in a very primitive environment, without any sophisticated graphics drivers. To display characters on the screen at this stage, it interacts directly with the VGA (Video Graphics Array) text mode buffer.
VGA Text Mode is a legacy display mode where the screen is treated as a simple grid of characters, typically 80 columns by 25 rows. Each individual character position on this grid holds both the character's code and its associated display attributes, such as foreground and background color. The VGA buffer address is located directly in memory at a specific physical address, which is commonly 0xB8000 (hexadecimal).
To display a character, you write directly to the memory location within this VGA buffer that corresponds to the desired screen position. Each character cell in the buffer is represented by two bytes. The first byte, the character byte, contains the ASCII value of the character you wish to display. The second byte, the attribute byte, defines the character's color. Within this attribute byte, the higher four bits specify the background color, and the lower four bits specify the foreground color.

- **GDT entry (Global Descriptor Table entry)**
A GDT entry is an 8-byte structure that describes a memory segment. Think of it as a "memory region definition" that tells the CPU:
  - Where the segment starts in memory
  - How big the segment is
  - What type of segment it is (code, data, stack)
  - What permissions it has (readable, writable, executable)
  - What privilege level it runs at (kernel mode vs user mode)

### GDT Entry Structure (8 bytes total)

Each GDT entry consists of these fields:

```
Byte 0-1:  Limit (bits 0-15)     - Segment size
Byte 2-3:  Base (bits 0-15)      - Segment start address (low 16 bits)
Byte 4:    Base (bits 16-23)     - Segment start address (bits 16-23)
Byte 5:    Access byte            - Segment type and permissions
Byte 6:    Flags + Limit (16-19) - Granularity and segment size
Byte 7:    Base (bits 24-31)     - Segment start address (high 8 bits)
```
### Detailed Field Breakdown

#### 1. Limit Field (2 bytes)
- **Purpose**: Defines the size of the memory segment
- **Value**: 0xFFFF = 65,535 bytes
- **With Granularity**: When granularity flag is set, this becomes 4GB (65,535 × 4KB)
- **Formula**: Actual size = (Limit + 1) × Granularity

#### 2. Base Address (4 bytes, split across 3 fields)
- **Purpose**: Defines where the segment starts in memory
- **Value**: 0x00000000 = starts at the very beginning of memory
- **Split across**: 
  - `dw 0x0000` (bits 0-15)
  - `db 0x00` (bits 16-23)  
  - `db 0x00` (bits 24-31)

#### 3. Access Byte (1 byte)
The access byte is a bit field that defines segment properties:

```
Bit 7: Present (P)           - 1 = segment exists, 0 = segment not present
Bit 6-5: Descriptor Privilege Level (DPL) - 00 = ring 0 (kernel), 11 = ring 3 (user)
Bit 4: Descriptor Type (S)   - 1 = code/data segment, 0 = system segment
Bit 3: Executable (E)        - 1 = code segment, 0 = data segment
Bit 2: Conforming (C)        - 1 = conforming code, 0 = non-conforming code
Bit 1: Readable (R)          - 1 = readable, 0 = not readable
Bit 0: Accessed (A)          - 1 = segment accessed, 0 = not accessed
```

**Example**: `10011010b` means:
- Present (1)
- Ring 0 (00) - kernel mode
- Code segment (1)
- Non-conforming (0)
- Readable (1)
- Not accessed yet (0)

#### 4. Flags + Limit (1 byte)
This byte combines flags and additional limit bits:

```
Bit 7-6: Limit bits 16-19    - Additional segment size bits
Bit 5: Available (AVL)       - Available for OS use
Bit 4: Long mode (L)         - 1 = 64-bit, 0 = 32-bit
Bit 3: Default operation size (D/B) - 1 = 32-bit, 0 = 16-bit
Bit 2: Granularity (G)       - 1 = 4KB pages, 0 = 1-byte pages
```

**Example**: `11001111b` means:
- Limit bits 16-19: 1111 (0xFFFF)
- Available: 0 (not used)
- Long mode: 0 (32-bit mode)
- Default size: 0 (16-bit default)
- Granularity: 1 (4KB pages)

### Types of GDT Entries in Our Kernel

Our kernel defines 6 GDT entries:

1. **Null Descriptor (0x00)**: Required first entry, always zero
2. **Kernel Code Segment (0x08)**: Executable code running in kernel mode
3. **Kernel Data Segment (0x10)**: Readable/writable data in kernel mode
4. **Kernel Stack Segment (0x18)**: Expand-down stack segment for kernel
5. **User Code Segment (0x20)**: Executable code running in user mode (ring 3)
6. **User Data Segment (0x28)**: Readable/writable data in user mode
7. **User Stack Segment (0x30)**: Expand-down stack segment for user mode

### Why GDT is Critical
Without proper GDT entries, your kernel would:
- **Crash immediately** when trying to access memory
- **Have undefined behavior** for privilege levels
- **Fail to enter protected mode** properly
- **Not be able to** distinguish between code and data segments

Visual Representation:
GDT Table (stored at 0x00000800):
┌─────────────────────────────────────┐
│ Entry 0: 0x00 (Null)               │
├─────────────────────────────────────┤
│ Entry 1: 0x08 (Code Segment)       │ ← Selector 0x08 points here
├─────────────────────────────────────┤
│ Entry 2: 0x10 (Data Segment)       │ ← Selector 0x10 points here  
├─────────────────────────────────────┤
│ Entry 3: 0x18 (Stack Segment)      │ ← Selector 0x18 points here
└─────────────────────────────────────┘

Memory Segments (described by GDT entries):
┌─────────────────────────────────────┐
│ 0x00000000 - 0xFFFFFFFF            │ ← Code Segment (selector 0x08)
│ (4GB, executable, readable)        │
├─────────────────────────────────────┤
│ 0x00000000 - 0xFFFFFFFF            │ ← Data Segment (selector 0x10)
│ (4GB, readable, writable)          │
├─────────────────────────────────────┤
│ 0x00000000 - 0xFFFFFFFF            │ ← Stack Segment (selector 0x18)
│ (4GB, readable, writable, expand)  │
└─────────────────────────────────────┘

## Memory System (Paging, PMM, Heap, Panics)

This kernel implements a complete, stable, and functional memory subsystem meeting the requirements below.

### What’s implemented
- **Paging enabled**: Page directory/tables are built at boot; paging is turned on (CR3/CR0.PG). Low memory is identity mapped; a kernel heap region is virtually mapped.
- **Memory structure with rights**: Page entries carry flags: present (P), writable (W), user (U). While we cannot yet know who accesses memory, flags are set appropriately for kernel/user conceptual separation.
- **Kernel/User space definition**: Kernel uses supervisor mappings; user mappings may use PAGE_USER. The design supports a high-half split later; for now, low memory identity mapping plus a separate kernel heap region.
- **Create/get memory pages**: Virtual Memory Manager (VMM) exposes `vmm_map_page`, `vmm_unmap_page`, `vmm_get_mapping`. Physical frames come from the Physical Memory Manager (PMM).
- **Allocate/free/get size (virtual/physical)**:
  - Virtual: `kmalloc`, `kfree`, `ksize` on a kernel heap backed by pages.
  - Physical: `pmm_alloc_page`, `pmm_free_page` manage 4 KiB page frames via a bitmap.
- **Kernel panics**:`kpanic_fatal` (halt) to differentiate recoverable vs fatal conditions.
- **Size limit (≤ 10 MB)**: PMM is capped to manage at most 10 MB, matching the subject constraint.

### Key files
- `src/pmm.c` / `src/pmm.h`: Physical Memory Manager (bitmap of page frames, 4 KiB pages)
- `src/paging.c` / `src/paging.h`: Page tables, enable paging, map/unmap/get mapping
- `src/kheap.c` / `src/kheap.h`: Simple kernel heap on top of paging
- `src/panic.c` / `src/panic.h`: Panic and assertion helpers
- `src/memory.c`: `memory_init(...)` wires PMM → paging → heap
- `src/kernel_main.c`: calls `memory_init(...)` during boot

### Public APIs
- PMM (physical frames):
  - `void pmm_init(uint32_t mem_bytes);`
  - `void *pmm_alloc_page(void);`
  - `void pmm_free_page(void *page);`
  - `uint32_t pmm_free_pages(void);`
  - `uint32_t pmm_total_pages(void);`
- Paging (virtual mappings):
  - `void paging_init(void);`
  - `void paging_enable(void);`
  - `int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);`
  - `void vmm_unmap_page(uint32_t virt);`
  - `uint32_t vmm_get_mapping(uint32_t virt);`
- Kernel heap (virtual allocations):
  - `void kheap_init(void);`
  - `void *kmalloc(size_t size);`
  - `void kfree(void *ptr);`
  - `size_t ksize(void *ptr);`
- Panics:
  - `void kpanic_fatal(const char *fmt, ...);` (halts)

### How to test in the shell
- `meminfo` → prints total/free PMM pages
- `kmalloc <bytes>` → returns a virtual address; `ksize <addr>` prints the aligned size; `kfree <addr>` frees it
- `vget <virt>` → shows PD/PT indices, PTE flags, and the mapped physical address
- Stress: repeat `kmalloc 4096` until out-of-memory → triggers a fatal panic (kernel halts)

### Notes
- The PMM currently assumes a contiguous region starting at 1 MB and caps to 10 MB (subject requirement). A full implementation can parse the multiboot memory map.
- Kernel/user permissions are modeled via page flags; true user-mode isolation comes when entering ring 3 code paths later.

### Why these requirements matter (notions and rationale)

- **Enable paging**: Required to use virtual memory. It lets the CPU translate virtual addresses to physical frames and enforce permissions (present/write/user). Without paging, you cannot isolate kernel from future user space or flexibly manage memory.

- **Memory structure with rights**: Page directory/table entries carry rights bits. Even before user-mode tasks, setting rights defines the contract (kernel-only vs user-accessible, read/write). This is the foundation for protection and stability.

- **Define Kernel vs User space**: A clear split prevents user code from corrupting the kernel. It also keeps a stable kernel mapping across processes (each user address space changes, kernel region stays the same).

- **Create/get memory pages (map/unmap/get)**: Page-level APIs (`vmm_map_page`, `vmm_unmap_page`, `vmm_get_mapping`) are the primitive operations on which all allocators, stacks, and heaps rely. They turn raw frames into usable virtual memory.

- **Allocate/free/size variables (heap)**: Higher-level allocation (`kmalloc`, `kfree`, `ksize`) is essential for dynamic data structures in the kernel (buffers, tables). Returning the size aids debugging and introspection.

- **Virtual and physical memory functions**: Managing both layers is necessary. The PMM hands out raw frames; the VMM maps them for use. Keeping these concerns separate simplifies design and future process isolation.

- **Kernel panics (fatal and non-fatal)**: When invariants are broken (e.g., out of memory, corrupted state), the kernel must either stop (to protect data) or warn and continue. Differentiating fatal vs non-fatal avoids unnecessary system halts while still surfacing bugs.

- **≤ 10 MB limit**: Keeps the project scoped and ensures the kernel operates within tight resource bounds. It also matches typical bootloader lab environments and simplifies PMM design before implementing full memory map parsing.

# Paging: Page Directory and Page Table (x86, 32-bit, 4KB pages)
## Entry Format

Both Page Directory Entries (PDEs) and Page Table Entries (PTEs) are 32 bits wide.
High 20 bits: a physical base address (aligned to 4KB, so the low 12 bits are always 0).

In a PDE → base address of a Page Table.
In a PTE → base address of a physical page frame.

Low 12 bits: control flags.
Common Flags

P (Present)
1 = the page/table is present in physical memory.
0 = not present (CPU will raise a page fault).

R/W (Read/Write)
0 = read-only.
1 = read/write.

U/S (User/Supervisor)
0 = only kernel can access.
1 = user mode can also access.

A (Accessed)
Set by CPU when the page/table is accessed.
D (Dirty) – only in PTE
Set by CPU when the page is written.

PS (Page Size) – only in PDE
0 = PDE points to a Page Table (4KB pages).
1 = PDE maps directly to a large 4MB page.

Permissions Rule
Permissions are checked at both PDE and PTE level. The final access right is the most restrictive:

If PDE says read-only, then even if PTE says read/write, the page is read-only.
If PDE says supervisor-only, then even if PTE says user-accessible, only kernel can access.

Address Translation (4KB pages)
A 32-bit virtual address is split into three fields:
| Directory Index (10 bits) | Table Index (10 bits) | Offset (12 bits) |
Directory Index → select a PDE from the Page Directory.
PDE contains the physical base address of a Page Table (or a large page).
Table Index → select a PTE from that Page Table.
PTE contains the physical base address of a 4KB page frame.
Offset → added to the page frame base address to get the final physical address.

Memory Coverage
A Page Table has 1024 entries × 4KB = 4MB coverage.
A Page Directory has 1024 entries × 4MB = 4GB coverage (full 32-bit space).
Each Page Directory or Page Table uses 1024 × 4 bytes = 4KB.

## Why Two Levels?
Scalability:
One single-level table for 4GB would require 1M entries × 4B = 4MB, always resident in memory.
With two levels, only the Page Directory (4KB) is always present. Page Tables (4KB each) are allocated on demand.

Flexibility:
Different regions can have different permissions.

Efficiency:
4KB page size is a hardware choice:
4KB = 2¹², which allows 12 bits for page offset.
It balances memory fragmentation and table size.
Supported by the CPU’s TLB (fast lookup).

## Who Sets the Flags?

Operating System (OS): builds and updates PDEs/PTEs, decides P bits, permissions, etc.
CPU: only reads them. If P=0, CPU raises a page fault and lets the OS handle it.
PDE.P = whether the Page Table exists in memory.
PTE.P = whether the actual page frame exists in memory.


Virtual Memory Layout:
0x00000000 ┌─────────────────┐
           │   BIOS Reserved │ ← BIOS, hardware (1MB)
0x00100000 ├─────────────────┤ ← Kernel start (identity mapped)
           │   Kernel Code   │ ← .text section
0x00101000 ├─────────────────┤ ← Kernel data (identity mapped)
           │   Kernel Data   │ ← .data section
0x00102000 ├─────────────────┤ ← Kernel bss (identity mapped)
           │   Kernel BSS    │ ← .bss section
0x00C00000 ├─────────────────┤ ← IDENTITY MAPPING ENDS HERE
           │   UNUSED        │ ← 0x00C00000-0x00FFFFFF (4MB)
0x01000000 ├─────────────────┤ ← Kernel Heap (kmalloc) starts here
           │   Dynamic Alloc │ ← ONLY kernel heap
0x02000000 ├─────────────────┤ ← Virtual Memory (vmalloc) starts here
           │   Page Alloc    │ ← vmalloc region (NOT kernel heap)
0x08000000 ├─────────────────┤ ← User Space starts here
           │   User Heap     │ ← umalloc() region (0x08000000-0xBFFFFFFF)
0xC0000000 ├─────────────────┤ ← Kernel Space starts here
           │   Kernel Space  │ ← Future kernel virtual space
0xFFFFFFFF └─────────────────┘