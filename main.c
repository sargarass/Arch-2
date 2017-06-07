#include <stdio.h>
#include "test.h"
#include <locale.h>
//(x + y - 1)/y * y округление к y

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("PANIC\n");
        exit(1);
    }

    run_test(argv[1], argv[2]);

    return 0;
}

