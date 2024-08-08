#ifndef WCHAR_H_INCLUDED
#define WCHAR_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

bool utf8_is_valid(const char* utf8, int len);
void replace_invalid_utf8(char* utf8, int len); // TODO if needed
bool utf8_to_wchar(const char* utf8, int len, wchar_t* out, int out_len);
void wchar_to_utf8(const wchar_t* wchar, int len, const char* out, int* out_len);

#endif // WCHAR_H_INCLUDED
