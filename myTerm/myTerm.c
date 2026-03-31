#include "myTerm.h"

#include <stdio.h>

void mt_clrscr(void) {
    printf("\033[2J\033[H");
}

void mt_goto_xy(int row, int col) {
    if (row < 1) row = 1;
    if (col < 1) col = 1;
    printf("\033[%d;%dH", row, col);
}

void mt_set_fg(MTColor color) {
    printf("\033[%dm", (int)color);
}

void mt_set_bg(MTColor color) {
    printf("\033[%dm", ((int)color) + 10);
}

void mt_reset_color(void) {
    printf("\033[0m");
}
