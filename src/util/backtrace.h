#ifndef BACKTRACE_H_INCLUDED
#define BACKTRACE_H_INCLUDED

#define BOOST_STACKTRACE_USE_WINDBG
#include <stdbool.h>

void init_exceptions(bool threaded);
void exception_msg(const char* message);
void exception();

#endif
