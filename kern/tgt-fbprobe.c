/* CreoOS — fb probe: find the live framebuffer sboot left running.
 * The Exynos9820's own DPU keeps fetching pixels continuously from the fb
 * address it was last configured with. We paint candidate addresses with
 * timed unique colors: WHATEVER the user sees on the glass pinpoints
 * the real fb. No UART, no USB, no jig needed — skin game, eyes only.
 * DRAM walk-back is safe: sboot handed everything over at kernel entry.
 */
#include <creo/types.h>

/* candidate fb bases for exynos9820-based devices (s4 = 2720x1440 BGRA = 15.6MB).
 * slot__; самый — sboot reserves: near the tail, 256MB-aligned-ish,
 * configured by the previous chain of stages. */
static const struct { u64 base; u32 sig; } cands[] = {
    { 0xB0800000UL, 0xFF0000 }, /* 0: red    */
    { 0x8E000000UL, 0x00FF00 }, /* 1: green  */
    { 0xA0000000UL, 0x0000FF }, /* 2: blue   */
    { 0x78000000UL, 0xFFFF00 }, /* 3: yellow */
    { 0x98000000UL, 0xFF00FF }, /* 4: purple */
    { 0xE0000000UL, 0x00FFFF }, /* 5: cyan   */
    { 0x60000000UL, 0xFFFFFF }, /* 6: white  */
    { 0xC0000000UL, 0xFF5500 }, /* 7: orange */
};

#define W 1000      /* paint width only — fast, visible */
#define H 1500
#define WAIT 3000000

static void delay(u64 n) { for (u64 i = 0; i < n; i++) __asm__ volatile("nop"); }

void fb_probe_run(void)
{
    for (unsigned i = 0; i < (sizeof(cands) / sizeof(cands[0])); i++) {
        volatile u32 *fb = (volatile u32 *)cands[i].base;
        u32 c = cands[i].sig | 0xFF000000;
        for (u32 y = 0; y < H; y++)
            for (u32 x = 0; x < W; x++)
                fb[y * 2720 + x] = c;
        delay(WAIT);
        /* black-out so the next color is unambiguous */
        for (u32 y = 0; y < H; y++)
            for (u32 x = 0; x < W; x++)
                fb[y * 2720 + x] = 0xFF000000;
    }
}
