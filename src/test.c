#include <stdio.h>
#include "config.h"

int main() {
    printf("%s version %d.%d\n",
            PROJECT_NAME,
            PROJECT_VERSION_MAJOR,
            PROJECT_VERSION_MINOR
    );
    printf("%s\n\n", PROJECT_DESCRIPTION);
}
