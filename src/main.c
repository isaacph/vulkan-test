#include "winmain.h"

int main() {
#if defined(_WIN32)
    return winmain();
#endif
}
