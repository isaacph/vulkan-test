#include <util/utf.h>
#include <assert.h>
#include <unity.h>
#define SIZED(S) (S), (sizeof(S) - 1)

void setUp(void) {}
void tearDown(void) {}

typedef struct U8 {
    char buffer[1024];
    int length;
} U8;
U8 u8(const char* buffer, int length) {
    U8 out;
    out.buffer[0] = 0;
    memcpy(out.buffer, buffer, length);
    out.length = length;
    return out;
}
typedef struct C {
    codepoint_t buffer[1024];
    int length;
} C;
C c(const char* buffer, int length) {
    C out;
    out.buffer[0] = 0;
    for (int i = 0; i < length; i += 4) {
        out.buffer[i/4] = ((buffer[i] << 24) & 0xff000000) | ((buffer[i + 1] << 16) & 0xff0000) |
            ((buffer[i + 2] << 8) & 0xff00) | ((buffer[i + 3]) & 0xff);
    }
    out.length = length / 4;
    out.buffer[out.length] = 0;
    return out;
}

bool codepoint_equals_dbg(const codepoint_t* a, const codepoint_t* b, int length) {
    for (int i = 0; i < length; ++i) {
        if (a[i] != b[i]) {
            printf("Left: ");
            for (int i = 0; i < length; ++i) {
                printf("%x ", a[i]);
            }
            printf("\nRight: ");
            for (int i = 0; i < length; ++i) {
                printf("%x ", b[i]);
            }
            printf("\n");
            return false;
        }
    }
    return true;
}

void test_empty(void) {
    U8 emptyU8 = u8(SIZED(""));
    C emptyC = c(SIZED(""));

    assert_utf8_is_valid(emptyU8.buffer, emptyU8.length);
    assert(utf8_is_valid(emptyU8.buffer, emptyU8.length));

    char buffer[1];
    codepoint_t codepoint_buffer[1];
    int out_len;

    *buffer = 1; out_len = 1;
    assert(!utf8_replace_invalid(emptyU8.buffer, emptyU8.length, buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf8_to_codepoint_replace_invalid(emptyU8.buffer, emptyU8.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf8_to_codepoint_unchecked(emptyU8.buffer, emptyU8.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf8_to_codepoint(emptyU8.buffer, emptyU8.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *buffer = 1; out_len = 1;
    assert(!codepoint_to_utf8(emptyC.buffer, emptyC.length, buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);
}

void test_valid(void) {
    U8 tests[] = {
        u8(SIZED("hello")),
        u8(SIZED("Lorem ipsum dolor sit amet")),
        u8(SIZED("Ðʰ́")),
        u8(SIZED("ࠅ࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺࠺")),
        u8(SIZED("𐅁jdi𐔵ofs𐔵")),
    };
    C codepoints[] = {
        c(SIZED("\0\0\0h\0\0\0e\0\0\0l\0\0\0l\0\0\0o")),
        c(SIZED("\0\0\0L\0\0\0o\0\0\0r\0\0\0e\0\0\0m\0\0\0 \0\0\0i\0\0\0p\0\0\0s\0\0\0u\0\0\0m"
                "\0\0\0 \0\0\0d\0\0\0o\0\0\0l\0\0\0o\0\0\0r\0\0\0 \0\0\0s\0\0\0i\0\0\0t\0\0\0 "
                "\0\0\0a\0\0\0m\0\0\0e\0\0\0t")),
        c(SIZED("\0\0\0\xD0\0\0\x02\xB0\0\0\x03\x01")),
        c(SIZED("\0\0\x08\x05\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A"
                "\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A"
                "\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A"
                "\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A\0\0\x08\x3A"
                "\0\0\x08\x3A\0\0\x08\x3A")),
        c(SIZED("\0\x01\x01\x41\0\0\0j\0\0\0d\0\0\0i\0\x01\x05\x35\0\0\0o\0\0\0f\0\0\0s\0\x01\x05\x35")),
    };
    int len = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < len; ++i) {
        printf("Testing %d\n", i);
        codepoint_t codepoint_buffer[1025] = {0};
        char utf8_buffer[1025] = {0};
        int out_len = 0;
        // check valid functions
        assert_utf8_is_valid(tests[i].buffer, tests[i].length);
        assert(utf8_is_valid(tests[i].buffer, tests[i].length));

        // check utf8_replace_invalid 
        out_len = 0;
        utf8_buffer[0] = 0;
        assert(!utf8_replace_invalid(tests[i].buffer, tests[i].length, utf8_buffer, 1024, &out_len));
        assert(utf8_buffer[0] != 0);
        assert(!strncmp(tests[i].buffer, utf8_buffer, tests[i].length));
        assert(tests[i].length == out_len);

        // check utf8_to_codepoint_replace_invalid
        out_len = 0;
        utf8_buffer[0] = 0;
        assert(!utf8_to_codepoint_replace_invalid(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check utf8_to_codepoint
        out_len = 0;
        codepoint_buffer[0] = 0;
        utf8_buffer[0] = 0;
        assert(!utf8_to_codepoint(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check utf8_to_codepoint_unchecked
        out_len = 0;
        codepoint_buffer[0] = 0;
        utf8_buffer[0] = 0;
        assert(!utf8_to_codepoint_unchecked(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check codepoint_to_utf8
        assert(!codepoint_to_utf8(codepoints[i].buffer, codepoints[i].length, utf8_buffer, 1024, &out_len));
        assert(!strncmp(utf8_buffer, tests[i].buffer, tests[i].length));
        assert(out_len == tests[i].length);
    }
}

void test_invalid(void) {
    U8 tests[] = {
        u8(SIZED("\200a")),
        u8(SIZED("b\377")),
        u8(SIZED("\377\377\377\377\377")),
        u8(SIZED("hello \301")),
        u8(SIZED("hello \311hi")),
        u8(SIZED("hello \312\300\312\300")),
        u8(SIZED("hello \340")),
        u8(SIZED("hello \340hi")),
        u8(SIZED("hello \340\300")),
        u8(SIZED("hello \360")),
        u8(SIZED("hello \360\0")),
        u8(SIZED("hello \360\200")),
        u8(SIZED("hello \360\200hi")),
        u8(SIZED("hello \360\200\300")),
        u8(SIZED("hello \370asdjfio")),
        u8(SIZED("hello \360\200\200")),
        u8(SIZED("hello \370\200\200\200")),
        u8(SIZED("hello \360\300\200\300\200\200")),
        u8(SIZED("hello \360\200\300\200\300\200")),
    };
    U8 tests_fixed[] = {
        u8(SIZED("\357\277\275a")),
        u8(SIZED("b\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275")),
        u8(SIZED("hello \357\277\275hi")),
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275")),
        u8(SIZED("hello \357\277\275hi")),
        u8(SIZED("hello \357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275")),
        u8(SIZED("hello \357\277\275\0")),
        u8(SIZED("hello \357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275\357\277\275hi")),
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275asdjfio")),
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275\300\200\300\200\357\277\275")),
        u8(SIZED("hello \357\277\275\357\277\275\300\200\300\200")),
    };
    int len = sizeof(tests) / sizeof(tests[0]);
    codepoint_t buffer[1] = {0};
    int out_len = 0;
    char out_buffer[1025] = {0};
    codepoint_t codepoint_buffer[1025] = {0};
    for (int i = 0; i < len; ++i) {
        printf("%d\n", i);
        out_len = 1;
        assert(!utf8_is_valid(tests[i].buffer, tests[i].length));
        assert(!utf8_to_codepoint(tests[i].buffer, tests[i].length, buffer, 0, &out_len));
        assert(out_len == 0);

        printf("utf8: ");
        for (int j = 0; j < tests[i].length; ++j) {
            printf("%x ", tests[i].buffer[j] & 0xff);
        }
        printf("\n");

        out_len = 1;
        assert(utf8_replace_invalid(tests[i].buffer, tests[i].length, out_buffer, 1024, &out_len));
        printf("utf8_replaced: ");
        for (int j = 0; j < out_len; ++j) {
            printf("%x ", out_buffer[j] & 0xff);
        }
        printf("\n");
        assert(!strncmp(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
        assert(tests_fixed[i].length == out_len);

        out_len = 1;
        assert(utf8_to_codepoint_replace_invalid(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(out_len != 0);
        printf("replaced_codepoint: ");
        for (int j = 0; j < out_len; ++j) {
            printf("%x ", codepoint_buffer[j]);
        }
        printf("\n");
        printf("len: %d\n", out_len);
        assert(!codepoint_to_utf8(codepoint_buffer, out_len, out_buffer, 1024, &out_len));
        for (int j = 0; j < tests_fixed[i].length; ++j) {
            printf("%x ", out_buffer[j] & 0xff);
        }
        printf("\n");
        for (int j = 0; j < tests_fixed[i].length; ++j) {
            printf("%x ", tests_fixed[i].buffer[j] & 0xff);
        }
        printf("\n");
        assert(!strncmp(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
        assert(out_len == tests_fixed[i].length);
    }
}

int main() {
    init_exceptions(false);
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_valid);
    RUN_TEST(test_invalid);
    return UNITY_END();
}

