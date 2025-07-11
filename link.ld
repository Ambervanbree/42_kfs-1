/* filepath: link.ld */
ENTRY(start)

SECTIONS
{
    . = 1M;

    .text : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    .data : ALIGN(4K) {
        *(.data)
    }

    .bss : ALIGN(4K) {
        *(.bss)
    }

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.eh_frame)
    }
}