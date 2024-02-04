#include "memory.h"

MemoryStack initMemoryStack(void* memory, uint64_t memory_length) {
    MemoryStack stack = {
        .memory = memory,
        .memory_length = memory_length,
        .stack_record_length = 0,
        .stack_record = NULL,
    };
    return stack;
}
void* memoryStackPush(MemoryStack* stack, uint64_t length) {
    return NULL;
}
void memoryStackPop(MemoryStack* stack) {
}
