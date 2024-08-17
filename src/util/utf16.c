#include "utf.h"
#include <assert.h>

// the invalid UTF-16 character marker (also equal to the codepoint in this case)
static const wchar INVALID_UTF16 = 0xFFFD;
static const int INVALID_UTF16_LEN = 1;

// returns 2 if c is an upper surrogate, 0 (invalid) if c is a lower surrogate, else 1
// returns the length of the unicode character (1 or 2 wchars)
static uint8_t utf16_length(wchar c) {
    wchar r = c & 0xFC00;
    if (r == 0xD800) {
        return 2; // upper surrogate
    }
    if (r == 0xDC00) {
        return 0; // invalid lower surrogate on first wchar
    }
    return 1;
}

bool utf16_is_valid_at(const wchar* utf16, int length) {
    assert(length >= 0);
    assert(length <= 2);
    switch (length) {
    case 0:
        return false;
    case 1:
        return true;
    case 2:
        // require lower surrogate format for second wchar
        if ((utf16[1] & 0xFC00) != 0xDC00) {
            return false;
        }
        // there are no overlong strings for utf-16
        return true;
    }
    // should never reach here
    return false;
}
void assert_utf16_is_valid_at(const wchar* utf16, int length) {
    assert(length >= 0);
    assert(length <= 2);
    switch (length) {
    case 0:
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-16 lower surrogate");
        snprintf(msg, 1000, "UTF-16 is not valid. Unexpected lower surrogate %d", utf16[0]);
        exception_msg(msg);
        free(msg); // we never get here
    case 1:
        return;
    case 2:
        // require lower surrogate format for second wchar
        if ((utf16[1] & 0xFC00) != 0xDC00) {
            printf("????\n");
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, second wchar was not a lower surrogate");
            snprintf(msg, 1000, "UTF-16 is not valid. Invalid lower surrogate %x", utf16[1]);
            exception_msg(msg);
            free(msg); // we never get here
        }
        // there are no overlongs for utf-16
        return;
    }
}

bool utf16_is_valid(const wchar* utf16, int utf16_len) {
    int index = 0;
    while (index < utf16_len) {
        uint8_t length = utf16_length(utf16[index]);
        if (index + length > utf16_len) {
            return false;
        }
        if (!utf16_is_valid_at(utf16, length)) {
            return false;
        }
        index += length;
    }
    return true;
}
void assert_utf16_is_valid(const wchar* utf16, int utf16_len) {
#ifndef NDEBUG
    int index = 0;
    while (index < utf16_len) {
        uint8_t length = utf16_length(utf16[index]);
        if (index + length > utf16_len) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, upper surrogate attempts string overflow");
            snprintf(msg, 1000, "UTF-16 is not valid. Upper surrogate %d attempts string overflow at index %d", utf16[index], index);
            exception_msg(msg);
            free(msg); // we never get here
        }
        assert_utf16_is_valid_at(utf16 + index, length);
        index += length;
    }
#endif
}

// the user should advance the input 1 if invalid, else advance by length
bool utf16_replace_invalid_at(const wchar* utf16, int length, int remaining_space, wchar* out, int out_buf_len, uint8_t* wchar_written) {
    bool invalid = false;
    // determine what to write
    const wchar* write_this;
    if (length > remaining_space) {
        invalid = true;
        write_this = &INVALID_UTF16;
        length = INVALID_UTF16_LEN;
    } else {
        if (utf16_is_valid_at(utf16, length)) {
            write_this = utf16;
        } else {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            invalid = true;
            write_this = &INVALID_UTF16;
            length = INVALID_UTF16_LEN;
        }
    }

    // write bytes to out
    if (length > out_buf_len) {
        // we have no space left to emit characters
        *wchar_written = 0;
        return true;
    }
    printf("writing %llu bytes, ", length * sizeof(wchar));
    memcpy((void*) out, write_this, length * sizeof(wchar));
    *wchar_written = length;
    return invalid;
}
// out_len must be >= len or else writing may stop prematurely
// returns true if had to replace any
bool utf16_replace_invalid(const wchar* utf16, int utf16_len, wchar* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool invalid = false;
    while (in_index < utf16_len) {
        if (out_index >= out_buf_len) {
            invalid = true;
            break;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        printf("index %d, char %x, length %d, ", in_index, utf16[in_index] & 0xffff, length);

        uint8_t bytes_written;
        bool current_invalid = utf16_replace_invalid_at(
                utf16 + in_index, length, utf16_len - in_index,
                out + out_index, out_buf_len - out_index,
                &bytes_written);
        printf("invalid %d, out %x, written %d\n", current_invalid, out[out_index] & 0xffff, bytes_written);
        invalid = invalid || current_invalid;

        // advance 1 if invalid, but
        // only advance 1 if the reason for being invalid is not related to length
        in_index += current_invalid && in_index + length <= utf16_len ? 1 : length;
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

void utf16_to_codepoint_unchecked_at(const wchar* utf16, int length, codepoint_t* out) {
    assert(length > 0);
    assert(length <= 2);
    assert_utf16_is_valid_at(utf16, length);

    switch (length) {
    case 1:
        out[0] = (codepoint_t) ((utf16[0] & 0xFFFF));
        break;
    case 2:
        out[0] = (((utf16[0] & 0x03FF) << 10) | (utf16[1] & 0x03FF)) + 0x10000;
        break;
    }
}
// the main conversion implementation, does not check utf16 validity in release
// returns true if it ran out of space to write to out
bool utf16_to_codepoint_unchecked(const wchar* utf16, int in_len, codepoint_t* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool out_of_space = false;
    while (in_index < in_len) {
        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        assert(in_index + length <= in_len);
        utf16_to_codepoint_unchecked_at(utf16 + in_index, length, out + out_index);

        in_index += length;
        out_index += 1;
    }

    out[out_index] = '\0';
    *out_len = out_index;
    return out_of_space;
}

// returns true if conversion was invalid, false if successful
bool utf16_to_codepoint(const wchar* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    if (!utf16_is_valid(utf16, len)) {
        *out_len = 0;
        return false;
    }
    return utf16_to_codepoint_unchecked(utf16, len, out, out_buf_len, out_len);
}
// remaining space is the amount of space left in the buffer to read from, while length is the amount of space
// the current character encoding requires. these are parameters to "replace_invalid" functions because in the case
// that there is not enough space left to read the rest of an encoding we want to output an invalid character and I decided
// to let that be handled by the "at" functions instead of the wrapper.
// output: you should advance utf8 by 1 if invalid, else by length
bool utf16_to_codepoint_replace_invalid(const wchar* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < len) {
        if (out_index >= out_buf_len) {
            return true;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        wchar utf16_fixed[2] = {0};
        uint8_t utf16_fixed_len;
        printf("index %d, char u0 %x u1 %x, length %d, ", in_index, utf16[in_index] & 0xffff, utf16[in_index + 1] & 0xffff, length);
        bool current_invalid = utf16_replace_invalid_at(utf16 + in_index, length, len - in_index, utf16_fixed, 2, &utf16_fixed_len);
        invalid = invalid || current_invalid;
        printf("u0 %x, u1 %x, length %d, ", utf16_fixed[0] & 0xffff, utf16_fixed[1] & 0xffff, length);
        utf16_to_codepoint_unchecked_at(utf16_fixed, utf16_fixed_len, out + out_index);
        printf("invalid %d, out %x\n", current_invalid, out[out_index]);
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

static uint8_t codepoint_utf16_length(codepoint_t codepoint) {
    if (codepoint <= 0xFFFF) {
        return 1;
    }
    if (codepoint <= 0x10FFFF) {
        return 2;
    }
    return 0; // invalid codepoint
}
uint8_t codepoint_to_utf16_at(codepoint_t codepoint, wchar* out, int out_buf_len) {
    uint8_t length = codepoint_utf16_length(codepoint);
    if (length > out_buf_len) {
        return 0; // out of space
    }
    if (length == 1) {
        out[0] = codepoint & 0xFFFF;
        return length;
    }
    if (length == 2) {
        out[0] = 0xD800 | (((codepoint - 0x10000) & 0xFFC00) >> 10);
        out[1] = 0xDC00 | ((codepoint - 0x10000) & 0x3FF);
        return length;
    }

    // invalid codepoint out of range
    char* msg = malloc(1000);
    if (!msg) exception_msg("Error while erroring, malloc, invalid unicode codepoint: too large for UTF-16");
    snprintf(msg, 1000, "Unicode codepoint is not valid. Codepoint %d is too large for UTF-16", codepoint);
    exception_msg(msg);
    free(msg); // we never get here
    return length;
}
// returns true if it ran out of space to write to out
// SIGINT if any codepoints are out of range
bool codepoint_to_utf16(const codepoint_t* in, int in_len, wchar* out, int out_buf_len, int* out_len) {
    bool out_of_space = false;
    int out_index = 0;
    for (int in_index = 0; in_index < in_len; ++in_index) {
        codepoint_t codepoint = in[in_index];
        uint8_t length = codepoint_to_utf16_at(codepoint, out + out_index, out_buf_len - out_index);
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
bool utf16_to_utf8(const wchar* utf16, int utf16_len, char* out, int out_buf_len, int* out_len) {
    assert(utf16_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    if (!utf16_is_valid(utf16, utf16_len)) return true;
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf16_len) {
        if (out_index >= out_buf_len) {
            invalid = true;
            break;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        codepoint_t codepoint;
        utf16_to_codepoint_unchecked_at(utf16 + in_index, utf16_len, &codepoint);
        uint8_t out_length = codepoint_to_utf8_at(codepoint, out + out_index, out_buf_len - out_index);
        if (out_length == 0) {
            // out of space
            invalid = true;
            break;
        }
        in_index += length;
        out_index += 1;
    }
    out[out_index] = 0;
    *out_len = out_index;
    return invalid;
}

// returns true if it ran out of space for output text
bool utf16_to_utf8_unchecked(const wchar* utf16, int utf16_len, char* out, int out_buf_len, int* out_len) {
    assert(utf16_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    assert_utf16_is_valid(utf16, utf16_len);
    bool out_of_space = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf16_len) {
        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        codepoint_t codepoint;
        utf16_to_codepoint_unchecked_at(utf16 + in_index, utf16_len, &codepoint);
        uint8_t out_length = codepoint_to_utf8_at(codepoint, out + out_index, out_buf_len - out_index);
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
bool utf16_to_utf8_replace_invalid(const wchar* utf16, int utf16_len, char* out, int out_buf_len, int* out_len) {
    assert(utf16_len >= 0);
    assert(out_buf_len >= 0);
    assert(out_len != NULL);
    bool invalid = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < utf16_len) {
        if (out_index >= out_buf_len) {
            // out of space
            invalid = true;
            break;
        }
        uint8_t length = utf16_length(utf16[in_index]);
        wchar utf16_fixed[2];
        uint8_t utf16_fixed_len;
        codepoint_t codepoint;
        bool current_invalid = utf16_replace_invalid_at(utf16 + in_index, length, utf16_len - in_index, utf16_fixed, 2, &utf16_fixed_len);
        invalid = invalid || current_invalid;
        utf16_to_codepoint_unchecked_at(utf16_fixed, utf16_fixed_len, &codepoint);
        uint8_t out_length = codepoint_to_utf8_at(codepoint, out + out_index, out_buf_len - out_index);
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
    out[out_index] = 0;
    *out_len = out_index;
    return invalid;
}

