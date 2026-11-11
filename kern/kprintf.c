/* CreoOS — kprintf. 60 lines vs 2000. no floating point. no bloat. */
#include <stdarg.h>
#include <creo/types.h>

extern int kvsprintf(char *buf, usize cap, const char *fmt, va_list ap);

int kprintf(const char *fmt, ...)
{
    char buf[240];
    va_list ap;
    va_start(ap, fmt);
    int n = kvsprintf(buf, (int)sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && buf[i] != '\0'; i++)
        putc(buf[i]);
    if (n == (int)sizeof(buf) - 1)
        puts("[truncated]");
    return n;
}

int kvsprintf(char *buf, usize cap, const char *fmt, va_list ap)
{
    char *p = buf, *end = buf + cap - 1;
    for (const char *f = fmt; *f != '\0' && p < end; f++) {
        if (*f != '%') {
            *p++ = *f;
            continue;
        }
        f++;
        if (*f == '\0')
            break;
        int padz = 0, w10 = 0;
        while (*f == '0' || *f == '-') {
            if (*f == '0')
                padz = 1;
            f++;
        }
        while (*f >= '0' && *f <= '9') {
            w10 = w10 * 10 + (*f - '0');
            f++;
        }
        int lx = 0;
        while (*f == 'l')
            f++, lx++;
        char c = *f;
        u64 v;
        char t[24], n[2] = { 0, 0 };
        const char *src = t;
        usize len = 0;
        switch (c) {
        case 'c':
            n[0] = (char)va_arg(ap, int);
            src = n, len = 1;
            break;
        case 's':
            src = va_arg(ap, const char *);
            if (src == NULL)
                src = "(null)";
            while (src[len] != '\0')
                len++;
            break;
        case 'd':
        case 'u': {
            i64 sv = lx >= 2 ? va_arg(ap, i64)
                             : (c == 'u' ? (i64)va_arg(ap, unsigned int)
                                         : (i64)va_arg(ap, int));
            char *q = &t[sizeof(t) - 1];
            *q = '\0';
            u64 uv = sv < 0 ? (u64)0 - (u64)sv : (u64)sv;
            do {
                *--q = '0' + (char)(uv % 10);
                uv /= 10;
            } while (uv != 0);
            if (sv < 0 && q > t)
                *--q = '-';
            src = q;
            while (*q != '\0')
                q++;
            len = (usize)(q - src);
            break;
        }
        case 'x':
        case 'p':
            v = (c == 'p') ? va_arg(ap, u64)
                           : (lx >= 2 ? va_arg(ap, u64) : (u64)va_arg(ap, unsigned int));
            if (v == 0) {
                src = "0x0", len = 3;
                break;
            }
            {
                char *q = &t[sizeof(t) - 1];
                *q = '\0';
                while (v != 0) {
                    *--q = "0123456789abcdef"[v & 0xf];
                    v >>= 4;
                }
                src = q;
                while (*q != '\0')
                    q++;
                len = (usize)(q - src);
            }
            break;
        case '%':
            src = "%", len = 1;
            break;
        default:
            src = "?", len = 1;
            break;
        }
        if (w10 > (int)len && p < end)
            for (int s = 0; s < w10 - (int)len && p < end; s++)
                *p++ = padz ? '0' : ' ';
        for (usize i = 0; i < len && p < end; i++)
            *p++ = src[i];
    }
    *p = '\0';
    return (int)(p - buf);
}
