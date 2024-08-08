#include "utf8.h"

static uint8_t const u8_length[] = {
// 0 1 2 3 4 5 6 7 8 9 A B C D E F
   1,1,1,1,1,1,1,1,0,0,0,0,2,2,3,4
};
#define u8length(s) u8_length[((uint8_t *)(s))[0] >> 4];

// U+FFFD, the invalid character marker
static uint8_t const INVALID_UTF8[] = {
    0xEF, 0xBF, 0xBD
};
static int const INVALID_UTF8_LEN = 3;

// any valid UTF-8 encoding will fit into 4 bytes
// in fact, it will fit into 22 bits at the time of writing
typedef uint32_t utf8_t;

static const char test[] = "こ\343\201\223\0";

bool utf8_is_valid(const char* utf8, int utf8_len) {
    int index = 0;
    while (index < utf8_len) {
        // get length of next utf8 character
        uint8_t length = u8length(utf8[index]);
        if (index + length >= utf8_len) {
            return false; // character was too long
        } else if (length == 0) {
            return false; // character was invalid
        }

        // condense to one 4-byte int
        utf8_t encoding = 0;
        for (int i = 0; i < length; ++i) {
            if (utf8[index + i] == '\0') {
                return false; // invalid null byte
            }
            encoding = (encoding << 8) | utf8[index + i];
        }

        // validate all bits based on wikipedia description of UTF-8
        switch (length) {
        case 1:
            if ((encoding & 0x80) != 0) {
                return false;
            }
            break;
        case 2:
            if ((encoding & 0xE0C0) != 0xC080) {
                return false;
            }
            break;
        case 3:
            if ((encoding & 0xF0C0C0) != 0xE08080) {
                return false;
            }
            break;
        case 4:
            if ((encoding & 0xF8C0C0C0) != 0xF0808080) {
                return false;
            }
            break;
        }

        index += length; // to next byte
    }
    return false;
}

void assert_utf8_is_valid(const char* utf8, int len) {
#ifndef NDEBUG
    int index = 0;
    while (index < len) {
        // assertion on calculated length
        uint8_t length = u8length(utf8[index]);
        if (index + length >= len) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 length");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid character length %d for character at index %d", length, index);
            exception_msg(msg);
            free(msg); // we never get here
        } else if (length == 0) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 char code");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid first bits for byte %d for at index %d", utf8[index], index);
            exception_msg(msg);
            free(msg); // we never get here
        }

        // condense to one 4-byte int
        utf8_t encoding = 0;
        for (int i = 0; i < length; ++i) {
            if (utf8[index + i] == '\0') {
                char* msg = malloc(1000);
                if (!msg) exception_msg("Error while erroring, malloc, invalid null byte mid-UTF-8");
                snprintf(msg, 1000, "UTF-8 is not valid. Invalid null byte in the middle of character for byte %d for at index %d",
                        utf8[i], index + i);
                exception_msg(msg);
                free(msg); // we never get here
            }
            encoding = (encoding << 8) | utf8[index + i];
        }

        // validate all bits
        switch (length) {
        case 1:
            if ((encoding & 0x80) != 0) {
                char* msg = malloc(1000);
                if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 1");
                snprintf(msg, 1000, "UTF-8 is not valid. Invalid byte %d (bit at 0x80 should be 0) at index %d",
                        utf8[index], index);
                exception_msg(msg);
                free(msg); // we never get here
            }
            break;
        case 2:
            if ((encoding & 0xE0C0) != 0xC080) {
                char* msg = malloc(1000);
                if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 2");
                snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %d at index %d (%d & 0xE0C0 should be 0xC080)",
                        encoding, index, encoding);
                exception_msg(msg);
                free(msg); // we never get here
            }
            break;
        case 3:
            if ((encoding & 0xF0C0C0) != 0xE08080) {
                char* msg = malloc(1000);
                if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 3");
                snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %d at index %d (%d & 0xF0C0C0 should be 0xE08080)",
                        encoding, index, encoding);
                exception_msg(msg);
                free(msg); // we never get here
            }
            break;
        case 4:
            if ((encoding & 0xF8C0C0C0) != 0xF0808080) {
                char* msg = malloc(1000);
                if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 4");
                snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %d at index %d (%d & 0xF8C0C0C0 should be 0xF0808080)",
                        encoding, index, encoding);
                exception_msg(msg);
                free(msg); // we never get here
            }
            break;
        }

        index += length; // to next byte
    }
#endif
}

// helper function for below
static bool utf8_add_invalid(char* out, int out_len, int* out_index) {
    if (*out_index + INVALID_UTF8_LEN > out_len) {
        return false;
    }
    memcpy((void*) INVALID_UTF8, out + *out_index, INVALID_UTF8_LEN);
    *out_index += 3;
    return true;
}
// returns true if had to replace any
// out_len must be at least 3*len to guarantee all characters can be replaced if needed
// otherwise we will stop and return once we hit out_len
// out_len will contain the final length of out that was written to
bool utf8_replace_invalid(const char* utf8, int in_len, char* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool invalid = false;
    while (in_index < in_len) {
        uint8_t length = u8length(utf8[in_index]);
        if (in_index + length >= in_len) {
            in_index = in_len; // ensure we don't continue processing afterwards
            goto invalid_char;
        } else if (length == 0) {
            goto invalid_char;
        }

        // condense to one 4-byte int
        utf8_t encoding = 0;
        for (int i = 0; i < length; ++i) {
            if (utf8[in_index + i] == '\0') {
                goto invalid_char;
            }
            encoding = (encoding << 8) | utf8[in_index + i];
        }

        // validate all bits
        switch (length) {
        case 1:
            if ((encoding & 0x80) != 0) {
                goto invalid_char;
            }
            break;
        case 2:
            if ((encoding & 0xE0C0) != 0xC080) {
                goto invalid_char;
            }
            break;
        case 3:
            if ((encoding & 0xF0C0C0) != 0xE08080) {
                goto invalid_char;
            }
            break;
        case 4:
            if ((encoding & 0xF8C0C0C0) != 0xF0808080) {
                goto invalid_char;
            }
            break;
        }

        // write bits to out
        memcpy((void*) (out + out_index), utf8 + in_index, length);
        in_index += length; // to next byte
        continue;

        // jump here when we have hit an invalid character
        // according to the unicode FAQ, when we hit an invalid multi-byte character, we
        // should drop the first byte and continue parsing at the second, replacing the byte with a marker
        invalid_char:
        invalid = true;
        if (out_index + INVALID_UTF8_LEN > out_buf_len) {
            // we have no space left to emit characters
            *out_len = out_index;
            return invalid;
        }
        memcpy((void*) INVALID_UTF8, out + out_index, INVALID_UTF8_LEN);
        out_index += INVALID_UTF8_LEN;
        in_index++;
    }
    *out_len = out_index;
    return invalid;
}

// the main conversion implementation, does not check utf8 validity in release
// returns true if it ran out of space to write to out
bool utf8_to_wchar_unchecked(const char* utf8, int in_len, wchar_t* out, int out_len) {
    assert_utf8_is_valid(utf8, in_len); // no-op outside debug mode

    int out_index = 0;
    int in_index = 0;
    while (in_index < in_len) {
        int length = u8length(utf8[in_index]);
        assert(length > 0);
        assert(in_index + length < in_len);

        // condense to one 4-byte int
        utf8_t encoding = 0;
        for (int i = 0; i < length; ++i) {
            assert(utf8[in_index + i] != '\0');
            encoding = (encoding << 8) | utf8[in_index + i];
        }

        // convert to unicode codepoint
    }
    // 011 10001101 00000010 10010011
    const int v = u8length(test);
    printf("v: %d\n", v);

    const unsigned char* ptr = (unsigned char*) test;
    while (*ptr != '\0') {
        printf("%d, ", *ptr);
        ++ptr;
    }
    printf("\n");
    printf("valid? %d\n", utf8_is_valid(test, sizeof(test)));
    return false;
}

// returns true if conversion was invalid, false if successful
bool utf8_to_wchar(const char* utf8, int len, wchar_t* out, int out_len) {
    if (!utf8_is_valid(utf8, len)) {
        return false;
    }
    return utf8_to_wchar_unchecked(utf8, len, out, out_len);
}
// auxiliary should have size equal to sizeof(utf8) * 3 to guarantee no characters are dropped
// returns true if it had to replace any invalid cahracters
bool utf8_to_wchar_replace_invalid(const char* utf8, int len, wchar_t* out, int out_len, void* auxiliary, int aux_len) {
    bool invalid = false;
    char* utf8_fixed = (char*) auxiliary;
    int utf8_fixed_len = 0;

    invalid = invalid || utf8_replace_invalid(utf8, len, utf8_fixed, aux_len, &utf8_fixed_len);
    invalid = invalid || utf8_to_wchar_unchecked(utf8_fixed, utf8_fixed_len, out, out_len);
    return invalid;
}

void wchar_to_utf8(const wchar_t* wchar, int len, const char* out, int out_len) {
}

