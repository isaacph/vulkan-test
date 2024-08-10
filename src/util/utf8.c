#include "utf.h"

static uint8_t const u8_length[] = {
// 0 1 2 3 4 5 6 7 8 9 A B C D E F
   1,1,1,1,1,1,1,1,0,0,0,0,2,2,3,4
};
#define u8length(s) u8_length[((uint8_t *)(s))[0] >> 4];

// U+FFFD, the invalid character marker
static char const INVALID_UTF8[] = {
    0xEF, 0xBF, 0xBD
};
static uint8_t const INVALID_UTF8_LEN = 3;

// any valid UTF-8 encoding will fit into 4 bytes
// in fact, it will fit into 22 bits at the time of writing
typedef uint32_t utf8_t;
typedef uint32_t codepoint_t;

static const char test[] = "こ\343\201\223\0";

// validation functions
static bool utf8_is_valid_at(const char* utf8, int utf8_len, int index, uint8_t* out_char_length) {
    // get length of next utf8 character
    uint8_t length = u8length(utf8[index]);
    *out_char_length = length;
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

    // reject utf-16 high surrogates (worked it out on paper)
    if ((encoding & 0xFFE0C0) == 0xEDA080) {
        return false;
    }

    return true;
}
static void assert_utf8_is_valid_at(const char* utf8, int utf8_len, int index, uint8_t* out_char_length) {
    // get length of next utf8 character
    uint8_t length = u8length(utf8[index]);
    *out_char_length = length;
    if (index + length >= utf8_len) {
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
            snprintf(msg, 1000,
                    "UTF-8 is not valid. Invalid null byte in the middle of multi-byte character for byte at index %d",
                    index + i);
            exception_msg(msg);
            free(msg); // we never get here
        }
        encoding = (encoding << 8) | utf8[index + i];
    }

    // validate all bits based on wikipedia description of UTF-8
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

    // reject utf-16 high surrogates (worked it out on paper)
    if ((encoding & 0xFFE0C0) == 0xEDA080) {
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, contained UTF-16 high surrogate");
        snprintf(msg, 1000, "UTF-8 is not valid. Invalid UTF-16 high surrogate %d at index %d (%d & 0xFFE0C0 should be 0xEDA080)",
                encoding, index, encoding);
        exception_msg(msg);
        free(msg); // we never get here
    }
}


bool utf8_is_valid(const char* utf8, int utf8_len) {
    int index = 0;
    while (index < utf8_len) {
        uint8_t length;
        if (!utf8_is_valid_at(utf8, utf8_len, index, &length)) {
            return false;
        }
        index += length; // to next byte
    }
    return false;
}

void assert_utf8_is_valid(const char* utf8, int len) {
#ifndef NDEBUG
    int index = 0;
    while (index < len) {
        uint8_t length;
        assert_utf8_is_valid_at(utf8, len, index, &length);
        index += length; // to next byte
    }
#endif
}

// returns true if had to replace any or if it ran out of space
// out_len must be at least 3*len to guarantee all characters can be replaced if needed
// otherwise we will stop and return once we hit out_len
// out_len will contain the final length of out that was written to
bool utf8_replace_invalid(const char* utf8, int utf8_len, char* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool invalid = false;
    while (in_index < utf8_len) {
        // determine what to write
        uint8_t length;
        const char* write_this;
        if (utf8_is_valid_at(utf8, utf8_len, utf8_len, &length)) {
            write_this = out + out_index;
            in_index += length; // to next byte
        } else {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            invalid = true;
            write_this = INVALID_UTF8;
            length = INVALID_UTF8_LEN;
            in_index++;
        }

        // write bytes to out
        if (out_index + length > out_buf_len) {
            // we have no space left to emit characters
            invalid = true;
            break;
        }
        memcpy((void*) (out + out_index), write_this, length);
        out_index += length;
        continue;
    }

    *out_len = out_index;
    out[out_index] = '\0';
    return invalid;
}

// the main conversion implementation, does not check utf8 validity in release
// returns true if it ran out of space to write to out
bool utf8_to_codepoint_unchecked(const char* utf8, int in_len, codepoint_t* out, int out_buf_len, int* out_len) {
    assert_utf8_is_valid(utf8, in_len); // no-op outside debug mode

    bool out_of_space = false;
    int out_index = 0;
    int in_index = 0;
    while (in_index < in_len) {
        uint8_t length = u8length(utf8[in_index]);
        assert(length > 0);
        assert(in_index + length < in_len);

        // condense to one 4-byte int
        utf8_t encoding = 0;
        for (int i = 0; i < length; ++i) {
            assert(utf8[in_index + i] != '\0');
            encoding = (encoding << 8) | utf8[in_index + i];
        }
        in_index += length;

        // convert to unicode codepoint
        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        switch (length) {
            case 1:
                out[out_index] = encoding & 0x7F;
                break;
            case 2:
                out[out_index] = ((encoding & 0x1F00) >> 2) | (encoding & 0x3F);
                break;
            case 3:
                out[out_index] = ((encoding & 0xF0000) >> 4) | ((encoding & 0x3F) >> 2) | (encoding & 0x3F);
                break;
            case 4:
                out[out_index] = ((encoding & 0x7000000) >> 6) | ((encoding & 0x3F) >> 4) | ((encoding & 0x3F) >> 2) | (encoding & 0x3F);
                break;
        }
        out_index++;
    }
    out[out_index] = 0;
    *out_len = out_index;
    return out_of_space;
}

// returns true if conversion had errors, false if successful
// sets out_len to the resulting output string size, or to 0 if input string was invalid
bool utf8_to_codepoint(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    if (!utf8_is_valid(utf8, len)) {
        *out_len = 0;
        return false;
    }
    return utf8_to_wchar_unchecked(utf8, len, out, out_buf_len, out_len);
}
// auxiliary should have size equal to sizeof(utf8) * 3 + 1 to guarantee no characters are dropped
// returns true if it had to replace any invalid cahracters or ran out of output buffer size
bool utf8_to_codepoint_replace_invalid(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len, void* auxiliary, int aux_len) {
    bool invalid = false;
    char* utf8_fixed = (char*) auxiliary;
    int utf8_fixed_len = 0;

    invalid = utf8_replace_invalid(utf8, len, utf8_fixed, aux_len - 1, &utf8_fixed_len) || invalid;
    invalid = utf8_to_codepoint_unchecked(utf8_fixed, utf8_fixed_len, out, out_buf_len, out_len) || invalid;
    return invalid;
}

static uint8_t codepoint_utf8_length(codepoint_t codepoint) {
    if (codepoint <= 0x7F) {
        return 1;
    }
    if (codepoint <= 0x7FF) {
        return 2;
    }
    if (codepoint <= 0xFFFF) {
        return 3;
    }
    if (codepoint <= 0x10FFFF) {
        return 4;
    }
    return 0; // invalid
}
// returns true if it ran out of space to write to out
// SIGINT if any codepoints are out of range
bool codepoint_to_utf8(const codepoint_t* in, int in_len, char* out, int out_buf_len, int* out_len) {
    bool out_of_space = false;
    int out_index = 0;
    for (int in_index = 0; in_index < in_len; ++in_index) {
        codepoint_t codepoint = in[in_index];
        uint8_t length = codepoint_utf8_length(codepoint);
        if (out_index + length > out_buf_len) {
            out_of_space = true;
            break;
        }
        if (length == 1) {
            out[out_index + 0] = codepoint & 0x7F;
            out_index += 1;
            continue;
        }
        if (length == 2) {
            out[out_index + 0] = 0xC0 | ((codepoint & 0x7C0) >> 6);
            out[out_index + 1] = 0x80 | (codepoint & 0x3F);
            out_index += 2;
            continue;
        }
        if (length == 3) {
            out[out_index + 0] = 0xE0 | ((codepoint & 0xF000) >> 12);
            out[out_index + 1] = 0x80 | ((codepoint & 0xFC0) >> 6);
            out[out_index + 2] = 0x80 | (codepoint & 0x3F);
            out_index += 3;
            continue;
        }
        if (length == 4) {
            out[out_index + 0] = 0xF0 | ((codepoint & 0x1C0000) >> 18);
            out[out_index + 1] = 0x80 | ((codepoint & 0x7F000) >> 12);
            out[out_index + 2] = 0x80 | ((codepoint & 0xFC0) >> 6);
            out[out_index + 3] = 0x80 | (codepoint & 0x3F);
            out_index += 4;
            continue;
        }

        // invalid codepoint out of range
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid unicode codepoint: too large for UTF-8");
        snprintf(msg, 1000, "Unicode codepoint is not valid. Codepoint %d is too large for UTF-8 at index %d", codepoint, in_index);
        exception_msg(msg);
        free(msg); // we never get here
    }

    // write null byte for last character
    out[out_index] = '\0';
    *out_len = out_index;
    return out_of_space;
}
