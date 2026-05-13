#include "myReadKey.h"

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

static struct termios g_saved;
static int g_saved_ok = 0;

int rk_mytermsave(void) {
    if (tcgetattr(STDIN_FILENO, &g_saved) == -1) return -1;
    g_saved_ok = 1;
    return 0;
}

int rk_mytermrestore(void) {
    if (!g_saved_ok) return -1;
    return tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
}

int rk_mytermregime(int regime, int vtime, int vmin, int echo, int sigint) {
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &t) == -1) return -1;

    if (regime) t.c_lflag &= (tcflag_t)~ICANON;
    else t.c_lflag |= ICANON;

    if (echo) t.c_lflag |= ECHO;
    else t.c_lflag &= (tcflag_t)~ECHO;

    if (sigint) t.c_lflag |= ISIG;
    else t.c_lflag &= (tcflag_t)~ISIG;

    t.c_cc[VTIME] = (cc_t)vtime;
    t.c_cc[VMIN] = (cc_t)vmin;
    return tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

ReadKey rk_read_key(void) {
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) return RK_UNKNOWN;
    if (c == '\n' || c == '\r') return RK_ENTER;
    if (c == 27) {
        unsigned char c2 = 0;
        if (read(STDIN_FILENO, &c2, 1) != 1) return RK_ESC;
        if (c2 == '[') {
            unsigned char c3 = 0;
            if (read(STDIN_FILENO, &c3, 1) != 1) return RK_ESC;
            if (c3 == 'A') return RK_UP;
            if (c3 == 'B') return RK_DOWN;
            if (c3 == 'C') return RK_RIGHT;
            if (c3 == 'D') return RK_LEFT;
            if (c3 == '1') {
                unsigned char c4 = 0;
                unsigned char c5 = 0;
                if (read(STDIN_FILENO, &c4, 1) == 1 && read(STDIN_FILENO, &c5, 1) == 1) {
                    if (c4 == '5' && c5 == '~') return RK_F5;
                    if (c4 == '7' && c5 == '~') return RK_F6;
                }
            }
        }
        if (c2 == 'O') {
            unsigned char c3 = 0;
            if (read(STDIN_FILENO, &c3, 1) != 1) return RK_ESC;
            if (c3 == 't') return RK_F5;
            if (c3 == 'u') return RK_F6;
            if (c3 == 'P') return RK_F5;
            if (c3 == 'Q') return RK_F6;
        }
        return RK_ESC;
    }
    return RK_UNKNOWN;
}
