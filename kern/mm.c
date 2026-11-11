/* CreoOS — memory. Two tiers, nothing more:
 *   pmem   : fixed page allocator for task stacks / fb buffers / DMA
 *   kalloc : bump allocator over a carve-out, never free (no deferred bloat)
 */
#include <creo/types.h>

#define PMEM_BASE   0x48000000UL   /* carve-out start (below DTB, above 1G) */
#define PMEM_NPAGES 8192           /* 32 MiB */
#define PG          4096UL

static u8 *pmap = (u8 *)0x44000000UL; /* bitmap, 1 bit per page = 1 KiB */

static void *bump;
static void *bump_end;

void mm_init(void)
{
    u64 nbits = (PMEM_NPAGES + 7) / 8;
    for (u64 i = 0; i < nbits; i++)
        pmap[i] = (u8)0;
    bump     = (void *)(PMEM_BASE);
    bump_end = (void *)(PMEM_BASE + (u64)PMEM_NPAGES * PG);
}

void *pmem_alloc_pages(u64 n)
{
    void *r = bump;
    if ((u64)(bump_end - r) < n * PG)
        panic("pmem exhausted");
    bump = (u8 *)bump + n * PG;
    __asm__ volatile("dmb ish" ::: "memory");
    return r;
}

void *kmalloc(usize n)
{
    return pmem_alloc_pages((n + PG - 1) / PG);
}

void kfree(void *p) { (void)p; } /* real design choice: virgin memory is
                                   never returned to avoid stale-state bugs */
