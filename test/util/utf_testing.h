#ifndef UTF_TESTING_H_INCLUDED
#define UTF_TESTING_H_INCLUDED
#include <util/utf.h>
#include <string.h>
#include <stdio.h>

typedef struct U8 {
    char buffer[1024];
    int length;
} U8;
static U8 u8(const char* buffer, int length) {
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

static void write_array(char* target, int length, char value) {
    for (int i = 0; i < length; ++i) {
        target[i] = value;
    }
}

static void write_array_cp(codepoint_t* target, int length, codepoint_t value) {
    for (int i = 0; i < length; ++i) {
        target[i] = value;
    }
}

static void write_array_wchar(wchar* target, int length, wchar value) {
    for (int i = 0; i < length; ++i) {
        target[i] = value;
    }
}

#endif // UTF_TESTING_H_INCLUDED
