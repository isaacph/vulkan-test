#include <util/memory.h>
#include <assert.h>
#include <stdlib.h>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void free_ptr(void* ptr, sc_t id) {
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

void test_add_1_sc_t_func(void* ptr, sc_t id) {
    *((bool*) ptr) = true;
}
void test_add_1_sc_t_func_2(void* ptr, sc_t id) {
    *((int*) ptr) += 1;
}
void test_add_1_sc_t(void) {
    StaticCache cache = StaticCache_init(1);
    bool val = false;
    int num = 0;
    sc_t id = StaticCache_add(&cache, test_add_1_sc_t_func, &val);
    assert(!val);
    assert(id == StaticCache_put(&cache, test_add_1_sc_t_func_2, &num, id));
    assert(val);
    assert(num == 0);
    StaticCache_clean_up(&cache);
    assert(num == 1);
}
void test_add_1_sc_t_none(void) {
    StaticCache cache = StaticCache_init(1);
    bool val = false;
    sc_t id = StaticCache_put(&cache, test_add_1_sc_t_func, &val, SC_ID_NONE);
    assert(!val);
    StaticCache_clean_up(&cache);
    assert(val);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_add_1);
    RUN_TEST(test_add_10);
    RUN_TEST(test_add_1_sc_t);
    RUN_TEST(test_add_1_sc_t_none);
    return UNITY_END();
}
