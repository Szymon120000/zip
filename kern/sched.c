/* CreoOS — tasks. Round-robin, preemptive.
 *
 * Each task has its own stack. A task that is not running is nothing but a
 * saved stack pointer aimed at a frame laid out by vectors.S:
 *   [0..30] x0..x30   [31] spsr_el1   [32] elr_el1   [33] pad
 * Switching = hand vectors.S a different frame pointer.
 *
 * Slot 0 is the boot context; it becomes the idle task and only runs when
 * nothing else is runnable.
 */
#include <creo/types.h>

#define NTASK       8
#define STACK_PAGES 8
#define FRAME_WORDS 34
#define SPSR_EL1H   0x5     /* EL1, own stack, interrupts enabled */

struct task {
    u64 *sp;
    u64  wake;              /* runnable once jiffies() >= wake */
};

static struct task tasks[NTASK];
static u64 ntask;
static u64 cur;
static volatile u64 njiffies;

u64 jiffies(void) { return njiffies; }

static void task_exit(void)
{
    panic("task returned");
}

void sched_init(void)
{
    cur = 0;
    ntask = 1;
}

void task_create(void (*entry)(void))
{
    if (ntask >= NTASK)
        panic("out of task slots");
    u64 *top = (u64 *)((u8 *)pmem_alloc_pages(STACK_PAGES) + STACK_PAGES * 4096);
    u64 *frame = top - FRAME_WORDS;
    memset(frame, 0, FRAME_WORDS * 8);
    frame[30] = (u64)task_exit;
    frame[31] = SPSR_EL1H;
    frame[32] = (u64)entry;
    tasks[ntask].sp = frame;
    tasks[ntask].wake = 0;
    ntask++;
}

/* Called with interrupts masked, from an exception. */
u64 *sched_switch(u64 *frame)
{
    tasks[cur].sp = frame;
    for (u64 i = 1; i <= ntask; i++) {
        u64 n = (cur + i) % ntask;
        if (n != 0 && tasks[n].wake <= njiffies) {
            cur = n;
            return tasks[n].sp;
        }
    }
    cur = 0;
    return tasks[0].sp;
}

u64 *sched_tick(u64 *frame)
{
    njiffies++;
    return sched_switch(frame);
}

void yield(void)
{
    __asm__ volatile("svc #0");
}

void sleep_ticks(u64 n)
{
    intr_disable();
    tasks[cur].wake = njiffies + n;
    intr_enable();
    yield();
}

void sched_run(void)
{
    intr_enable();
    for (;;)
        wfi();
}
