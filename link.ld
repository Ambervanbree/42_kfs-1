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

    .bss : {
        /* Mark the start of the .bss section for zeroing in boot.s */
        __bss_start = .;
        *(.bss)
        *(COMMON)
        /* Mark the end of the .bss section for zeroing in boot.s */
        __bss_end = .;
    }
}