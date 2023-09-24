/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(uint64_t in)
{
    return in * (uint64_t)6364136223846793005 % (uint64_t)1442695040888963407;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    uint64_t taille = (uint64_t)size;
    *(uint64_t *)ptr = taille;
    *(uint64_t *)(ptr + size - 8) = taille;
    uint64_t magic = ((uint64_t)knuth_mmix_one_round((unsigned long)ptr) & (~(0b11UL))) + (uint64_t)k;
    *(uint64_t *)(ptr + 8) = magic;
    *(uint64_t *)(ptr + size - 16) = magic;
    return ptr + 16;
}

Alloc mark_check_and_get_alloc(void *ptr)
{
    Alloc a = {};
    a.ptr = ptr - 16;
    a.size = *(uint64_t *)(a.ptr);
    uint64_t magic = *(uint64_t *)(ptr - 8);
    a.kind = (MemKind)(magic & 0x00000003);

    uint64_t test = (uint64_t)knuth_mmix_one_round(((unsigned long)a.ptr) & (~(0b11UL)));
    test++;
    uint64_t test2 =  *(uint64_t *)(a.ptr + a.size - 8);
    test2++;

    assert(magic == (uint64_t)knuth_mmix_one_round(((unsigned long)a.ptr) & (~(0b11UL))) + (uint64_t)a.kind);
    assert(a.size == *(uint64_t *)(a.ptr + a.size - 8));
    assert(magic == *(uint64_t *)(a.ptr + a.size - 16));
    return a;
}

unsigned long
mem_realloc_small()
{
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
                           size,
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           -1,
                           0);
    if (arena.chunkpool == MAP_FAILED)
        handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium()
{
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert(size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
                             size * 2, // twice the size to allign
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1,
                             0);
    if (arena.TZL[indice] == MAP_FAILED)
        handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}

// used for test in buddy algo
unsigned int
nb_TZL_entries()
{
    int nb = 0;

    for (int i = 0; i < TZL_SIZE; i++)
        if (arena.TZL[i])
            nb++;

    return nb;
}
