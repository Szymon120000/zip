/* CreoOS — irq-driven full-context switch. Every task switch is an exception
 * return; no separate path, nothing to desync.
 *
 * ctx layout (offsets from sp at save point, 8-byte, when sp points at slot 0
 * AFTER `sub sp, #320`):
 *   0..240  x0..x30   (31 regs)
 *   248     spsr_el1
 *   256     elr_el1
 *   264     sp_el1 (pre-interrupt)
 *  task structs carry exactly this layout.
 */
.macro ventry t
    b \t
    .balign 128
.endm

.section .vectors, "ax"
.p2align 11
.global vectors
vectors:
 Sync_Undef:  ventry vec_bad
 Sync_SVC:    ventry el1_svc
 Sync_HVC:    ventry vec_bad
 Sync_DBG:    ventry vec_bad
 IRQ:         ventry el1_irq
 FIQ:         ventry vec_bad
 Error:       ventry vec_bad
 Sync_Undef2: ventry vec_bad
 Sync_SVC2:   ventry el1_sync
 Sync_HVC2:   ventry vec_bad
 Sync_DBG2:   ventry vec_bad
 IRQ2:        ventry vec_bad
 FIQ2:        ventry vec_bad
 Error2:      ventry vec_bad
.p2align 11
vectors_end:
.global vectors_end

vec_bad:
el1_sync:
    sub sp, sp, #320
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mrs x1, spsr_el1
    mrs x2, elr_el1
    str x1, [sp, #248]
    str x2, [sp, #256]
    msr     daifset, #0xf
    mov     x0, sp
    bl      sync_trap
    mov     sp, x0
    b       ctx_restore

el1_irq:
    sub sp, sp, #320
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mrs x1, spsr_el1
    mrs x2, elr_el1
    str x1, [sp, #248]
    str x2, [sp, #256]
    msr daifset, #0xf
    mov x0, sp
    bl      irq_handler
    mov     sp, x0
    /* fallthrough restore */

/* yield(): svc from any task; swaps via ctx frames, no GIC needed */
el1_svc:
    sub sp, sp, #320
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mrs x1, spsr_el1
    mrs x2, elr_el1
    str x1, [sp, #248]
    str x2, [sp, #256]
    msr  daifset, #0xf
    mov  x0, sp
    mov  x19, x0
    bl   sched_switch     /* saves cur ctx -> tasks[cur], picks next, returns frame */
    mov  sp, x0
    b    ctx_restore


ctx_restore:
    ldr x1, [sp, #256]
    msr elr_el1, x1
    ldr x1, [sp, #248]
    msr spsr_el1, x1
    ldp x0,  x1,  [sp, #0]
    ldp x2,  x3,  [sp, #16]
    ldp x4,  x5,  [sp, #32]
    ldp x6,  x7,  [sp, #48]
    ldp x8,  x9,  [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    add sp, sp, #320
    eret
