#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED
#include <stdbool.h>
#include <stdint.h>

// fuck I want unit testing or something to verify this works

typedef struct MemoryStack {
    void* memory;
    uint64_t memory_length;
    int stack_record_length;
    uint64_t* stack_record;
} MemoryStack;

MemoryStack initMemoryStack(void* memory, uint64_t memory_length);
void* memoryStackPush(MemoryStack* stack, uint64_t length);
void memoryStackPop(MemoryStack* stack);

#endif // MEMORY_H_INCLUDED
