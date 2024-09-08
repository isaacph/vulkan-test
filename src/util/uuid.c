#if defined(_WIN32)

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

// Windows redefines uuid_t so we have to use a workaround to get our version here
#define WIN32_UUID_TYPE
#include "uuid.h"

uuid_alias generate_uuid() {
    UUID uuid;
    UuidCreate(&uuid);
    uuid_alias uuid_conv;
    uuid_conv.win32 = uuid;
    return uuid_conv;
}
#else
#error TODO: implement UUID gen on linux
#endif

// appends 4 characters to out using 2 bytes of input
static void append_short_hex(unsigned char* input, char* out) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            int v = (input[i] >> ((1 - j) * 4)) & 0xF;
            if (v >= 10) out[i * 2 + j] = v - 10 + 'a';
            else out[i * 2 + j] = v + '0';
        }
    }
}

// returns 0 if failed, else returns 36 (number of characters written)
int uuid_to_string(uuid_alias uuid, char* out, int out_len) {
    if (out_len < 36) return 0;
    int out_index = 0;
    for (int i = 0; i < 8; ++i) {
        if (2 <= i && i <= 5) {
            out[out_index++] = '-';
        }
        append_short_hex(uuid.data + i * 2, out + out_index);
        out_index += 4;
    }
    return out_index;
}
