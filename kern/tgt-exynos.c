/* CreoOS — early board init for Exynos 9820 (beyond2lte) flash test.
 * Minimal, defensive: probe known UART bases; print the banner wherever one
 * plays; DRV with bounded TXFF wait so a dead UART can never wedge us.
 */
#include <creo/types.h>

/* plausible sboot/debug UART bases (alphabetical-ish, tuned to exynos9820) */
static const u64 uart_bases[] = {
    0x13940000UL, 0x13900000UL, 0x13910000UL, 0x13920000UL,
    0x16340000UL, 0x162F0000UL,
};

static u64 found_uart;

static int uart_try(u64 b)
{
    volatile u32 *fr = (u32 *)(b + 0x18);
    volatile u32 *dr = (u32 *)(b + 0x0);
    /* FIFO enable, TX enable */
    *(volatile u32 *)(b + 0x100) = (3u << 12) | (1u); /* ULCON: 8n1 */
    *(volatile u32 *)(b + 0x0C) = (1u << 7) | (1u << 6); /* S3C2440 UFCON */
    *(volatile u32 *)(b + 0x08) = (1u << 2) | (1u << 3) | (1u << 0);
    for (unsigned i = 0; i < 100; i++) {
        if (!(*fr & (1u << 5))) /* TXFF clear → TX works */
        {
            *dr = (u32)'C';
            *dr = (u32)'R';
            *dr = (u32)'E';
            *dr = (u32)'O';
            found_uart = b;
            return 1;
        }
        for (u64 n = 0; n < 1000; n++)
            __asm__ volatile("nop");
    }
    return 0;
}

void exynos_uart_probe(void)
{
    for (unsigned i = 0; i < sizeof(uart_bases) / sizeof(uart_bases[0]); i++)
        if (uart_try(uart_bases[i]))
            return;
}

void exynos_uart_putc(char c)
{
    if (found_uart == 0)
        return;
    volatile u32 *fr = (u32 *)(found_uart + 0x18);
    volatile u32 *dr = (u32 *)(found_uart + 0x0);
    for (unsigned i = 0; i < 10000 && (*fr & (1u << 5)); i++)
        __asm__ volatile("nop");
    *dr = (u32)c;
}

void exynos_uart_puts(const char *s)
{
    for (usize i = 0; s[i] != '\0'; i++)
        exynos_uart_putc(s[i]);
}
