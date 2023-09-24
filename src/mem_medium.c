/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size)
{
    unsigned int p = 0;
    size = size - 1; // allocation start in 0
    while (size)
    { // get the largest bit
        p++;
        size >>= 1;
    }
    if (size > (1 << p))
        p++;
    return p;
}

void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    unsigned int p = puiss2(size);
    for (int i = p; i <= FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant; i++)
    {
        if (arena.TZL[i])
        {

            return (void *)0;
        }
    }
    // realoc
    return (void *)0;
}

void efree_medium(Alloc a)
{
    /* ecrire votre code ici */
}
