#include "myBigChars.h"

#include <stdio.h>

void bc_print_word_hex(int16_t value) {
    unsigned masked = (unsigned)value & 0x7FFFu;
    printf("====\n");
    printf("|%04X|\n", masked);
    printf("====\n");
}
