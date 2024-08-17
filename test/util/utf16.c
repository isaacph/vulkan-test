#include <util/utf.h>
#include <assert.h>
#include <unity.h>
#define SIZED(S) (S), (sizeof(S) - 1)

void setUp(void) {}
void tearDown(void) {}

typedef struct U {
    wchar buffer[1024];
    int length;
} U;
static U u(const char* buffer, int length) {
    U out;
    out.buffer[0] = 0;
    for (int i = 0; i < length; i += 2) {
        out.buffer[i/2] = ((buffer[i + 0] << 8) & 0xff00) | ((buffer[i + 1]) & 0xff);
    }
    out.length = length / 2;
    out.buffer[out.length] = 0;
    return out;
}
typedef struct C {
    codepoint_t buffer[1024];
    int length;
} C;
static C c(const char* buffer, int length) {
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

static bool wchar_equals_dbg(const wchar* a, const wchar* b, int length) {
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
static bool codepoint_equals_dbg(const codepoint_t* a, const codepoint_t* b, int length) {
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
    U emptyU = u(SIZED(""));
    C emptyC = c(SIZED(""));

    assert_utf16_is_valid(emptyU.buffer, emptyU.length);
    assert(utf16_is_valid(emptyU.buffer, emptyU.length));

    wchar buffer[1];
    codepoint_t codepoint_buffer[1];
    int out_len;

    *buffer = 1; out_len = 1;
    assert(!utf16_replace_invalid(emptyU.buffer, emptyU.length, buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf16_to_codepoint_replace_invalid(emptyU.buffer, emptyU.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf16_to_codepoint_unchecked(emptyU.buffer, emptyU.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *codepoint_buffer = 1; out_len = 1;
    assert(!utf16_to_codepoint(emptyU.buffer, emptyU.length, codepoint_buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);

    *buffer = 1; out_len = 1;
    assert(!codepoint_to_utf16(emptyC.buffer, emptyC.length, buffer, 0, &out_len));
    assert(out_len == 0);
    assert(*codepoint_buffer == 0);
}

void test_valid(void) {
    U tests[] = {
        u(SIZED("\0h\0e\0l\0l\0o")),
        u(SIZED("\0L\0o\0r\0e\0m\0 \0i\0p\0s\0u\0m\0 \0d\0o\0l\0o\0r\0 \0s\0i\0t\0 \0a\0m\0e\0t")),
        u(SIZED("\0\xD0\x02\xB0\x03\x01")),
        u(SIZED("\x08\x05\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A"
                "\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A"
                "\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A"
                "\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A\x08\x3A"
                "\x08\x3A\x08\x3A")),
        u(SIZED("\xD8\x00\xDD\x41\0j\0d\0i\xD8\x01\xDD\x35\0o\0f\0s\xD8\x01\xDD\x35")),
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
        printf("%d\n", i);
        codepoint_t codepoint_buffer[1025] = {0};
        wchar utf16_buffer[1025] = {0};
        int out_len = 0;
        // check valid functions
        assert_utf16_is_valid(tests[i].buffer, tests[i].length);
        assert(utf16_is_valid(tests[i].buffer, tests[i].length));

        // check utf16_replace_invalid 
        out_len = 0;
        utf16_buffer[0] = 0;
        assert(!utf16_replace_invalid(tests[i].buffer, tests[i].length, utf16_buffer, 1024, &out_len));
        assert(utf16_buffer[0] != 0);
        assert(wchar_equals_dbg(tests[i].buffer, utf16_buffer, tests[i].length));
        assert(tests[i].length == out_len);

        // check utf16_to_codepoint_replace_invalid
        out_len = 0;
        utf16_buffer[0] = 0;
        assert(!utf16_to_codepoint_replace_invalid(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check utf16_to_codepoint
        out_len = 0;
        codepoint_buffer[0] = 0;
        utf16_buffer[0] = 0;
        assert(!utf16_to_codepoint(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check utf16_to_codepoint_unchecked
        out_len = 0;
        codepoint_buffer[0] = 0;
        utf16_buffer[0] = 0;
        assert(!utf16_to_codepoint_unchecked(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(codepoint_equals_dbg(codepoints[i].buffer, codepoint_buffer, codepoints[i].length));
        assert(out_len == codepoints[i].length);

        // check codepoint_to_utf16
        assert(!codepoint_to_utf16(codepoints[i].buffer, codepoints[i].length, utf16_buffer, 1024, &out_len));
        assert(wchar_equals_dbg(utf16_buffer, tests[i].buffer, tests[i].length));
        assert(out_len == tests[i].length);
    }
}

void test_invalid(void) {
    U tests[] = {
        u(SIZED("\0h\0e\0l\0l\0o\0 \xD8\x00")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xDC\x00")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xD8\x00\xD8\x00")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xD8\x00\0\0")),
        u(SIZED("\xD8\x00\0\0\xD8\x00\0\0\xD8\x00\0\0\xD8\x00\0\0\xD8\x00\0\0")),
        u(SIZED("\xD8\x3F\xDF\xFF\xDF\xFF\xD8\x40\xDC\x00")),
    };
    U tests_fixed[] = {
        u(SIZED("\0h\0e\0l\0l\0o\0 \xFF\xFD")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xFF\xFD")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xFF\xFD\xFF\xFD")),
        u(SIZED("\0h\0e\0l\0l\0o\0 \xFF\xFD\0\0")),
        u(SIZED("\xFF\xFD\0\0\xFF\xFD\0\0\xFF\xFD\0\0\xFF\xFD\0\0\xFF\xFD\0\0")),
        u(SIZED("\xD8\x3F\xDF\xFF\xFF\xFD\xD8\x40\xDC\x00")),
    };
    int len = sizeof(tests) / sizeof(tests[0]);
    codepoint_t buffer[1] = {0};
    int out_len = 0;
    wchar out_buffer[1025] = {0};
    codepoint_t codepoint_buffer[1025] = {0};
    for (int i = 0; i < len; ++i) {
        printf("%d\n", i);
        out_len = 1;
        assert(!utf16_is_valid(tests[i].buffer, tests[i].length));
        assert(!utf16_to_codepoint(tests[i].buffer, tests[i].length, buffer, 0, &out_len));
        assert(out_len == 0);

        out_len = 1;
        assert(utf16_replace_invalid(tests[i].buffer, tests[i].length, out_buffer, 1024, &out_len));
        assert(wchar_equals_dbg(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
        assert(tests_fixed[i].length == out_len);

        out_len = 1;
        assert(utf16_to_codepoint_replace_invalid(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(out_len != 0);
        assert(!codepoint_to_utf16(codepoint_buffer, out_len, out_buffer, 1024, &out_len));
        assert(wchar_equals_dbg(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
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


