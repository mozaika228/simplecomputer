#ifndef MY_READ_KEY_H
#define MY_READ_KEY_H

typedef enum {
    RK_UNKNOWN = 0,
    RK_UP,
    RK_DOWN,
    RK_LEFT,
    RK_RIGHT,
    RK_ENTER,
    RK_ESC,
    RK_F5,
    RK_F6
} ReadKey;

ReadKey rk_read_key(void);

#endif
