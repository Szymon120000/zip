/* CreoOS — one file, all shared types. No bloat. */
#ifndef CREO_TYPES_H
#define CREO_TYPES_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef signed   char  i8;
typedef short          i16;
typedef int            i32;
typedef long           i64;
typedef unsigned long  usize;
typedef long           isize;

#define NULL ((void *)0)

static inline void barrier(void) { __asm__ volatile ("dsb sy" ::: "memory"); }
static inline void wfi(void)     { __asm__ volatile ("wfi"); }
static inline void dmb(void)     { __asm__ volatile ("dmb ish" ::: "memory"); }
static inline void intr_enable(void) { __asm__ volatile ("msr daifclr, #0xf"); }
static inline void intr_disable(void){ __asm__ volatile ("msr daifset, #0xf"); }

/* register-value read with a compile-time-name only. no runtime strings. */
#define RD64(reg) ({ u64 _v; __asm__ volatile("mrs %0, " #reg : "=r"(_v)); _v; })


#define KB (1024UL)
#define MB (1024UL * 1024)
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))

void  panic(const char *why) __attribute__((noreturn));

/* console */
void  puts(const char *s);
void  putc(char c);
void  putx(u64 v);
void  kputc_raw(char c);
void  kputs_raw(const char *s);
int   kprintf(const char *fmt, ...);
int   ksnprintf(char *buf, usize cap, const char *fmt, ...);

/* mm */
void  mm_init(void);
void *kmalloc(usize n);
void  kfree(void *p);

/* sched */
void  sched_init(void);
void  task_create(void (*entry)(void));
void  sched_run(void) __attribute__((noreturn));
void  yield(void);
void  sleep_ticks(u64 n);
u64   jiffies(void);

/* low-level plumbing */
void *pmem_alloc_pages(u64 n);
void *memcpy(void *d, const void *s, u64 n);
void *memset(void *d, int c, u64 n);

/* timer */
#define HZ 100
void  timer_init(void);
u64   ticks(void);
void  timer_rearm(void);

/* exceptions/intc */
void  gic_init(void);

/* ui */
void  ui_init(void);
void  ui_paint(void);
void  ui_puts(u64 r, u64 c, const char *s);

#endif
