#include "backtrace.h"
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <backtrace.h>

#if defined(_WIN32)
#include <minwindef.h>
#include <winnt.h>
#include <dbghelp.h>
#include <windows.h>
#include <errhandlingapi.h>
#include <excpt.h>
#endif

#define WALK_LENGTH 100

void do_backtrace(bool fatal);
void force_interrupt();

static const char* signal_name(int s) {
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

void exception_msg(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    force_interrupt();
}

void exception() {
    fprintf(stderr, "Unlabeled exception\n");
    force_interrupt();
}

void force_interrupt() {
    raise(SIGSEGV);
}

#if defined(__linux__)

struct backtrace_state* backtrace_state;

void exception_backtrace_error_callback(void* data, const char* msg, int errnum) {
    printf("Error creating backtrace\nErrno: %i\nMsg: %s\n\n", errnum, msg);
}

int exception_backtrace_full_callback(void* data, uintptr_t pc, const char* filename, int lineno, const char* function) {
    printf("%s:%s:%i at %p\n", filename, function, lineno, (void*) pc);
    return 0;
}

void sigAction(int signal, siginfo_t* info, void* ptr) {
    fprintf(stderr, "Exception: %s (%d)\n", signal_name(signal), signal);

    do_backtrace(true);
}

void init_exceptions(bool threaded) {
    int signals[] = {
        SIGABRT,
        SIGFPE,
        SIGILL,
        SIGINT,
        SIGSEGV,
        SIGTERM,
    };
    for (int i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        int signal = signals[i];
        struct sigaction action = {
            .sa_handler = NULL,
            .sa_sigaction = sigAction,
            .sa_mask = 0,
            .sa_flags = 0,
            .sa_restorer = NULL,
        };
        sigaction(signal, &action, NULL);
    }
    backtrace_state = backtrace_create_state(NULL, false, exception_backtrace_error_callback, NULL);
}

void do_backtrace(bool fatal) {
    backtrace_full(backtrace_state, 0, exception_backtrace_full_callback, exception_backtrace_error_callback, NULL);

    if (fatal) exit(1);
}
#endif


#if defined(_WIN32)

void sigHandler(int s) {
    fprintf(stderr, "Exception: %s (%d)\n", signal_name(s), s);

    do_backtrace(true);
}

LONG WINAPI WindowsExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo) {
    fprintf(stderr, "Caught Windows exception: %lu\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
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
    fprintf(stderr, "Backtrace:\n");

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
        fprintf(stderr, "Failed to initialize Win32 symbol reading. Error: %lu. Exiting\n", error);
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
            fprintf(stderr, "Failed to get symbol for frame %i, address: 0x%0llX. Error: %lu\n", i, addr, error);
            continue;
        }
        if(!SymGetLineFromAddr64(process, addr, &dwDisplacement, (PIMAGEHLP_LINE64) &line)) {
            DWORD error = GetLastError();
            fprintf(stderr, "Failed to get line for frame %i, address: 0x%0llX. Error: %lu\n", i, addr, error);
            continue;
        }

        fprintf(stderr, "%i: %s:%s:%lu - 0x%0llX\n", frames - i - 1, line.FileName, symbol->Name, line.LineNumber, symbol->Address);
    }

    free( symbol );
    SymCleanup(process);

    if (fatal) {
        exit(1);
    }
}
#endif
