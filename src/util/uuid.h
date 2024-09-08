#ifndef UTIL_UUID_H_INCLUDED
#define UTIL_UUID_H_INCLUDED
#include <stdint.h>

// for this class, we are basically defining the UUID type to be its own thing
// but if Windows is included we are making it a union with that type
typedef union union_uuid_t {
    unsigned char data[16];
#ifdef _WINDOWS_ 
    UUID win32;
#endif
} uuid_alias;

#ifndef _WINDOWS_ 
typedef union union_uuid_t uuid_t;
#endif
uuid_alias generate_uuid();

// platform-independent way to print uuid
int uuid_to_string(uuid_alias uuid, char* out, int out_len);

#endif // UTIL_UUID_H_INCLUDED
