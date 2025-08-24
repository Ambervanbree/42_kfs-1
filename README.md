# KFS_1

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