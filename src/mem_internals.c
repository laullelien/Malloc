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
#define MAGIC_MASK 0xfffffffffffffffc
#define MEMKIND_MASK 0x0000000000000003

unsigned long knuth_mmix_one_round(unsigned long in) {
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k) {
    uint64_t taille = (uint64_t)size;
    uint64_t magic =
        (knuth_mmix_one_round((unsigned long)ptr) & MAGIC_MASK) + (uint64_t)k;
    *(uint64_t *)ptr = taille;
    *(uint64_t *)(ptr + taille - 8) = taille;
    *(uint64_t *)(ptr + 8) = magic;
    *(uint64_t *)(ptr + taille - 16) = magic;
    return ptr + 16;
}

Alloc mark_check_and_get_alloc(void *ptr) {
    Alloc a = {};
    a.ptr = ptr - 16;
    uint64_t taille = *(uint64_t *)a.ptr;
    uint64_t magic = *(uint64_t *)(a.ptr + 8);
    a.kind = magic & MEMKIND_MASK;
    a.size = taille;
    assert(magic == (knuth_mmix_one_round((unsigned long)a.ptr) & MAGIC_MASK) +
                        (uint64_t)a.kind);
    assert(taille == *(uint64_t *)(a.ptr + taille - 8));
    assert(magic == *(uint64_t *)(a.ptr + taille - 16));
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
