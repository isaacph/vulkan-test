#include "utf.h"
#include <assert.h>

// the invalid UTF-16 character marker (also equal to the codepoint in this case)
static const wchar_t INVALID_UTF16 = 0xFFFD;
static const int INVALID_UTF16_LEN = 1;

// returns 2 if c is an upper surrogate, 0 (invalid) if c is a lower surrogate, else 1
// returns the length of the unicode character (1 or 2 wchars)
static uint8_t utf16_length(wchar_t c) {
    wchar_t r = c & 0xFC00;
    if (r == 0xD800) {
        return 2; // upper surrogate
    }
    if (r == 0xDC00) {
        return 0; // invalid lower surrogate on first wchar
    }
    return 1;
}

static bool utf16_is_valid_at(const wchar_t* utf16, int utf16_len, int index, uint8_t* out_char_length) {
    uint8_t length = utf16_length(utf16[index]);
    *out_char_length = length;
    switch (length) {
    case 0:
        return false;
    case 1:
        return true;
    case 2:
        // require no overflowing characters
        if (index + length > utf16_len) {
            return false;
        }

        // require lower surrogate format for second wchar
        if ((utf16[index + 1] & 0xFC00) != 0xDC00) {
            return false;
        }
        return true;
    }
    return true;
}
static void assert_utf16_is_valid_at(const wchar_t* utf16, int utf16_len, int index, uint8_t* out_char_length) {
    uint8_t length = utf16_length(utf16[index]);
    *out_char_length = length;
    switch (length) {
    case 0:
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid UTF-16 lower surrogate");
        snprintf(msg, 1000, "UTF-16 is not valid. Unexpected lower surrogate %d at index %d", utf16[index], index);
        exception_msg(msg);
        free(msg); // we never get here
    case 1:
        return;
    case 2:
        // require no overflowing characters
        if (index + length > utf16_len) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, upper surrogate attempts string overflow");
            snprintf(msg, 1000, "UTF-16 is not valid. Upper surrogate %d attempts string overflow at index %d", utf16[index], index);
            exception_msg(msg);
            free(msg); // we never get here
        }

        // require lower surrogate format for second wchar
        if ((utf16[index + 1] & 0xFC00) != 0xDC00) {
            char* msg = malloc(1000);
            if (!msg) exception_msg("Error while erroring, malloc, second wchar was not a lower surrogate");
            snprintf(msg, 1000, "UTF-16 is not valid. Invalid lower surrogate %d at index %d", utf16[index], index);
            exception_msg(msg);
            free(msg); // we never get here
        }
        return;
    }
}

bool utf16_is_valid(const wchar_t* utf16, int utf16_len) {
    int index = 0;
    while (index < utf16_len) {
        uint8_t length;
        if (!utf16_is_valid_at(utf16, utf16_len, index, &length)) {
            return false;
        }
        index += length;
    }
    return true;
}
void assert_utf16_is_valid(const wchar_t* utf16, int utf16_len) {
#ifndef NDEBUG
    int index = 0;
    while (index < utf16_len) {
        uint8_t length;
        assert_utf16_is_valid_at(utf16, utf16_len, index, &length);
        index += length;
    }
#endif
}
// out_len must be >= len or else writing may stop prematurely
// returns true if had to replace any
bool utf16_replace_invalid(const wchar_t* utf16, int utf16_len, wchar_t* out, int out_buf_len, int* out_len) {
    int in_index = 0;
    int out_index = 0;
    bool invalid = false;
    while (in_index < utf16_len) {
        // determine what to write
        const wchar_t* write_this;
        uint8_t length;
        if (utf16_is_valid_at(utf16, utf16_len, in_index, &length)) {
            write_this = out + out_index;
            in_index += length; // to next byte
        } else {
            // according to the unicode FAQ, when we hit an invalid multi-byte character, we
            // should drop the first byte and continue parsing at the second, replacing the byte with a marker
            invalid = true;
            write_this = &INVALID_UTF16;
            length = INVALID_UTF16_LEN;
            in_index++;
        }

        // write bytes to out
        if (out_index + length > out_buf_len) {
            // we have no space left to emit characters
            break;
        }
        memcpy((void*) (out + out_index), write_this, length * sizeof(wchar_t));
        out_index += length;
        continue;
    }

    *out_len = out_index;
    out[out_index] = '\0';
    return invalid;
}

// the main conversion implementation, does not check utf16 validity in release
// returns true if it ran out of space to write to out
bool utf16_to_codepoint_unchecked(const wchar_t* utf16, int in_len, codepoint_t* out, int out_buf_len, int* out_len) {
    bool out_of_space = false;
    int in_index = 0;
    int out_index = 0;
    while (in_index < in_len) {
        uint8_t length = utf16_length(utf16[in_index]);
        assert(length > 0);
        assert(in_index + length <= in_len);

        if (out_index >= out_buf_len) {
            out_of_space = true;
            break;
        }
        switch (length) {
        case 1:
            out[out_index] = utf16[in_index];
        case 2:
            out[out_index] = ((utf16[in_index] & 0x03FF) >> 16) | (utf16[in_index + 1] & 0x03FF);
        }
        out_index++;
    }

    out[out_index] = 0;
    *out_len = out_index;
    return out_of_space;
}

// returns true if conversion was invalid, false if successful
bool utf16_to_codepoint(const char* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len) {
    if (!utf8_is_valid(utf16, len)) {
        return false;
    }
    return utf8_to_wchar_unchecked(utf16, len, out, out_buf_len, out_len);
}
// auxiliary should have size equal to sizeof(utf16) + 1 to guarantee no characters are dropped
// returns true if it had to replace any invalid cahracters
bool utf16_to_codepoint_replace_invalid(
        const wchar_t* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len, void* auxiliary, int aux_len) {
    bool invalid = false;
    wchar_t* utf16_fixed = (wchar_t*) auxiliary;
    int utf16_fixed_len = 0;

    invalid = utf16_replace_invalid(utf16, len, utf16_fixed, aux_len - 1, &utf16_fixed_len) || invalid;
    invalid = utf16_to_codepoint_unchecked(utf16_fixed, utf16_fixed_len, out, out_buf_len, out_len) || invalid;
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
// returns true if it ran out of space to write to out
// SIGINT if any codepoints are out of range
bool codepoint_to_utf16(const codepoint_t* in, int in_len, wchar_t* out, int out_buf_len, int* out_len) {
    bool out_of_space = false;
    int out_index = 0;
    for (int in_index = 0; in_index < in_len; ++in_index) {
        codepoint_t codepoint = in[in_index];
        uint8_t length = codepoint_utf16_length(codepoint);
        if (out_index + length > out_buf_len) {
            out_of_space = true;
            break;
        }
        if (length == 1) {
            out[out_index + 0] = codepoint & 0xFFFF;
            out_index += 1;
        }
        if (length == 2) {
            out[out_index + 0] = (codepoint & 0xFFC00) >> 10;
            out[out_index + 1] = codepoint & 0x3FF;
            out_index += 1;
        }

        // invalid codepoint out of range
        char* msg = malloc(1000);
        if (!msg) exception_msg("Error while erroring, malloc, invalid unicode codepoint: too large for UTF-16");
        snprintf(msg, 1000, "Unicode codepoint is not valid. Codepoint %d is too large for UTF-16 at index %d", codepoint, in_index);
        exception_msg(msg);
        free(msg); // we never get here
    }
    out[out_index] = 0;
    *out_len = out_index;
    return out_of_space;
}
