/* filepath: link.ld */
ENTRY(start)

SECTIONS
{
    . = 1M;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .data : {
        *(.data)
    }

    /* Place GDT section after data but at a known location */
    . = ALIGN(8);
    _gdt_start = .;
    .gdt_section : {
        *(.gdt_section)
    }
    _gdt_end = .;

    .bss : {
        /* Mark the start of the .bss section for zeroing in boot.s */
        __bss_start = .;
        *(.bss)
        *(COMMON)
        /* Mark the end of the .bss section for zeroing in boot.s */
        __bss_end = .;
    }
}