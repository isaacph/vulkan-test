#include "utf.h"
#include <stdlib.h>
#include <assert.h>

static uint8_t const u8_length[] = {
// 0 1 2 3 4 5 6 7 8 9 A B C D E F
   1,1,1,1,1,1,1,1,0,0,0,0,2,2,3,4
};
#define u8length(s) u8_length[((uint8_t)(s)) >> 4];

// U+FFFD, the invalid character marker
static char const INVALID_UTF8[] = {
    0xEF, 0xBF, 0xBD
};
static uint8_t const INVALID_UTF8_LEN = 3;

// any valid UTF-8 encoding will fit into 4 bytes
// in fact, it will fit into 22 bits at the time of writing
typedef uint32_t utf8_t;
typedef uint32_t codepoint_t;

// validation functions
bool utf8_is_valid_at(const char* utf8, uint8_t length) {
    if (length == 0) {
        return false; // character was invalid
    }

    // condense to one 4-byte int
    utf8_t encoding = 0;
    for (int i = 0; i < length; ++i) {
        if (utf8[i] == '\0' && i > 0) {
            return false; // invalid null byte
        }
        encoding = (encoding << 8) | (utf8[i] & 0xff);
    }

    // validate all bits based on wikipedia description of UTF-8
    switch (length) {
    case 1:
        if ((encoding & 0x80) != 0) {
            return false;
        }
        break;
    case 2:
        if ((encoding & 0xE0C0) != 0xC080 || (encoding & 0x1E00) == 0x0000) {
            return false;
        }
        break;
    case 3:
        if ((encoding & 0xF0C0C0) != 0xE08080 || (encoding & 0x0F2000) == 0x000000) {
            return false;
        }
        break;
    case 4:
        if ((encoding & 0xF8C0C0C0) != 0xF0808080 || (encoding & 0x07300000) == 0x00000000) {
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
void assert_utf8_is_valid_at(const char* utf8, int length) {
#ifndef NDEBUG
    if (length == 0) {
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 char code");
        snprintf(msg, 1000, "UTF-8 is not valid. Invalid first bits for byte %x", utf8[0] & 0xff);
        exception_msg(msg);
        free(msg); // we never get here
    }

    // condense to one 4-byte int
    utf8_t encoding = 0;
    for (int i = 0; i < length; ++i) {
        if (utf8[i] == '\0' && i > 0) {
            exception_msg("UTF-8 is not valid. Invalid null byte in the middle of multi-byte character");
        }
        encoding = (encoding << 8) | (utf8[i] & 0xff);
    }

    // validate all bits based on wikipedia description of UTF-8
    switch (length) {
    case 1:
        if ((encoding & 0x80) != 0) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 1");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid byte %x (bit at 0x80 should be 0)",
                    utf8[0] & 0xff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        break;
    case 2:
        if ((encoding & 0xE0C0) != 0xC080) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 2");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %hx (%hx & 0xE0C0 should be 0xC080)",
                    encoding & 0xffff, encoding & 0xffff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        if ((encoding & 0x1E00) == 0) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, overlong 2-width");
            snprintf(msg, 1000, "UTF-8 is not valid. Overlong 2-width encoding %x",
                    encoding & 0xffff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        break;
    case 3:
        if ((encoding & 0xF0C0C0) != 0xE08080) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 3");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %x (%x & 0xF0C0C0 should be 0xE08080)",
                    encoding & 0xffffff, encoding & 0xffffff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        if ((encoding & 0x0F2000) == 0) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, overlong 3-width");
            snprintf(msg, 1000, "UTF-8 is not valid. Overlong 3-width encoding %x",
                    encoding & 0xffffff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        break;
    case 4:
        if ((encoding & 0xF8C0C0C0) != 0xF0808080) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, char length 4");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid bytes %x (%x & 0xF8C0C0C0 should be 0xF0808080)",
                    encoding, encoding);
            exception_msg(msg);
            free(msg); // we never get here
        }
        if ((encoding & 0x07300000) == 0) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, overlong 4-width");
            snprintf(msg, 1000, "UTF-8 is not valid. Overlong 4-width encoding %x",
                    encoding & 0xffffff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        break;
    }

    // reject utf-16 high surrogates (worked it out on paper)
    if ((encoding & 0xFFE0C0) == 0xEDA080) {
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 encoding, contained UTF-16 high surrogate");
        snprintf(msg, 1000, "UTF-8 is not valid. Invalid UTF-16 high surrogate %x (%x & 0xFFE0C0 should be 0xEDA080)",
                encoding, encoding);
        exception_msg(msg);
        free(msg); // we never get here
    }
#endif
}

bool utf8_is_valid(const char* utf8, int utf8_len) {
    int index = 0;
    while (index < utf8_len) {
        uint8_t length = u8length(utf8[index]);
        if (index + length > utf8_len) {
            return false;
        }
        if (!utf8_is_valid_at(utf8 + index, length)) {
            return false;
        }
        index += length; // to next byte
    }
    return true;
}

void assert_utf8_is_valid(const char* utf8, int len) {
#ifndef NDEBUG
    int index = 0;
    while (index < len) {
        uint8_t length = u8length(utf8[index]);
        if (index + length > len) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-8 length");
            snprintf(msg, 1000, "UTF-8 is not valid. Invalid character length %x", length & 0xff);
            exception_msg(msg);
            free(msg); // we never get here
        }
        assert_utf8_is_valid_at(utf8 + index, length);
        index += length;
    }
#endif
}

// remaining space is the amount of space left in the buffer to read from, while length is the amount of space
// the current character encoding requires. these are parameters to "replace_invalid" functions because in the case
// that there is not enough space left to read the rest of an encoding we want to output an invalid character and I decided
// to let that be handled by the "at" functions instead of the wrapper.
// output: you should advance utf8 by 1 if invalid, else by length
bool utf8_replace_invalid_at(const char* utf8, int length, int remaining_space, char* out, int out_buf_len, uint8_t* bytes_written) {
    bool invalid = false;
    const char* write_this;

    // check we can read
    if (length > remaining_space) {
        invalid = true;
        write_this = INVALID_UTF8;
        length = INVALID_UTF8_LEN;
    } else {
        if (utf8_is_valid_at(utf8, length)) {
            write_this = utf8;
        } else {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            invalid = true;
            write_this = INVALID_UTF8;
            length = INVALID_UTF8_LEN;
        }
    }

    // write bytes to out
    if (length > out_buf_len) {
        // we have no space left to emit characters
        *bytes_written = 0;
        return true;
    }
    memcpy((void*) out, write_this, length);
    *bytes_written = length;
    return invalid;
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
        if (out_index >= out_buf_len) {
            invalid = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);

        uint8_t bytes_written;
        bool current_invalid = utf8_replace_invalid_at(
                utf8 + in_index, length, utf8_len - in_index,
                out + out_index, out_buf_len - out_index,
                &bytes_written);
        invalid = invalid || current_invalid;

        in_index += current_invalid ? 1 : length;
        out_index += bytes_written;
        if (bytes_written == 0) {
            // no more space left in out
            break;
        }
    }
    out[out_index] = '\0';
    *out_len = out_index;
    return invalid;
}

void utf8_to_codepoint_unchecked_at(
        const char* utf8,
        int length,
        codepoint_t* out) {
    assert(length > 0);
    assert(length <= 4);
    assert_utf8_is_valid_at(utf8, length); // no-op outside debug mode

    // condense to one 4-byte int
    utf8_t encoding = 0;
    for (int i = 0; i < length; ++i) {
        assert(utf8[i] != '\0' || i == 0);
        encoding = (encoding << 8) | ((utf8_t) (utf8[i]) & 0xff);
    }

    // convert to unicode codepoint
    switch (length) {
    case 1:
        out[0] = encoding & 0x7F;
        break;
    case 2:
        out[0] = ((encoding & 0x1F00) >> 2) | (encoding & 0x3F);
        break;
    case 3:
        out[0] = ((encoding & 0xF0000) >> 4) | ((encoding & 0x3F00) >> 2) | (encoding & 0x3F);
        break;
    case 4:
        out[0] = ((encoding & 0x7000000) >> 6) | ((encoding & 0x3F0000) >> 4) | ((encoding & 0x3F00) >> 2) | (encoding & 0x3F);
        break;
    }
}
// returns true if it ran out of space to write to
// the main conversion implementation, does not check utf8 validity in release
bool utf8_to_codepoint_unchecked(const char* utf8, int in_len, codepoint_t* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool out_of_space = false;
    while (in_index < in_len) {
        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);
        assert(in_index + length <= in_len);
        utf8_to_codepoint_unchecked_at(utf8 + in_index, length, out + out_index);

        in_index += length;
        out_index += 1;
    }

    out[out_index] = '\0';
    *out_len = out_index;
    return out_of_space;
}

// returns true if conversion had errors, false if successful
// sets out_len to the resulting output string size, or to 0 if input string was invalid or empty
bool utf8_to_codepoint(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    if (!utf8_is_valid(utf8, len)) {
        *out_len = 0;
        return false;
    }
    return utf8_to_codepoint_unchecked(utf8, len, out, out_buf_len, out_len);
}
// returns true if it had to replace any invalid cahracters or ran out of output buffer size
bool utf8_to_codepoint_replace_invalid(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < len) {
        if (out_index >= out_buf_len) {
            invalid = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);
        char utf8_fixed[4];
        uint8_t utf8_fixed_len;
        bool current_invalid = utf8_replace_invalid_at(utf8 + in_index, length, len - in_index, utf8_fixed, 4, &utf8_fixed_len);
        invalid = invalid || current_invalid;
        utf8_to_codepoint_unchecked_at(utf8_fixed, utf8_fixed_len, out + out_index);
        if (current_invalid) {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            in_index += 1;
        } else {
            in_index += length;
        }
        out_index += 1;
    }
    out[out_index] = '\0';
    *out_len = out_index;
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
uint8_t codepoint_to_utf8_at(codepoint_t codepoint, char* out, int out_buf_len) {
    uint8_t length = codepoint_utf8_length(codepoint);
    if (length > out_buf_len) {
        return 0;
    }
    if (length == 1) {
        out[0] = codepoint & 0x7F;
        return length;
    }
    if (length == 2) {
        out[0] = 0xC0 | ((codepoint & 0x7C0) >> 6);
        out[1] = 0x80 | (codepoint & 0x3F);
        return length;
    }
    if (length == 3) {
        out[0] = 0xE0 | ((codepoint & 0xF000) >> 12);
        out[1] = 0x80 | ((codepoint & 0xFC0) >> 6);
        out[2] = 0x80 | (codepoint & 0x3F);
        return length;
    }
    if (length == 4) {
        out[0] = 0xF0 | ((codepoint & 0x1C0000) >> 18);
        out[1] = 0x80 | ((codepoint & 0x7F000) >> 12);
        out[2] = 0x80 | ((codepoint & 0xFC0) >> 6);
        out[3] = 0x80 | (codepoint & 0x3F);
        return length;
    }

    // invalid codepoint out of range
    char* msg = malloc(1000);
    if (!msg) exception_msg("Error while erroring, malloc, invalid unicode codepoint: too large for UTF-8");
    snprintf(msg, 1000, "Unicode codepoint is not valid. Codepoint %x is too large for UTF-8", codepoint & 0xff);
    exception_msg(msg);
    free(msg); // we never get here
    return length;
}
// returns true if it ran out of space to write to out
// SIGINT if any codepoints are out of range
bool codepoint_to_utf8(const codepoint_t* in, int in_len, char* out, int out_buf_len, int* out_len) {
    bool out_of_space = false;
    int out_index = 0;
    for (int in_index = 0; in_index < in_len; ++in_index) {
        codepoint_t codepoint = in[in_index];
        uint8_t length = codepoint_to_utf8_at(codepoint, out + out_index, out_buf_len - out_index);
        if (length == 0) {
            out_of_space = true;
            break;
        }
        out_index += length;
    }

    // write null byte for last character
    out[out_index] = '\0';
    *out_len = out_index;
    return out_of_space;
}

// returns true if output text is unusable (due to invalid input or no output buffer space)
bool utf8_to_utf16(const char* utf8, int utf8_len, wchar* out, int out_buf_len, int* out_len) {
    assert(utf8_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    if (!utf8_is_valid(utf8, utf8_len)) return true;
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf8_len) {
        if (out_index >= out_buf_len) {
            // out of space
            invalid = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);
        codepoint_t codepoint;
        utf8_to_codepoint_unchecked_at(utf8 + in_index, utf8_len, &codepoint);
        uint8_t out_length = codepoint_to_utf16_at(codepoint, out + out_index, out_buf_len - out_index);
        if(out_length == 0) {
            // out of space
            invalid = true;
            break;
        }
        in_index += length;
        out_index += out_length;
    }
    out[out_index] = 0;
    *out_len = out_index;
    return invalid;
}

// returns true if it ran out of space for output text
bool utf8_to_utf16_unchecked(const char* utf8, int utf8_len, wchar* out, int out_buf_len, int* out_len) {
    assert(utf8_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    assert_utf8_is_valid(utf8, utf8_len);
    bool out_of_space = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf8_len) {
        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);
        codepoint_t codepoint;
        utf8_to_codepoint_unchecked_at(utf8 + in_index, utf8_len, &codepoint);
        uint8_t out_length = codepoint_to_utf16_at(codepoint, out + out_index, out_buf_len - out_index);
        if (out_length == 0) {
            out_of_space = true;
            break;
        }
        in_index += length;
        out_index += out_length;
    }
    out[out_index] = 0;
    *out_len = out_index;
    return out_of_space;
}

// returns true there were any invalid or it ran out of space
bool utf8_to_utf16_replace_invalid(const char* utf8, int utf8_len, wchar* out, int out_buf_len, int* out_len) {
    assert(utf8_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf8_len) {
        if (out_index >= out_buf_len) {
            // out of space
            invalid = true;
            break;
        }
        uint8_t length = u8length(utf8[in_index]);
        char utf8_fixed[4];
        uint8_t utf8_fixed_len;
        codepoint_t codepoint;
        bool current_invalid = utf8_replace_invalid_at(utf8 + in_index, length, utf8_len - in_index, utf8_fixed, 4, &utf8_fixed_len);
        invalid = invalid || current_invalid;
        utf8_to_codepoint_unchecked_at(utf8_fixed, utf8_fixed_len, &codepoint);
        uint8_t out_length = codepoint_to_utf16_at(codepoint, out + out_index, out_buf_len - out_index);
        if (out_length == 0) {
            // out of space
            invalid = true;
            break;
        }
        if (current_invalid) {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            in_index += 1;
        } else {
            in_index += length;
        }
        out_index += out_length;
    }
    out[out_index] = '\0';
    *out_len = out_index;
    return invalid;
}

