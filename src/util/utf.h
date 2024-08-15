#ifndef WCHAR_H_INCLUDED
#define WCHAR_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t codepoint_t;

// note: all functions will assume there is space to write a null byte at out[len] (one after the len-th character of out)
// let's make the code testable cross platform (although we probably only need it for windows)
#if defined(_WIN32)
typedef wchar_t wchar;
#else
typdef int16_t wchar;
#endif

// utf8 code
bool utf8_is_valid_at(const char* utf8, uint8_t length);
bool utf8_is_valid(const char* utf8, int utf8_len);
void assert_utf8_is_valid_at(const char* utf8, int length);
void assert_utf8_is_valid(const char* utf8, int len);

bool utf8_replace_invalid_at(const char* utf8, int length, int remaining_space, char* out, int out_buf_len, uint8_t* bytes_written);
bool utf8_replace_invalid(const char* utf8, int utf8_len, char* out, int out_buf_len, int* out_len);
bool utf8_to_codepoint_replace_invalid(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len);

void utf8_to_codepoint_unchecked_at(const char* utf8, int length, codepoint_t* out);
bool utf8_to_codepoint_unchecked(const char* utf8, int in_len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf8_to_codepoint(const char* utf8, int len, codepoint_t* out, int out_buf_len, int* out_len);
uint8_t codepoint_to_utf8_at(codepoint_t codepoint, char* out, int out_buf_len);
bool codepoint_to_utf8(const codepoint_t* in, int in_len, char* out, int out_buf_len, int* out_len);

// utf16 code
bool utf16_is_valid_at(const wchar* utf16, int length);
bool utf16_is_valid(const wchar* utf16, int utf16_len);
void assert_utf16_is_valid_at(const wchar* utf16, int length);
void assert_utf16_is_valid(const wchar* utf16, int utf16_len);

bool utf16_replace_invalid_at(const wchar* utf16, int length, int remaining_space, wchar* out, int out_buf_len, uint8_t* wchar_written);
bool utf16_replace_invalid(const wchar* utf16, int utf16_len, wchar* out, int out_buf_len, int* out_len);
bool utf16_to_codepoint_replace_invalid(const wchar* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len);

void utf16_to_codepoint_unchecked_at(const wchar* utf16, int length, codepoint_t* out);
bool utf16_to_codepoint_unchecked(const wchar* utf16, int in_len, codepoint_t* out, int out_buf_len, int* out_len);
bool utf16_to_codepoint(const char* utf16, int len, codepoint_t* out, int out_buf_len, int* out_len);
uint8_t codepoint_to_utf16_at(codepoint_t codepoint, wchar* out, int out_buf_len);
bool codepoint_to_utf16(const codepoint_t* in, int in_len, wchar* out, int out_buf_len, int* out_len);

#endif // WCHAR_H_INCLUDED
