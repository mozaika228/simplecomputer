#include "simpleassembler.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: simpleassembler <input.sa> <output.sc>\n");
        return 1;
    }
    if (sa_assemble_file(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Assembler error\n");
        return 2;
    }
    return 0;
}
