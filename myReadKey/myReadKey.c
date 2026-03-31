#include "myReadKey.h"

#include <stdio.h>

ReadKey rk_read_key(void) {
    int c = getchar();
    if (c == '\n' || c == '\r') return RK_ENTER;
    if (c == 27) {
        int c2 = getchar();
        if (c2 == '[') {
            int c3 = getchar();
            if (c3 == 'A') return RK_UP;
            if (c3 == 'B') return RK_DOWN;
            if (c3 == 'C') return RK_RIGHT;
            if (c3 == 'D') return RK_LEFT;
        }
        if (c2 == 'O') {
            int c3 = getchar();
            if (c3 == 't') return RK_F5;
            if (c3 == 'u') return RK_F6;
        }
        return RK_ESC;
    }
    return RK_UNKNOWN;
}
