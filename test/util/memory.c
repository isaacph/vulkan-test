#include <unity.h>
#include "util/memory.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_memory_stack(void) {
    char memory[1024];
    MemoryStack obj = initMemoryStack(memory, 1024);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init_memory_stack);
    return UNITY_END();
}
