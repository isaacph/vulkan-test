#ifndef UTIL_UUID_H_INCLUDED
#define UTIL_UUID_H_INCLUDED
#ifdef _WINDOWS_ 
#include <stdint.h>
#endif
#if defined(__linux__)
#include <uuid/uuid.h>
#endif

// for this class, we are basically defining the UUID type to be its own thing
// but if Windows is included we are making it a union with that type
typedef union union_uuid_t {
#if defined(__linux__)
    uuid_t data;
#else
    unsigned char data[16];
#endif
#ifdef _WINDOWS_ 
    UUID win32;
#endif
} uuid_alias;

uuid_alias generate_uuid();

// platform-independent way to print uuid
int uuid_to_string(uuid_alias uuid, char* out, int out_len);

#endif // UTIL_UUID_H_INCLUDED
