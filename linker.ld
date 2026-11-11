/* CreoOS linker script: image loaded flat at 0x40080000 (QEMU -kernel &
 * u-boot compatible). No convoluted sections — bloat-free by design. */
ENTRY(_start)
SECTIONS
{
    . = 0x40080000;
    .text : ALIGN(4096) {
        KEEP(*(.text.boot))
        *(.text*)
        *(.vectors)
    }
    .rodata : ALIGN(4096) {
        *(.rodata.vectors)
        *(.rodata*)
    }
    .data : ALIGN(4096) { *(.data*) }
    .bss (NOLOAD) : ALIGN(4096) {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
    }
    /DISCARD/ : { *(.comment) *(.icc) *(.note*) *(.eh_frame*) }
    __bss_end = .;
    . = ALIGN(16);
    PROVIDE(kstack_top = . + 0x10000);  /* 64 KiB kernel stack, top-down */
}
