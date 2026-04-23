#include "myTerm.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define write _write
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static void mt_enable_vt_mode(void) {
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE) return;

    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(out, mode);
}
#endif

static int mt_write_str(const char *s) {
#ifdef _WIN32
    mt_enable_vt_mode();
#endif
    size_t len = strlen(s);
    return (write(1, s, (unsigned)len) == (int)len) ? 0 : -1;
}

int mt_clrscr(void) {
    return mt_write_str("\033[2J\033[H");
}

int mt_gotoXY(int row, int col) {
    char buf[32];
    if (row < 1) row = 1;
    if (col < 1) col = 1;
    snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    return mt_write_str(buf);
}

int mt_getscreensize(int *rows, int *cols) {
    if (!rows || !cols) return -1;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (!GetConsoleScreenBufferInfo(h, &csbi)) return -1;
    *cols = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    *rows = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    return 0;
#else
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1) return -1;
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
#endif
}

int mt_setfgcolor(mt_color color) {
    char buf[16];
    if (color < MT_COLOR_BLACK || color > MT_COLOR_WHITE) return -1;
    snprintf(buf, sizeof(buf), "\033[%dm", 30 + (int)color);
    return mt_write_str(buf);
}

int mt_setbgcolor(mt_color color) {
    char buf[16];
    if (color < MT_COLOR_BLACK || color > MT_COLOR_WHITE) return -1;
    snprintf(buf, sizeof(buf), "\033[%dm", 40 + (int)color);
    return mt_write_str(buf);
}

int mt_setdefaultcolor(void) {
    return mt_write_str("\033[0m");
}

int mt_setcursorvisible(int value) {
    return mt_write_str(value ? "\033[?25h" : "\033[?25l");
}

int mt_delLine(void) {
    return mt_write_str("\033[2K\r");
}
