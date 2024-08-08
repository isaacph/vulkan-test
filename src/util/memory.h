#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED
#include <stdbool.h>
#include <stdint.h>

// checks a malloc pointer and returns it if it's not NULL. helper function
// seg faults and prints stack trace if it fails
void* checkMalloc(void* ptr);

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
