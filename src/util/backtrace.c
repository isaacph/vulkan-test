#include "backtrace.h"
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#include <winnt.h>
#include <dbghelp.h>
#include <errhandlingapi.h>
#include <excpt.h>
#endif

#define WALK_LENGTH 100

void do_backtrace(bool fatal);
void force_interrupt();
void init_exceptions(bool threaded);

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
    fprintf(stderr, "Manual exception\n");
    force_interrupt();
}

void force_interrupt() {
    raise(SIGSEGV);
}

#if defined(__linux__)
#include <execinfo.h>

void do_backtrace(bool fatal) {
    printf("Doing backtrace\n");
    void * buffer = NULL;
    int addresses = backtrace(&buffer, 64);
    char ** symbols = backtrace_symbols(buffer, addresses);

    for (int i = 0; i < addresses; ++i) {
        char * symbol = symbols[i];
        printf("%s\n", symbol);
    }

    free(symbols);
    if (fatal) {
        exit(1);
    }
}
void sigHandler(int s) {
    fprintf(stderr, "Exception: %s (%d)\n", signal_name(s), s);

    do_backtrace(true);
}
void init_exceptions(bool threaded) {
    signal(SIGABRT, sigHandler);
    signal(SIGFPE, sigHandler);
    signal(SIGILL, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGSEGV, sigHandler);
    signal(SIGTERM, sigHandler);
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
