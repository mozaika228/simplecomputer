#ifndef MY_READ_KEY_H
#define MY_READ_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

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

int rk_mytermsave(void);
int rk_mytermrestore(void);
int rk_mytermregime(int regime, int vtime, int vmin, int echo, int sigint);
ReadKey rk_read_key(void);

#ifdef __cplusplus
}
#endif

#endif
