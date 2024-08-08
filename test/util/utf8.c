#include <util/utf8.h>
#include <assert.h>
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_empty(void) {
    wchar_to_unicode(NULL, 0, NULL, 0);
    unicode_to_wchar(NULL, 0, NULL, 0);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    return UNITY_END();
}

