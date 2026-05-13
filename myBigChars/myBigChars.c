#include "myBigChars.h"
#include "myTerm.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int bc_printA(const char *str) {
    if (!str) return -1;
    return (write(1, str, strlen(str)) >= 0) ? 0 : -1;
}

int bc_box(int x1, int y1, int x2, int y2) {
    if (x1 < 1 || y1 < 1 || x2 <= x1 || y2 <= y1) return -1;
    mt_gotoXY(x1, y1); bc_printA("┌");
    mt_gotoXY(x1, y2); bc_printA("┐");
    mt_gotoXY(x2, y1); bc_printA("└");
    mt_gotoXY(x2, y2); bc_printA("┘");
    for (int y = y1 + 1; y < y2; ++y) {
        mt_gotoXY(x1, y); bc_printA("─");
        mt_gotoXY(x2, y); bc_printA("─");
    }
    for (int x = x1 + 1; x < x2; ++x) {
        mt_gotoXY(x, y1); bc_printA("│");
        mt_gotoXY(x, y2); bc_printA("│");
    }
    return 0;
}

int bc_setbigcharpos(int big[2], int x, int y, int value) {
    if (!big || x < 0 || x > 7 || y < 0 || y > 7 || (value != 0 && value != 1)) return -1;
    int bit = x * 8 + y;
    int idx = bit / 32;
    int sh = bit % 32;
    if (value) big[idx] |= (1 << sh);
    else big[idx] &= ~(1 << sh);
    return 0;
}

int bc_getbigcharpos(const int big[2], int x, int y, int *value) {
    if (!big || !value || x < 0 || x > 7 || y < 0 || y > 7) return -1;
    int bit = x * 8 + y;
    int idx = bit / 32;
    int sh = bit % 32;
    *value = (big[idx] >> sh) & 1;
    return 0;
}

int bc_printbigchar(const int big[2], int x, int y, int fg, int bg) {
    if (!big) return -1;
    mt_setfgcolor((mt_color)fg);
    mt_setbgcolor((mt_color)bg);
    for (int i = 0; i < 8; ++i) {
        mt_gotoXY(x + i, y);
        for (int j = 0; j < 8; ++j) {
            int v = 0;
            bc_getbigcharpos(big, i, j, &v);
            bc_printA(v ? "█" : " ");
        }
    }
    mt_setdefaultcolor();
    return 0;
}

int bc_bigcharwrite(int fd, const int *big, int count) {
    if (!big || count < 0) return -1;
    ssize_t n = write(fd, big, (size_t)count * 2 * sizeof(int));
    return (n == (ssize_t)((size_t)count * 2 * sizeof(int))) ? 0 : -1;
}

int bc_bigcharread(int fd, int *big, int need_count, int *count) {
    if (!big || need_count < 0) return -1;
    ssize_t n = read(fd, big, (size_t)need_count * 2 * sizeof(int));
    if (n < 0) return -1;
    if (count) *count = (int)(n / (2 * (int)sizeof(int)));
    return 0;
}

typedef struct {
    char c;
    int d[2];
} Glyph;

static const Glyph g_font[] = {
    {'0', {0x3C666666, 0x6666663C}}, {'1', {0x18381818, 0x1818183C}},
    {'2', {0x3C66060C, 0x1830607E}}, {'3', {0x3C66061C, 0x0606663C}},
    {'4', {0x0C1C3C6C, 0x7E0C0C0C}}, {'5', {0x7E607C06, 0x0606663C}},
    {'6', {0x1C30607C, 0x6666663C}}, {'7', {0x7E060C18, 0x30303030}},
    {'8', {0x3C66663C, 0x6666663C}}, {'9', {0x3C666666, 0x3E060C38}},
    {'A', {0x183C6666, 0x7E666666}}, {'B', {0x7C66667C, 0x6666667C}},
    {'C', {0x3C666060, 0x6060663C}}, {'D', {0x786C6666, 0x66666C78}},
    {'E', {0x7E60607C, 0x6060607E}}, {'F', {0x7E60607C, 0x60606060}},
    {'+', {0x0018187E, 0x7E181800}}, {'-', {0x0000007E, 0x7E000000}},
    {'!', {0x18181818, 0x18180018}}, {' ', {0x00000000, 0x00000000}}
};

int bc_symbol(char c, int out[2]) {
    if (!out) return -1;
    for (size_t i = 0; i < sizeof(g_font) / sizeof(g_font[0]); ++i) {
        if (g_font[i].c == c) {
            out[0] = g_font[i].d[0];
            out[1] = g_font[i].d[1];
            return 0;
        }
    }
    return -1;
}

void bc_print_word_hex(int16_t value) {
    unsigned masked = (unsigned)value & 0x7FFFu;
    printf("+%04X", masked);
}

void bc_print_bigvalue(uint16_t value, int x, int y) {
    char s[6];
    snprintf(s, sizeof(s), "%04X", (unsigned)(value & 0x7FFFu));
    int plus[2];
    if (bc_symbol('+', plus) == 0) bc_printbigchar(plus, x, y, MT_COLOR_YELLOW, MT_COLOR_BLACK);
    for (int i = 0; i < 4; ++i) {
        int g[2];
        if (bc_symbol(s[i], g) == 0) bc_printbigchar(g, x, y + 10 + i * 9, MT_COLOR_CYAN, MT_COLOR_BLACK);
    }
}
