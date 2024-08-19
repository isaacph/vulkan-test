#include <util/utf.h>
#include <assert.h>
#include <unity.h>
#include "utf_testing.h"
#define SIZED(S) (S), (sizeof(S) - 1)

void setUp(void) {}
void tearDown(void) {}

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

        // overlongs
        u8(SIZED("\300\200")),
        u8(SIZED("\301\277")),
        u8(SIZED("\340\200\200")),
        u8(SIZED("\340\201\277")),
        u8(SIZED("\340\202\200")),
        u8(SIZED("\340\237\277")),
        u8(SIZED("\360\200\200\200")),
        u8(SIZED("\360\200\201\277")),
        u8(SIZED("\360\200\202\200")),
        u8(SIZED("\360\200\237\277")),
        u8(SIZED("\360\200\240\200")),
        u8(SIZED("\360\217\277\277")),
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
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("hello \357\277\275\357\277\275\357\277\275\357\277\275\357\277\275\357\277\275")),

        // overlongs
        u8(SIZED("\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
        u8(SIZED("\357\277\275\357\277\275\357\277\275\357\277\275")),
    };
    int len = sizeof(tests) / sizeof(tests[0]);
    codepoint_t buffer[1] = {0};
    int out_len = 0;
    char out_buffer[1025] = {0};
    codepoint_t codepoint_buffer[1025] = {0};
    for (int i = 0; i < len; ++i) {
        out_len = 1;
        assert(!utf8_is_valid(tests[i].buffer, tests[i].length));
        assert(utf8_to_codepoint(tests[i].buffer, tests[i].length, buffer, 0, &out_len));
        assert(out_len == 0);

        out_len = 1;
        assert(utf8_replace_invalid(tests[i].buffer, tests[i].length, out_buffer, 1024, &out_len));
        assert(!strncmp(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
        assert(tests_fixed[i].length == out_len);

        out_len = 1;
        assert(utf8_to_codepoint_replace_invalid(tests[i].buffer, tests[i].length, codepoint_buffer, 1024, &out_len));
        assert(out_len != 0);
        assert(!codepoint_to_utf8(codepoint_buffer, out_len, out_buffer, 1024, &out_len));
        assert(!strncmp(out_buffer, tests_fixed[i].buffer, tests_fixed[i].length));
        assert(out_len == tests_fixed[i].length);
    }
}

void test_replace_limited_buffer(void) {
    U8 buffer = u8(SIZED("ab"));
    char out_limited_char[5];
    int out_len;
    assert_utf8_is_valid(buffer.buffer, buffer.length);

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 0, &out_len));
    assert(out_len == 0);
    assert(!strncmp(out_limited_char, "\0\0\0\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 1, &out_len));
    assert(out_len == 1);
    assert(!strncmp(out_limited_char, "a\0\0\0\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(!utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 2, &out_len));
    assert(out_len == 2);
    assert(!strncmp(out_limited_char, "ab\0\0\0", 5));

    buffer = u8(SIZED("\300\200"));
    assert(!utf8_is_valid(buffer.buffer, buffer.length));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 0, &out_len));
    assert(out_len == 0);
    assert(!strncmp(out_limited_char, "\0\0\0\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 1, &out_len));
    assert(out_len == 0);
    assert(!strncmp(out_limited_char, "\0\0\0\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 2, &out_len));
    assert(out_len == 0);
    assert(!strncmp(out_limited_char, "\0\0\0\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 3, &out_len));
    assert(out_len == 3);
    assert(!strncmp(out_limited_char, "\xEF\xBF\xBD\0", 5));

    out_len = -1;
    write_array(out_limited_char, 5, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char, 4, &out_len));
    assert(out_len == 3);
    assert(!strncmp(out_limited_char, "\xEF\xBF\xBD\0", 5));

    char out_limited_char_2[7];

    out_len = -1;
    write_array(out_limited_char_2, 7, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char_2, 5, &out_len));
    assert(out_len == 3);
    assert(!strncmp(out_limited_char_2, "\xEF\xBF\xBD\0\0\0", 7));

    out_len = -1;
    write_array(out_limited_char_2, 7, '\0');
    assert(utf8_replace_invalid(buffer.buffer, buffer.length, out_limited_char_2, 6, &out_len));
    assert(out_len == 6);
    assert(!strncmp(out_limited_char_2, "\xEF\xBF\xBD\xEF\xBF\xBD", 7));
}

void test_utf8_to_cp_limited_buffer(void) {
    U8 buffer = u8(SIZED("abcd"));
    codepoint_t out_limited[6];
    C comp;
    int out_len;
    assert_utf8_is_valid(buffer.buffer, buffer.length);

    out_len = -1;
    write_array_cp(out_limited, 5, 0);
    assert(utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 0, &out_len));
    assert(out_len == 0);
    comp = c(SIZED(""));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 5, 0);
    assert(utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 1, &out_len));
    assert(out_len == 1);
    comp = c(SIZED("\0\0\0a"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 5, 0);
    assert(utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 2, &out_len));
    assert(out_len == 2);
    comp = c(SIZED("\0\0\0a\0\0\0b"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 5, 0);
    assert(utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 3, &out_len));
    assert(out_len == 3);
    comp = c(SIZED("\0\0\0a\0\0\0b\0\0\0c"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 5, 0);
    assert(!utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 4, &out_len));
    assert(out_len == 4);
    comp = c(SIZED("\0\0\0a\0\0\0b\0\0\0c\0\0\0d"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 6, 0);
    assert(!utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 5, &out_len));
    assert(out_len == 4);
    comp = c(SIZED("\0\0\0a\0\0\0b\0\0\0c\0\0\0d"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    buffer = u8(SIZED("\xD0\x80"));
    out_len = -1;
    write_array_cp(out_limited, 6, 0);
    assert(utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 0, &out_len));
    assert(out_len == 0);
    comp = c(SIZED(""));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 6, 0);
    assert(!utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 1, &out_len));
    assert(out_len == 1);
    comp = c(SIZED("\0\0\x04\x00"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array_cp(out_limited, 6, 0);
    assert(!utf8_to_codepoint(buffer.buffer, buffer.length, out_limited, 2, &out_len));
    assert(out_len == 1);
    comp = c(SIZED("\0\0\x04\x00"));
    assert(codepoint_equals_dbg(out_limited, comp.buffer, comp.length + 1));
}

void test_cp_to_utf8_limited_buffer(void) {
    C buffer = c(SIZED("\0\0\0a\0\0\x04\x00\0\0\x0F\x00\0\x0F\x00\x00"));
    char out_limited[16];
    U8 comp;
    int out_len;

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 0, &out_len));
    assert(out_len == 0);
    comp = u8(SIZED(""));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 1, &out_len));
    assert(out_len == 1);
    comp = u8(SIZED("a"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 2, &out_len));
    assert(out_len == 1);
    comp = u8(SIZED("a"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 3, &out_len));
    assert(out_len == 3);
    comp = u8(SIZED("a\xD0\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 4, &out_len));
    assert(out_len == 3);
    comp = u8(SIZED("a\xD0\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 5, &out_len));
    assert(out_len == 3);
    comp = u8(SIZED("a\xD0\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 6, &out_len));
    assert(out_len == 6);
    comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 7, &out_len));
    assert(out_len == 6);
    comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 8, &out_len));
    assert(out_len == 6);
    comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 9, &out_len));
    assert(out_len == 6);
    comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    out_len = -1;
    write_array(out_limited, 16, '\0');
    assert(!codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, 10, &out_len));
    assert(out_len == 10);
    comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80\xF3\xB0\x80\x80"));
    assert(!strncmp(out_limited, comp.buffer, comp.length + 1));

    for (int i = 11; i <= 16; ++i) {
        out_len = -1;
        write_array(out_limited, 16, '\0');
        assert(!codepoint_to_utf8(buffer.buffer, buffer.length, out_limited, i, &out_len));
        assert(out_len == 10);
        comp = u8(SIZED("a\xD0\x80\xE0\xBC\x80\xF3\xB0\x80\x80"));
        assert(!strncmp(out_limited, comp.buffer, comp.length + 1));
    }
}

// most is_valid tests are covered above. add more tests here if is_valid diverges for cross
// conversion
void test_utf8_to_utf16_invalid(void) {
    wchar out[8] = {0};
    int len = -1;
    assert(utf8_to_utf16(SIZED("\xD0"), out, 8, &len));
    assert(len == 0);
    for (int i = 0; i < 8; ++i) assert(out[i] == 0);
}

void test_utf8_utf16_valid(void) {
    U8 tests[] = {
        u8(SIZED("")),
        u8(SIZED("Mary had a little lamb\r\n")),
        u8(SIZED("a\xD0\x80\xE0\xBC\x80\xF3\xB0\x80\x80")),
    };
    U expected[] = {
        u(SIZED("")),
        u(SIZED("\0M\0a\0r\0y\0 \0h\0a\0d\0 \0a\0 \0l\0i\0t\0t\0l\0e\0 \0l\0a\0m\0b\0\r\0\n")),
        u(SIZED("\0a\x04\x00\x0F\x00\xD8\x03\xDF\x00")),
    };
    for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        wchar out_u16[1025] = {0};
        char out_u8[1025] = {0};
        int out_len = -1;

        out_len = -1;
        write_array_wchar(out_u16, 1025, 0);
        assert(!utf8_to_utf16(tests[i].buffer, tests[i].length, out_u16, 1024, &out_len));
        assert(wchar_equals_dbg(out_u16, expected[i].buffer, expected[i].length));
        assert(out_len == expected[i].length);

        out_len = -1;
        write_array_wchar(out_u16, 1025, 0);
        assert(!utf8_to_utf16_unchecked(tests[i].buffer, tests[i].length, out_u16, 1024, &out_len));
        assert(wchar_equals_dbg(out_u16, expected[i].buffer, expected[i].length));
        assert(out_len == expected[i].length);

        out_len = -1;
        write_array(out_u8, 1025, 0);
        assert(!utf16_to_utf8(expected[i].buffer, tests[i].length, out_u8, 1024, &out_len));
        assert(!strncmp(out_u8, tests[i].buffer, tests[i].length));
        assert(out_len == tests[i].length);

        out_len = -1;
        write_array(out_u8, 1025, 0);
        assert(!utf16_to_utf8_unchecked(expected[i].buffer, tests[i].length, out_u8, 1024, &out_len));
        assert(!strncmp(out_u8, tests[i].buffer, tests[i].length));
        assert(out_len == tests[i].length);
    }
}

void test_utf8_to_utf16_replace(void) {
}

void test_utf8_to_utf16_limited_buffer(void) {
}

void test_utf8_to_utf16_replace_limited_buffer(void) {
}

int main() {
    init_exceptions(false);
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_valid);
    RUN_TEST(test_invalid);
    RUN_TEST(test_replace_limited_buffer);
    RUN_TEST(test_utf8_to_cp_limited_buffer);
    RUN_TEST(test_utf8_to_utf16_invalid);
    RUN_TEST(test_utf8_utf16_valid);
    RUN_TEST(test_utf8_to_utf16_replace);
    RUN_TEST(test_utf8_to_utf16_limited_buffer);
    RUN_TEST(test_utf8_to_utf16_replace_limited_buffer);
    return UNITY_END();
}

