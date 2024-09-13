#include "memory.h"
#include <assert.h>
#include <stdlib.h>
#include "backtrace.h"

void StaticCache_noop_func(void* user_ptr, sc_t id) {}

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
sc_t StaticCache_add(StaticCache* cache, CleanUpCallback callback, void* user_ptr) {
    assert(callback != NULL);
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(cache->index >= 0);
    assert(cache->index < cache->size);

    cache->entries[cache->index] = (CleanUpEntry) {
        .callback = callback,
        .user_ptr = user_ptr,
    };
    return cache->index++;
}
sc_t StaticCache_put(StaticCache* cache, CleanUpCallback callback, void* user_ptr, sc_t id) {
    if (id == SC_ID_NONE) {
        return StaticCache_add(cache, callback, user_ptr);
    }
    assert(callback != NULL);
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(id >= 0);
    assert(id < cache->size);
    assert(id < cache->index);

    CleanUpEntry entry = cache->entries[id];
    entry.callback(entry.user_ptr, (sc_t) id);
    cache->entries[id] = (CleanUpEntry) {
        .callback = callback,
        .user_ptr = user_ptr,
    };
    return id;
}
void StaticCache_clear(StaticCache* cache, sc_t id) {
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(id >= 0);
    assert(id < cache->size);
    assert(id < cache->index);

    cache->entries[id] = (CleanUpEntry) {
        .callback = StaticCache_noop,
        .user_ptr = NULL,
    };
}
void StaticCache_clean_up(StaticCache* cache) {
    assert(cache != NULL);
    assert(cache->entries != NULL);
    assert(cache->index >= 0);
    assert(cache->index <= cache->size);

    for (int i = cache->index - 1; i >= 0; --i) {
        CleanUpEntry entry = cache->entries[i];
        entry.callback(entry.user_ptr, (sc_t) i);
    }
    free(cache->entries);
    cache->index = -1;
    cache->size = 0;
    cache->entries = NULL;
}
