/* CreoOS — phone build: image lands at 0x10008000 (Samsung sboot load addr
 * for beyond2lte = base 0x10000000 + offset 0x8000). */
ENTRY(_start)
SECTIONS
{
    . = 0x10008000;
    .text : ALIGN(8192) {
        KEEP(*(.text.boot))
        *(.text*)
        *(.vectors)
    }
    .rodata : ALIGN(8192) { *(.rodata*) }
    .data : ALIGN(8192) { *(.data*) }
    .bss (NOLOAD) : ALIGN(8192) {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
        . = ALIGN(16);
        PROVIDE(kstack_top = . + 0x40000);
    }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
