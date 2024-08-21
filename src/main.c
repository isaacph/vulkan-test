#include "util/backtrace.h"
#include "winmain.h"
#include <stdio.h>

int main() {
#if defined(_WIN32)
    return winmain();
#endif
    printf("Init signals\n");
    init_exceptions(false);
    exception_msg("linux\n");
    printf("linux\n");
}
