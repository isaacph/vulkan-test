#include <unity.h>
#include <util/uuid.h>
#include <assert.h>
#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

void test_generate(void) {
    // uuid_t uuid = generate_uuid();
    // for (int i = 0; i < 16; ++i) {
    //     printf("%d", (int) uuid.data[i]);
    // }
    // printf("\n");
}

void test_to_string(void) {
    uuid_t uuid = generate_uuid();
    char str[37] = { 0 };
    assert(uuid_to_string(uuid, str, 36) == 36);
    printf("UUID: %s\n", str);
}

void test_to_string_more_space(void) {
    uuid_t uuid = generate_uuid();
    char str[39] = { 0 };
    assert(uuid_to_string(uuid, str, 38) == 36);
    printf("UUID: %s\n", str);
    assert(str[36] == 0);
    assert(str[37] == 0);
    assert(str[38] == 0);
}

void test_insufficient_space(void) {
    uuid_t uuid = generate_uuid();
    char str[37] = { 0 };
    assert(!uuid_to_string(uuid, str, 30));
    for (int i = 0; i < 37; ++i) {
        assert(str[i] == 0);
    }
    assert(!uuid_to_string(uuid, str, 0));
    for (int i = 0; i < 37; ++i) {
        assert(str[i] == 0);
    }
}


int main() {
    UNITY_BEGIN();
    RUN_TEST(test_generate);
    RUN_TEST(test_to_string);
    RUN_TEST(test_to_string_more_space);
    RUN_TEST(test_insufficient_space);
    return UNITY_END();
}

