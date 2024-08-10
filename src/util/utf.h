#ifndef WCHAR_H_INCLUDED
#define WCHAR_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t codepoint_t;

// note: all functions will assume there is space to write a null byte at out[len] (one after the len-th character of out)

bool utf8_is_valid(const char* utf8, int utf8_len);
void assert_utf8_is_valid(const char* utf8, int len);
bool utf8_replace_invalid(const char* utf8, int in_len, char* out, int out_buf_len, int* out_len);
bool utf8_to_codepoint_unchecked(const char* utf8, int in_len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf8_to_codepoint(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf8_to_codepoint_replace_invalid(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len, void* auxiliary, int aux_len);

bool utf16_is_valid(const wchar_t* utf16, int utf16_len);
void assert_utf16_is_valid(const wchar_t* utf16, int utf16_len);
bool utf16_replace_invalid(const wchar_t* utf16, int utf16_len, wchar_t* out, int out_buf_len, int* out_len);
bool utf16_to_codepoint_unchecked(const wchar_t* utf16, int in_len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf16_to_codepoint(const char* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf16_to_codepoint_replace_invalid(
        const wchar_t* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len, void* auxiliary, int aux_len);

#endif // WCHAR_H_INCLUDED
