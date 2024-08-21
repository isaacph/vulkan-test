#include "memory.h"
#include <assert.h>
#include <stdlib.h>
#include "backtrace.h"

void* checkMalloc(void* ptr) {
    if (ptr == NULL) {
        const char* msg = "Malloc returned NULL, out of memory!";
        exception_msg(ptr);
    }
    return ptr;
}

StaticCache StaticCache_init(int size) {
    assert(size > 0);
    size_t x = sizeof(CleanUpEntry) * size;
    void* memory = malloc(x);
    return (StaticCache) {
        .entries = (CleanUpEntry*) memory,
        .size = size,
        .index = 0,
    };
}
void StaticCache_add(StaticCache* cache, CleanUpCallback callback, void* user_ptr) {
    assert(callback != NULL);
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(cache->index >= 0);
    assert(cache->index < cache->size);

    cache->entries[cache->index] = (CleanUpEntry) {
        .callback = callback,
        .user_ptr = user_ptr,
    };
    cache->index++;
}
void StaticCache_clean_up(StaticCache* cache) {
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(cache->index >= 0);
    assert(cache->index <= cache->size);

    for (int i = cache->index - 1; i >= 0; --i) {
        CleanUpEntry entry = cache->entries[i];
        entry.callback(entry.user_ptr);
    }
    free(cache->entries);
    cache->index = -1;
    cache->size = 0;
    cache->entries = NULL;
}
