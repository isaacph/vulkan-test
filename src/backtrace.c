#include <windows.h>
#include "backtrace.h"
#include <errhandlingapi.h>
#include <excpt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <minwindef.h>
#include <winnt.h>
#include <dbghelp.h>
#endif

#define WALK_LENGTH 100

void do_backtrace(bool fatal);
void force_interrupt();

void exception_msg(const char* message) {
    printf("Error: %s\n", message);
    force_interrupt();
}

void exception() {
    printf("Unlabeled exception\n");
    force_interrupt();
}

void force_interrupt() {
#if defined(_WIN32)
    raise(SIGSEGV);
#endif
}

#if defined(__linux__)
// don't free the return value
const char* get_executable_path() {
#if defined(_WIN32)
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/get-pgmptr?view=msvc-170
    return _pgmptr;
#else
    return NULL;
#endif
}

void exception_backtrace_error_callback(void* data, const char* msg, int errnum) {
    printf("Error creating backtrace\nErrno: %i\nMsg: %s\n\n", errnum, msg);
}

struct backtrace_state* backtrace_state;

int exception_backtrace_full_callback(void* data, uintptr_t pc, const char* filename, int lineno, const char* function) {
    printf("%s:%s:%i at %p\n", filename, function, lineno, (void*) pc);
    return 0;
}

void init_exceptions(bool threaded) {
    const char* executable = get_executable_path();
    printf("Found executable name: %s\n", executable);
    // this doesn't work
    // int len = strlen(executable) + 3;
    // char* executable2 = malloc(len);
    // snprintf(executable2, len, "%s.p", executable);
    // printf("Found executable name: %s\n", executable2);
    backtrace_state = backtrace_create_state(executable, threaded, exception_backtrace_error_callback, NULL);
}

void do_backtrace() {
    backtrace_full(
            backtrace_state,
            1,
            exception_backtrace_full_callback,
            exception_backtrace_error_callback,
            NULL);
    backtrace_print(backtrace_state, 1, stdout);
}
#endif


#if defined(_WIN32)

const char* signal_name(int s) {
    switch(s) {
        case SIGABRT: return "SIGABRT";
        case SIGFPE: return "SIGFPE";
        case SIGILL: return "SIGILL";
        case SIGINT: return "SIGINT";
        case SIGSEGV: return "SIGSEGV";
        case SIGTERM: return "SIGTERM";
        default: return "Unknown signal";
    }
}

void sigHandler(int s) {
    printf("Exception: %s (%d)\n", signal_name(s), s);

    do_backtrace(true);
}

LONG WINAPI WindowsExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo) {
    printf("Caught Windows exception: %lu\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
    do_backtrace(true);
    return EXCEPTION_CONTINUE_SEARCH;
}

void init_exceptions(bool threaded) {
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/signal?view=msvc-170
    signal(SIGABRT, sigHandler);
    signal(SIGFPE, sigHandler);
    signal(SIGILL, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGSEGV, sigHandler);
    signal(SIGTERM, sigHandler);
    SetUnhandledExceptionFilter(WindowsExceptionFilter);
}

// TODO: add thread sync since Win32 API doesn't support concurrency here
// once I start multithreading
void do_backtrace(bool fatal) {
    printf("Backtrace:\n");

    unsigned int   i;
    void         * stack[ WALK_LENGTH ];
    unsigned short frames;
    SYMBOL_INFO  * symbol;
    HANDLE         process;
    IMAGEHLP_LINE  line;
    DWORD          dwDisplacement;

    process = GetCurrentProcess();

    SymSetOptions(SYMOPT_LOAD_LINES);
    if (!SymInitialize( process, NULL, TRUE )) {
        DWORD error = GetLastError();
        printf("Failed to initialize Win32 symbol reading. Error: %lu. Exiting\n", error);
        exit(1);
    }

    frames               = CaptureStackBackTrace( 0, WALK_LENGTH, stack, NULL );
    symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    // skip the first frames
    for( i = 0; i < frames; i++ )
    {
        DWORD64 addr = (DWORD64)(stack[i]);
        if(!SymFromAddr( process, addr, 0, symbol)) {
            DWORD error = GetLastError();
            printf("Failed to get symbol for frame %i, address: 0x%0llX. Error: %lu\n", i, addr, error);
            continue;
        }
        if(!SymGetLineFromAddr64(process, addr, &dwDisplacement, (PIMAGEHLP_LINE64) &line)) {
            DWORD error = GetLastError();
            printf("Failed to get line for frame %i, address: 0x%0llX. Error: %lu\n", i, addr, error);
            continue;
        }

        printf("%i: %s:%s:%lu - 0x%0llX\n", frames - i - 1, line.FileName, symbol->Name, line.LineNumber, symbol->Address);
    }

    printf("Test4\n");
    free( symbol );
    SymCleanup(process);

    if (fatal) {
        exit(1);
    }
}
#endif
