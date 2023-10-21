#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#ifdef _MSC_VER
#define MAX(a, b) (__max(a, b))
#define MIN(a, b) (__min(a, b))
#else
#define MAX(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define MIN(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})
#endif

#endif
