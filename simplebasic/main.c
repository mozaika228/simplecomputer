#include "simplebasic.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: simplebasic <input.sb> <output.sa>\n");
        return 1;
    }
    if (sb_compile_file(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Simple Basic compile error\n");
        return 2;
    }
    return 0;
}
