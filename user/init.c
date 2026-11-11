/* first task + demo task */
#include <creo/types.h>

extern void ui_paint(void);
static u64 counter;

void task01(void)
{
    counter = 0;
    for (;;) {
        kprintf("task01 tick %x\n", (u32)counter++);
        ui_paint();
        yield();
    }
}
