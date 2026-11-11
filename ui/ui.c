/* CreoUI v0 — text compositing now, DRM/virtio-gpu pixels next pass.
 * Screen = 80x25 cells redrawn to uart with ANSI codes. Zero deps.
 * Port path to the phone: same cell API over a real writable fb (XRES via DPU).
 */
#include <creo/types.h>

#define COLS    80
#define ROWS    25

static char cells[ROWS][COLS];

void ui_putc(u64 r, u64 c, char ch)
{
    if (r < ROWS && c < COLS)
        cells[r][c] = ch;
}

void ui_puts(u64 r, u64 c, const char *s)
{
    for (usize i = 0; s[i] != '\0' && c + i < COLS; i++)
        ui_putc(r, c + i, s[i]);
}

static void ui_clear(void)
{
    for (u64 r = 0; r < ROWS; r++)
        for (u64 c = 0; c < COLS; c++)
            cells[r][c] = ' ';
}

void ui_paint(void)
{
    kputs_raw("\x1b[2J\x1b[H");
    for (u64 r = 0; r < ROWS; r++) {
        kputc_raw('|');
        for (u64 c = 0; c < COLS; c++)
            kputc_raw(cells[r][c]);
        kputc_raw('|');
        kputc_raw('\n');
    }
}

static const char *version = "CreoUI 0.1";

void ui_init(void)
{
    ui_clear();
    ui_puts(0,  2,   "CREO OS");
    ui_puts(0,  14,  version);
    ui_puts(0,  COLS - 10, "- NO BLOAT");
}
