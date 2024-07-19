#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED
#include <stdbool.h>
#include <stdint.h>

// at some point I will start doing using some sort of memory management API
// but at the moment we will just suffer through malloc
//
// it's not real suffering, and honestly it shouldn't be that hard to replace later on if I do it right

// first memory management goal: deletion cache (deletes everything at the end)
typedef void (*CleanUpCallback)(void* user_ptr);
typedef struct CleanUpEntry {
    CleanUpCallback callback;
    void* user_ptr;
} CleanUpEntry;
typedef struct StaticCache {
    CleanUpEntry* entries;
    int size;
    int index;
} StaticCache;

StaticCache StaticCache_init(int size);
void StaticCache_add(StaticCache* cache, CleanUpCallback callback, void* user_ptr);
void StaticCache_clean_up(StaticCache* cache);

#endif // MEMORY_H_INCLUDED
