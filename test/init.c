#include <stdio.h>
#include <render/context.h>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_init(void) {
    VkInstance instance = VK_NULL_HANDLE;

    {
    }

    printf("Hello from experiments\n");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init);
    return UNITY_END();
}

