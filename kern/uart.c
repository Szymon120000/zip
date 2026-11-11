/* CreoOS — PL011 uart (QEMU virt @0x09000000; beyond2lte boards remap via DT scan later) */
#include <creo/types.h>

#define UART_BASE 0x09000000UL
#define U(x)  (*(volatile u32 *)(UART_BASE + x))
#define UART_FCR  0x48
#define UART_LCR  0x2c
#define UART_CR   0x30
#define UART_IFLS 0x44
#define UART_IMSC 0x38
#define UART_FR   0x18

static char rendered[256];
static usize ridx;

void kputc_raw(char c)
{
    /* busy-drop, bounded retries; never wedges the cpu */
    for (unsigned i = 0; i < 64u && (U(UART_FR) & (1u << 5)); i++)
        __asm__ volatile("nop");
    if (U(UART_FR) & (1u << 5))
        return;
    U(0) = (u32)c;
}

void kputs_raw(const char *s)
{
    for (usize i = 0; s[i] != '\0'; i++)
        kputc_raw(s[i]);
}

static void uart_flush_line(void)
{
    rendered[ridx] = '\0';
    kputs_raw(rendered);
    if (ridx == 0 || rendered[ridx - 1] != '\n')
        kputc_raw('\n');
    ridx = 0;
}

void puts(const char *s)
{
    if (ridx) {
        uart_flush_line();
    }
    kputs_raw("~ ");
    kputs_raw(s);
    kputc_raw('\n');
}

void putc(char c)
{
    if (c == '\n') {
        if (ridx + 1 >= (int)sizeof(rendered)) { uart_flush_line(); return; }
        rendered[ridx++] = c;
        uart_flush_line();
        return;
    }
    if (ridx + 1 >= (int)sizeof(rendered)) { uart_flush_line(); return; }
    rendered[ridx++] = c;
}

void putx(u64 v)
{
    static const char hx[] = "0123456789abcdef";
    for (int i = 60; i >= 0; i -= 4)
        putc(hx[(v >> i) & 0xf]);
}

void uart_init(void)
{
    U(UART_CR)      = 0;
    U(UART_IFLS)    = 0;
    U(UART_IMSC)    = 0;
    U(UART_LCR)     = (3u << 5) | (1u << 4);
    U(UART_CR)      = (1u << 9) | (1u << 8) | (1u);
}
