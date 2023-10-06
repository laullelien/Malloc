/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"
#include <stdint.h>

void * emalloc_small(unsigned long size)
{
    if (arena.chunkpool == NULL)
    {
        // On realloc de la mémoire
        unsigned long size = mem_realloc_small();
        // On doit stocker un pointeur en plus
        void *ptr = arena.chunkpool;
        size -= CHUNKSIZE;
        while (size >= CHUNKSIZE)
        {
            *(void **)ptr = ptr + CHUNKSIZE;
            ptr = *(void **)ptr;
            size -= CHUNKSIZE;
        }

        // On met le dernier à null
        *(void **)ptr = (void *)NULL;
    }

    void *chunk_addr = arena.chunkpool;
    arena.chunkpool = *(void **)arena.chunkpool;
    return mark_memarea_and_get_user_ptr(chunk_addr, CHUNKSIZE, SMALL_KIND);
}

void efree_small(Alloc a)
{
    assert(a.kind == SMALL_KIND);
    void *prev = arena.chunkpool;
    arena.chunkpool = a.ptr;
    *(void **)arena.chunkpool = prev;
}
