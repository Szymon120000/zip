/* CreoOS kmain — bring-up order fixed: uart, vectors, mm, gic+timer, ui, tasks.
 * No DTB parser yet (that arrives with beyond2lte raw bring-up).
 */
#include <creo/types.h>

void kputs_raw(const char *s);   /* vectors.S calls this */
void uart_init(void);
void gic_init(void);
u64 sync_trap(u64 sp);
u64  irq_handler(u64 sp);
extern u8 vectors[];

static void vectors_init(void)
{
    __asm__ volatile("msr vbar_el1, %0" ::"r"((u64)vectors));
}

void el1_bad(void);
void panic_halt(void);

static char banner[] =
    "\n"
    "  ██████╗ ██████╗ ███████╗ ██████╗  ██████╗ ███████╗\n"
    "  ██╔════╝██╔═══██╗██╔════╝██╔═══██╗██╔════╝ ╚══███╔╝\n"
    "  ██║     ██║   ██║███████╗██║   ██║██║  ███╗  ███╔╝ \n"
    "  ██║     ██║   ██║╚════██║██║   ██║██║   ██║ ███╔╝  \n"
    "  ╚██████╗╚██████╔╝███████║╚██████╔╝╚██████╔╝███████╗\n"
    "   ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝  ╚═════╝ ╚══════╝\n"
    "  custom kernel • custom UI • absolutely no bloat\n\n";

void kmain(void)
{
#ifdef CREO_PHONE
    extern void exynos_uart_probe(void);
    extern void exynos_uart_puts(const char *);
    extern void fb_probe_run(void);
    exynos_uart_probe();
    exynos_uart_puts("creo: booting, running the fb probe\n");
    for (;;) {
        fb_probe_run();   /* red->green->blue->yellow->purple->cyan->white->orange */
        for (u64 i = 0; i < 20000; i++)
            __asm__ volatile("nop");
    }
#endif
    uart_init();
    kputs_raw(banner);

    vectors_init();
    puts("vectors: online");
    mm_init();
    puts("mm:      online");
    gic_init();
    timer_init();
    intr_enable();

    ui_init();
    puts("ui: online");

    sched_init();
    extern void task01(void);
    task_create(task01);
    kprintf("creo: scheduling (1 worker task)\n");
    sched_run();
}

void panic(const char *why)
{
    intr_disable();
    kprintf("\n!!! PANIC: %s\n", why == NULL ? "(unspeccable)" : why);
    for (;;)
        wfi();
}

void el1_bad(void)     { panic("bad vector"); }
void panic_halt(void)  { panic("halt from asm"); }

u64 sync_trap(u64 sp)
{
    u64 esr, elr, far, spsr;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    kprintf("sync esr=%x elr=%x far=%x spsr=%x sp=%x\n",
            (u32)(esr & 0xf0000001u), (u32)elr, (u32)far, (u32)spsr, (u32)sp);
    panic("sync exception");
    return sp; /* unreachable */
}
