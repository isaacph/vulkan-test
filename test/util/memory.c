#include <util/memory.h>
#include <assert.h>
#include <stdlib.h>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void free_ptr(void* ptr) {
    assert(ptr != NULL);
    free(ptr);
}

void test_empty(void) {
    StaticCache cache = StaticCache_init(10);
    StaticCache_clean_up(&cache);
}

void test_add_1(void) {
    StaticCache cache = StaticCache_init(1);
    StaticCache_add(&cache, free_ptr, malloc(1));
    StaticCache_clean_up(&cache);
}

void test_add_10(void) {
    StaticCache cache = StaticCache_init(100);
    for (int i = 0; i < 10; ++i) {
        StaticCache_add(&cache, free_ptr, malloc(1));
    }
    StaticCache_clean_up(&cache);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_add_1);
    RUN_TEST(test_add_10);
    return UNITY_END();
}
