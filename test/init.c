#include <unity.h>
#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

void test_init(void) {
    printf("Hello from experiments\n");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init);
    return UNITY_END();
}

