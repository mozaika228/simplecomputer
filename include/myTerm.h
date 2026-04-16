#ifndef MY_TERM_H
#define MY_TERM_H

#include <stddef.h>

typedef enum {
    MT_COLOR_BLACK = 0,
    MT_COLOR_RED = 1,
    MT_COLOR_GREEN = 2,
    MT_COLOR_YELLOW = 3,
    MT_COLOR_BLUE = 4,
    MT_COLOR_MAGENTA = 5,
    MT_COLOR_CYAN = 6,
    MT_COLOR_WHITE = 7
} mt_color;

int mt_clrscr(void);
int mt_gotoXY(int row, int col);
int mt_getscreensize(int *rows, int *cols);
int mt_setfgcolor(mt_color color);
int mt_setbgcolor(mt_color color);
int mt_setdefaultcolor(void);
int mt_setcursorvisible(int value);
int mt_delLine(void);

// Backward-compatible aliases used by earlier stages.
#define MT_BLACK MT_COLOR_BLACK
#define MT_RED MT_COLOR_RED
#define MT_GREEN MT_COLOR_GREEN
#define MT_YELLOW MT_COLOR_YELLOW
#define MT_BLUE MT_COLOR_BLUE
#define MT_MAGENTA MT_COLOR_MAGENTA
#define MT_CYAN MT_COLOR_CYAN
#define MT_WHITE MT_COLOR_WHITE

#define mt_goto_xy mt_gotoXY
#define mt_set_fg mt_setfgcolor
#define mt_set_bg mt_setbgcolor
#define mt_reset_color mt_setdefaultcolor

#endif
