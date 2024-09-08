#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED
#include <stdbool.h>
#include <stdint.h>

// checks a malloc pointer and returns it if it's not NULL. helper function
// seg faults and prints stack trace if it fails
void* checkMalloc(void* ptr);

// first memory management goal: deletion cache (deletes everything at the end)
typedef uint32_t sc_t;
static const sc_t SC_ID_NONE = UINT32_MAX;
typedef void (*CleanUpCallback)(void* user_ptr, sc_t id);
typedef struct CleanUpEntry {
    CleanUpCallback callback;
    void* user_ptr;
} CleanUpEntry;
typedef struct StaticCache {
    CleanUpEntry* entries;
    int size;
    int index;
} StaticCache;

void StaticCache_noop_func(void* user_ptr, sc_t id);
static const CleanUpCallback StaticCache_noop = StaticCache_noop_func;

StaticCache StaticCache_init(int size);
sc_t StaticCache_add(StaticCache* cache, CleanUpCallback callback, void* user_ptr);
// replaces the current cleanup callback at the position of id with a new one
// also calls the old one
// acts the same as StaticCache_add if id == SC_ID_NONE
sc_t StaticCache_put(StaticCache* cache, CleanUpCallback callback, void* user_ptr, sc_t id);
void StaticCache_clear(StaticCache* cache, sc_t id);
void StaticCache_clean_up(StaticCache* cache);

// // a stack structure whose memory footprint can be decided at runtime
// typedef struct RuntimeStack {
//     CleanUpCallback pop; // called by RuntimeStack_pop to destroy this whole stack
//     void* user_ptr;
//     void* start; // the start of the current stack frame
//     size_t size; // the capacity, so [start,start+size) is the range of memory available
//     size_t current; // the current position to allocate more memory from
// } RuntimeStack;
// 
// // init a stack backed by the given memory (so the RuntimeStack can actually be on the stack)
// RuntimeStack RuntimeStack_init_backed(void* backing_memory, size_t size, CleanUpCallback on_pop, void* user_ptr);
// RuntimeStack RuntimeStack_init_malloc(size_t size);
// 
// // push a smaller amount of memory reserved from the larger backing pool
// RuntimeStack RuntimeStack_push(RuntimeStack* base, size_t amount);
// 
// // after popping, accessing memory in the RuntimeStack is UB
// void RuntimeStack_pop(RuntimeStack* to_pop);

#endif // MEMORY_H_INCLUDED
