#ifndef MY_TERM_H
#define MY_TERM_H

typedef enum {
    MT_BLACK = 30,
    MT_RED = 31,
    MT_GREEN = 32,
    MT_YELLOW = 33,
    MT_BLUE = 34,
    MT_MAGENTA = 35,
    MT_CYAN = 36,
    MT_WHITE = 37
} MTColor;

void mt_clrscr(void);
void mt_goto_xy(int row, int col);
void mt_set_fg(MTColor color);
void mt_set_bg(MTColor color);
void mt_reset_color(void);

#endif
