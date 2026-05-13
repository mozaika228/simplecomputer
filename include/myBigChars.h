#ifndef MY_BIG_CHARS_H
#define MY_BIG_CHARS_H

#include <stdint.h>

int bc_printA(const char *str);
int bc_box(int x1, int y1, int x2, int y2);
int bc_printbigchar(const int big[2], int x, int y, int fg, int bg);
int bc_setbigcharpos(int big[2], int x, int y, int value);
int bc_getbigcharpos(const int big[2], int x, int y, int *value);
int bc_bigcharwrite(int fd, const int *big, int count);
int bc_bigcharread(int fd, int *big, int need_count, int *count);

int bc_symbol(char c, int out[2]);
void bc_print_word_hex(int16_t value);
void bc_print_bigvalue(uint16_t value, int x, int y);

#endif
