/* CreoOS — tasks. The only switching mechanism: timer IRQ saves full context
 * to the current task slot, loads the next one, single eret does the jump.
 * ctx layout MUST match vectors.S (sp-relative after `sub sp, #320`):
 *   x[0..30]  gp regs,  x[31]=spsr_el1,  x[32]=elr_el1
 */
#include <creo/types.h>

#define CTX_SLOTS   4
#define TASK_STACKS 8   /* pages each */

struct task {
    u64 x[33];
};

static struct task tasks[CTX_SLOTS];
static u64 stacks[CTX_SLOTS];
static u64 ntask;
static u64 cur;

u64 irq_handler(u64 sp)
{
    u64 *sched_pick(u64 *live);
    u64 pending = gic_pending();
    if ((pending & 0x3ff) == 29) { /* EL1 virtual timer On QEMU virt */
        timer_rearm();
        gic_ack(pending);
        sched_pick((u64 *)sp);
    } else {
        gic_ack(pending);
    }
    return sp;
}

void sched_init(void)
{
    cur = 0;
    ntask = 1; /* slot 0 = idle/first */
}

void task_create(void (*entry)(void))
{
    u64 slot = ntask;
    if (slot >= CTX_SLOTS)
        panic("out of task slots");
    ntask++;
    stacks[slot] = (u64)pmem_alloc_pages(TASK_STACKS) + TASK_STACKS * 4096;
    tasks[slot].x[32] = (u64)entry;      /* elr */
    tasks[slot].x[31] = 0x3c5;           /* spsr: EL1h, DAIF masked */
}

u64 *sched_pick(u64 *live)
{
    memcpy(tasks[cur].x, live, 33 * 8);
    cur = (cur + 1) % ntask;
    memcpy(live, tasks[cur].x, 33 * 8);
    return live;
}

void sched_run(void)
{
    for (;;)
        yield();
}

u64 *sched_switch(u64 *live)
{
    u64 *sched_pick(u64 *live);
    return sched_pick(live);
}

void yield(void)
{
    __asm__ volatile("svc #0");
}
