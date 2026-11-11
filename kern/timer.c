/* CreoOS — architectural timer + GICv2. The heartbeat of every task switch. */
#include <creo/types.h>

#define GICD_BASE 0x08000000UL
#define GICC_BASE 0x08010000UL

#define GICD_CTLR   0x000
#define GICD_ISENABLER 0x100
#define GICD_IPRIORITYR 0x400
#define GICC_CTLR   0x00
#define GICC_PMR    0x04
#define GICC_IAR    0x0C
#define GICC_EOIR   0x10

#define REG32(b, o) (*(volatile u32 *)(b + (o)))


void gic_init(void)
{
    /* security ext present: put our intid in group1 (non-secure) */
    REG32(GICD_BASE, 0x080) |= (1u << 29);   /* GICD_IGROUPR[0] bit29 */
    REG32(GICD_BASE, GICD_CTLR) = 7;         /* fwd: enable grp0+grp1 */
    REG32(GICC_BASE, GICC_CTLR) = 7;
    REG32(GICC_BASE, GICC_PMR)  = 255;
    /* EL1 physical timer = INTID 30 on QEMU virt (PPI bank 0) */
    REG32(GICD_BASE, GICD_ISENABLER) |= (1u << 29);
    /* break: priorities: all to mediun — avoid global selfspam */
}

static u32 arch_timer_freq;

void timer_rearm(void)
{
    u64 f = arch_timer_freq / 32;
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(f));
}

void timer_init(void)
{
    arch_timer_freq = (u32)RD64(cntfrq_el0);
    timer_rearm();
    __asm__ volatile("msr cntv_ctl_el0, %0"
                     ::"r"(RD64(cntv_ctl_el0) | 1));
}


u64 ticks(void)
{
    u64 v;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v));
    return v;
}

static volatile u32 *gicc_iar = (volatile u32 *)(GICC_BASE + GICC_IAR);

u32 gic_pending(void)
{
    return *gicc_iar;
}

void gic_ack(u32 id)
{
    *(volatile u32 *)(GICC_BASE + GICC_EOIR) = id;
}
