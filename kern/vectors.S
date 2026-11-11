/* CreoOS — exception vectors + full-context switch.
 *
 * Every task switch is an exception return. The handler saves a frame on the
 * interrupted task's OWN stack, C code returns the stack pointer of the task
 * to resume, and one eret does the jump.
 *
 * Frame layout (272 bytes, sp-relative, must match sched.c):
 *   0..240  x0..x30
 *   248     spsr_el1
 *   256     elr_el1
 *   264     pad (keeps sp 16-byte aligned)
 *
 * AArch64 table: 4 groups x (Sync, IRQ, FIQ, SError), 0x80 apart:
 *   0x000 current EL, SP0      0x200 current EL, SPx  <- the kernel runs here
 *   0x400 lower EL, AArch64    0x600 lower EL, AArch32
 */
#define FRAME 272

.macro ventry t
    .balign 128
    b \t
.endm

.macro save_frame
    sub sp, sp, #FRAME
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
    stp x1, x2, [sp, #248]
.endm

.section .vectors, "ax"
.balign 2048
.global vectors
vectors:
    /* current EL, SP0 — never used, we always run on SPx */
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
    /* current EL, SPx */
    ventry el1_sync
    ventry el1_irq
    ventry vec_bad
    ventry vec_bad
    /* lower EL, AArch64 — no usermode yet */
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
    /* lower EL, AArch32 */
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
    ventry vec_bad
.balign 128

.text
vec_bad:
    save_frame
    mov x0, sp
    bl  bad_trap            /* never returns */
    b   .

el1_sync:
    save_frame
    mov x0, sp
    bl  sync_handler        /* returns the frame to resume */
    mov sp, x0
    b   ctx_restore

el1_irq:
    save_frame
    mov x0, sp
    bl  irq_handler         /* returns the frame to resume */
    mov sp, x0

ctx_restore:
    ldp x1, x2, [sp, #248]
    msr spsr_el1, x1
    msr elr_el1, x2
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
    add sp, sp, #FRAME
    eret
