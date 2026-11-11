/* demo tasks: task01 sleeps and paints once a second, task02 never yields —
 * it only makes progress alongside task01 if timer preemption works. */
#include <creo/types.h>

static volatile u64 spins;

void task01(void)
{
    char line[64];
    for (u64 n = 0;; n++) {
        ksnprintf(line, sizeof(line), "uptime %us   task02 spins %lx      ",
                  (u32)(jiffies() / HZ), spins);
        ui_puts(2, 2, line);
        ui_paint();
        kprintf("task01 tick %u jiffies %u spins %lx\n", (u32)n, (u32)jiffies(), spins);
        sleep_ticks(HZ);
    }
}

void task02(void)
{
    for (;;)
        spins++;
}
