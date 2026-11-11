/* CreoOS — kernel entry. We own the CPU from line one.
 * Bootloaders hand over in EL2 or EL1 with x0 = DTB pointer (may be 0).
 * We run flat (MMU off) until a real MMU pass lands. Bloat budget: 0 bytes.
 */
.section .text.boot
.global _start
_start:
    mov  x20, x0                /* keep the DTB pointer for kmain */

    /* smp: park every core except the boot core */
    mrs  x1, mpidr_el1
    and  x1, x1, #0xff
    cbz  x1, 1f
park:
    wfe
    b    park
1:
    /* entered in EL2? drop to EL1h: AArch64 EL1, timers untrapped */
    mrs  x1, CurrentEL
    lsr  x1, x1, #2
    cmp  x1, #2
    b.ne at_el1
    mov  x1, #(1 << 31)         /* HCR_EL2.RW */
    msr  hcr_el2, x1
    mov  x1, #3                 /* CNTHCTL_EL2: EL1 may use counter + timer */
    msr  cnthctl_el2, x1
    msr  cntvoff_el2, xzr
    mov  x1, #0x3c5             /* EL1h, DAIF masked */
    msr  spsr_el2, x1
    adr  x1, at_el1
    msr  elr_el2, x1
    eret

at_el1:
    msr  spsel, #1
    ldr  x5, =kstack_top
    mov  sp, x5

    /* clear .bss */
    ldr  x6, =__bss_start
    ldr  x7, =__bss_end
bssloop:
    cmp  x6, x7
    b.hs 2f
    str  xzr, [x6], #8
    b    bssloop
2:
    mov  x0, x20
    bl   kmain
hang:
    wfi
    b    hang
