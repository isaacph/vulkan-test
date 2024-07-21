#include "memory.h"
#include <stdio.h>

RuntimeStack RuntimeStack_init_backed(void* backing_memory, size_t size, CleanUpCallback on_pop, void* user_ptr) {
    return (RuntimeStack) {
        .user_ptr = user_ptr,
        .pop = on_pop,
        .start = backing_memory,
        .current = 0,
        .size = size,
    };
}

static void pop_top(void* user_ptr) {
    free(user_ptr);
}
RuntimeStack RuntimeStack_init_malloc(size_t size) {
    void* malloced_ptr = checkMalloc(malloc(size));
    return RuntimeStack_init_backed(malloc(size), size, pop_top, malloced_ptr);
}

RuntimeStack RuntimeStack_push(RuntimeStack* base, size_t size) {
    if (size > base->size - base->current) {
        fprintf(stderr, "Error allocating to stack: tried to allocate %llu but only %llu was remaining",
                size, base->size - base->current);
    }
    RuntimeStack next = {
        .user_ptr = NULL,
        .pop = NULL,
        .current = 0,
        .size = size,
        .start = base->start + base->current,
    };
    base->current += size;
    return next;
}

void RuntimeStack_pop(RuntimeStack* to_pop) {
    if (to_pop->pop != NULL) {
        to_pop->pop(to_pop->user_ptr);
    }
    // todo: implement the stack self-cleaning and defragmenting for long functions at one level
}
