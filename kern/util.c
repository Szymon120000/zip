/* CreoOS — the freestanding runtime we're allowed to provide ourselves */
#include <creo/types.h>

void *memcpy(void *d, const void *s, u64 n)
{
    u8 *a = (u8 *)d;
    const u8 *b = (const u8 *)s;
    while (n >= 8) {
        *(u64 *)a = *(const u64 *)b;
        a += 8, b += 8, n -= 8;
    }
    while (n)
        *a++ = *b++, n--;
    return d;
}

void *memset(void *d, int c, u64 n)
{
    u8 *a = (u8 *)d;
    while (n--)
        *a++ = (u8)c;
    return d;
}

void *pmem_alloc_pages(u64 n);
