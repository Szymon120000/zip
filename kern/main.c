/* CreoOS kmain — bring-up order fixed: uart, vectors, mm, sched, gic+timer, ui, tasks.
 * No DTB parser yet (that arrives with beyond2lte raw bring-up).
 */
#include <creo/types.h>

void uart_init(void);
u64 *sched_switch(u64 *frame);
extern u8 vectors[];

#define ESR_EC(esr)  ((u32)((esr) >> 26) & 0x3f)
#define EC_SVC64     0x15

#ifndef CREO_PHONE
static void vectors_init(void)
{
    __asm__ volatile("msr vbar_el1, %0\nisb" ::"r"((u64)vectors));
}

static char banner[] =
    "\n"
    "  ██████╗ ██████╗ ███████╗ ██████╗  ██████╗ ███████╗\n"
    "  ██╔════╝██╔═══██╗██╔════╝██╔═══██╗██╔════╝ ╚══███╔╝\n"
    "  ██║     ██║   ██║███████╗██║   ██║██║  ███╗  ███╔╝ \n"
    "  ██║     ██║   ██║╚════██║██║   ██║██║   ██║ ███╔╝  \n"
    "  ╚██████╗╚██████╔╝███████║╚██████╔╝╚██████╔╝███████╗\n"
    "   ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝  ╚═════╝ ╚══════╝\n"
    "  custom kernel • custom UI • absolutely no bloat\n\n";
#endif

void kmain(u64 dtb)
{
#ifdef CREO_PHONE
    extern void exynos_uart_probe(void);
    extern void exynos_uart_puts(const char *);
    extern void fb_probe_run(void);
    (void)dtb;
    exynos_uart_probe();
    exynos_uart_puts("creo: booting, running the fb probe\n");
    for (;;) {
        fb_probe_run();   /* red->green->blue->yellow->purple->cyan->white->orange */
        for (u64 i = 0; i < 20000; i++)
            __asm__ volatile("nop");
    }
#else
    uart_init();
    kputs_raw(banner);
    kprintf("boot:    EL%u, dtb=%lx\n", (u32)(RD64(CurrentEL) >> 2), dtb);

    vectors_init();
    puts("vectors: online");
    mm_init();
    puts("mm:      online");
    sched_init();
    gic_init();
    timer_init();
    kprintf("timer:   %u Hz tick, counter %u Hz\n", (u32)HZ, (u32)RD64(cntfrq_el0));

    ui_init();
    puts("ui:      online");

    extern void task01(void);
    extern void task02(void);
    task_create(task01);
    task_create(task02);
    puts("creo:    scheduling (2 tasks, preemptive)");
    sched_run();
#endif
}

void panic(const char *why)
{
    intr_disable();
    kprintf("\n!!! PANIC: %s\n", why == NULL ? "(unspeccable)" : why);
    for (;;)
        wfi();
}

static void dump_trap(const char *what, u64 *frame)
{
    kprintf("%s esr=%lx elr=%lx far=%lx spsr=%lx sp=%lx\n", what,
            RD64(esr_el1), frame[32], RD64(far_el1), frame[31], (u64)frame);
}

/* Synchronous exception from EL1. `svc` is yield(); anything else is fatal. */
u64 *sync_handler(u64 *frame)
{
    if (ESR_EC(RD64(esr_el1)) == EC_SVC64)
        return sched_switch(frame);
    dump_trap("sync", frame);
    panic("sync exception");
}

void bad_trap(u64 *frame)
{
    dump_trap("bad vector", frame);
    panic("unexpected exception");
}
