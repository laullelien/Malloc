/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include "mem.h"
#include "mem_internals.h"
#include <assert.h>
#include <stdint.h>

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

void *emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    // On doit stocker size + 32 (marquage) + un pointeur pour le
    // chainage
    unsigned int p = puiss2(size + 32 + sizeof(void *));
    // On regarde s'il y a une possibilitée d'obtenir la taille nécessaire
    while (FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant <= p)
    {
        mem_realloc_medium();
        *(void **)arena.TZL[FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant - 1] = (void *)NULL;
    }
    // On regarde s'il existe un bloc de taille assez grande
    for (int i = p;
         i < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant; i++)
    {
        if (arena.TZL[i])
        {
            // On divise le bloc
            void *ptr = arena.TZL[i];
            arena.TZL[i] = (void *)(*(void **)(arena.TZL[i]));
            for (int j = i - 1; j >= p; j--)
            {
                void *cur_head = arena.TZL[j];
                arena.TZL[j] = ptr;
                *(void **)ptr = cur_head;
                ptr += (1 << j);
            }
            return mark_memarea_and_get_user_ptr(ptr + sizeof(void *), (1 << p) - sizeof(void *), MEDIUM_KIND);
        }
    }
    // Aucun bloc n'a été trouvé
    mem_realloc_medium();
    // On applique le même algorithme que plus haut
    void *ptr = arena.TZL[FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant - 1];
    *(void **)ptr = (void *)NULL;
    arena.TZL[FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant - 1] = (void *)NULL;
    for (int j = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant - 2; j >= p; j--)
    {
        void *cur_head = arena.TZL[j];
        arena.TZL[j] = ptr;
        *(void **)ptr = cur_head;
        ptr += (1 << j);
    }
    return mark_memarea_and_get_user_ptr(ptr + sizeof(void *), (1 << p) - sizeof(void *), MEDIUM_KIND);
}

void fuse(void *ptr, unsigned int p)
{
    assert(p < 48);
    if (p == 47)
    {
        void *cur_head = arena.TZL[p];
        arena.TZL[p] = ptr;
        *(void **)ptr = cur_head;
        return;
    }
    void *budd_ptr = (void *)((uint64_t)ptr ^ (1 << p));
    void *cur_ptr = arena.TZL[p];
    if (cur_ptr == NULL)
    {
        arena.TZL[p] = ptr;
        *(void **)ptr = (void *)NULL;
        return;
    }
    if (cur_ptr == budd_ptr)
    {
        arena.TZL[p] = *(void **)arena.TZL[p];
        if (ptr < budd_ptr)
            fuse(ptr, p + 1);
        else
            fuse(budd_ptr, p + 1);
        return;
    }
    while (*(void **)cur_ptr && *(void **)cur_ptr != budd_ptr)
    {
        cur_ptr = *(void **)cur_ptr;
    }
    if (*(void **)cur_ptr)
    {
        *(void **)cur_ptr = *(void **)budd_ptr;
        if (ptr < budd_ptr)
            fuse(ptr, p + 1);
        else
            fuse(budd_ptr, p + 1);
    }
    else
    {
        void *cur_head = arena.TZL[p];
        arena.TZL[p] = ptr;
        *(void **)ptr = cur_head;
    }
}

void efree_medium(Alloc a)
{
    assert(a.kind == MEDIUM_KIND);
    void *ptr = a.ptr - sizeof(void *);
    unsigned long size = a.size + sizeof(void *);
    unsigned int p = puiss2(size);
    assert(size == (1 << p));
    fuse(ptr, p);
}
