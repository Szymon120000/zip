/* CreoOS — kernel entry. No bootloader handout: we own the CPU from line one.
 * QEMU virt: CPUs in EL2, x0 may carry DTB pointer. exponential mapping: identity OFF (VA==PA useless),
 * we run flat until a real MMU pass lands. Bloat budget: 0 bytes.
 */
.section .text.boot
.global _start
_start:
    /* smp: park every core except boot core */
    mrs  x0, mpidr_el1
    and  x0, x0, #0xff
    cbz  x0, 1f
park:
    wfe
    b    park
1:
    /* stack */
    ldr  x5, =kstack_top
    mov  sp, x5
    /* skip built-in low vectors; we install our own later */
    msr  vbar_el1, xzr

    /* clear .bss */
    ldr x6, =__bss_start
    ldr x7, =__bss_end
    cbz x6, 2f
bssloop:
    cmp x6, x7
    b.hs 2f
    str xzr, [x6], #8
    b   bssloop
2:
    bl  kmain
hang:
    wfi
    b   hang
