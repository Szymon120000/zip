/* CreoOS — architectural timer + GICv2. The heartbeat of every task switch.
 *
 * QEMU virt PPIs (INTID = PPI + 16):
 *   27  EL1 virtual timer   (cntv_*)   <- what we use
 *   30  EL1 physical timer  (cntp_*)
 *   29  SECURE physical timer — not ours; enabling it delivers nothing
 */
#include <creo/types.h>

#define GICD_BASE 0x08000000UL
#define GICC_BASE 0x08010000UL

#define GICD_CTLR      0x000
#define GICD_ISENABLER 0x100
#define GICC_CTLR      0x00
#define GICC_PMR       0x04
#define GICC_IAR       0x0C
#define GICC_EOIR      0x10

#define IRQ_VTIMER   27
#define IRQ_SPURIOUS 1023

#define REG32(b, o) (*(volatile u32 *)((b) + (o)))

u64 *sched_tick(u64 *frame);

static u32 arch_timer_freq;

void gic_init(void)
{
    REG32(GICC_BASE, GICC_PMR)  = 0xff;
    REG32(GICD_BASE, GICD_ISENABLER) = (1u << IRQ_VTIMER);
    REG32(GICD_BASE, GICD_CTLR) = 1;
    REG32(GICC_BASE, GICC_CTLR) = 1;
}

void timer_rearm(void)
{
    u64 f = arch_timer_freq / HZ;
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(f));
}

void timer_init(void)
{
    arch_timer_freq = (u32)RD64(cntfrq_el0);
    timer_rearm();
    u64 on = 1;             /* ENABLE=1, IMASK=0 */
    __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(on));
}

u64 ticks(void)
{
    return RD64(cntvct_el0);
}

u64 *irq_handler(u64 *frame)
{
    u32 iar = REG32(GICC_BASE, GICC_IAR);
    u32 id = iar & 0x3ff;

    if (id == IRQ_SPURIOUS)
        return frame;
    if (id == IRQ_VTIMER) {
        timer_rearm();
        REG32(GICC_BASE, GICC_EOIR) = iar;
        return sched_tick(frame);
    }
    REG32(GICC_BASE, GICC_EOIR) = iar;
    return frame;
}
