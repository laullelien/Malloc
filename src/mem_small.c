/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

typedef struct _node
{
    void *ptr;
    struct _node *next;
} node;

void *
emalloc_small(unsigned long size)
{
    if (arena.chunkpool == NULL)
    {
        unsigned long size = mem_realloc_small();
        void *current_addr = arena.chunkpool;
        arena.chunkpool = malloc(sizeof(node));
        node *current_node = arena.chunkpool;
        current_node->ptr = current_addr;
        current_addr += CHUNKSIZE;
        size -= CHUNKSIZE;
        while (size >= CHUNKSIZE)
        {
            current_node->next = malloc(sizeof(node));
            current_node->ptr = current_addr;
            current_addr += CHUNKSIZE;
            size -= CHUNKSIZE;
            current_node = current_node->next;
        }
    }

    void *chunk_addr = ((node *)arena.chunkpool)->ptr;
    node *to_delete = arena.chunkpool;
    arena.chunkpool = ((node *)arena.chunkpool)->next;
    free(to_delete);

    return mark_memarea_and_get_user_ptr(chunk_addr, size + 32, SMALL_KIND);
}

void efree_small(Alloc a)
{
    assert(a.kind == SMALL_KIND);
    node *new_node = malloc(sizeof(node));
    new_node->ptr = a.ptr;
    new_node->next = arena.chunkpool;
    arena.chunkpool = new_node;
}
