#include "myBigChars.h"
#include "myReadKey.h"
#include "mySimpleComputer.h"
#include "myTerm.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#define read _read
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static volatile sig_atomic_t g_tick = 0;
static volatile sig_atomic_t g_reset = 0;
static int g_selected = 0;
static int g_running = 0;

static void on_sigalrm(int sig) {
    (void)sig;
    g_tick = 1;
}

static void on_sigusr1(int sig) {
    (void)sig;
    g_reset = 1;
}

static void timer_start(void) {
#ifndef _WIN32
    struct itimerval it;
    memset(&it, 0, sizeof(it));
    it.it_interval.tv_usec = 100000;
    it.it_value.tv_usec = 100000;
    setitimer(ITIMER_REAL, &it, NULL);
#endif
}

static void timer_stop(void) {
#ifndef _WIN32
    struct itimerval it;
    memset(&it, 0, sizeof(it));
    setitimer(ITIMER_REAL, &it, NULL);
#endif
}

static void print_cell(const SCComputer *sc, int addr) {
    int r = 3 + addr / 16;
    int c = 2 + (addr % 16) * 7;
    mt_gotoXY(r, c);
    if (addr == g_selected) {
        mt_setfgcolor(MT_COLOR_BLACK);
        mt_setbgcolor(MT_COLOR_CYAN);
    } else {
        mt_setfgcolor(MT_COLOR_WHITE);
        mt_setbgcolor(MT_COLOR_BLACK);
    }
    printf("+%04X ", (unsigned)(sc->memory[addr] & SC_WORD_MASK));
    mt_setdefaultcolor();
}

static void print_grid(const SCComputer *sc) {
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) print_cell(sc, i);
}

static void print_flags(const SCComputer *sc) {
    mt_gotoXY(12, 2);
    printf("FLAGS: %c %c %c %c %c",
           (sc->flags & SC_FLAG_OVERFLOW) ? 'P' : '_',
           (sc->flags & SC_FLAG_DIV_BY_ZERO) ? '0' : '_',
           (sc->flags & SC_FLAG_MEMORY) ? 'M' : '_',
           (sc->flags & SC_FLAG_IGNORE_CLOCK) ? 'T' : '_',
           (sc->flags & SC_FLAG_COMMAND) ? 'E' : '_');
}

static void print_command(const SCComputer *sc) {
    mt_gotoXY(13, 2);
    uint16_t v = (uint16_t)(sc->memory[sc->instruction_counter] & SC_WORD_MASK);
    uint8_t op = 0, arg = 0;
    if (sc_command_decode((int16_t)v, &op, &arg) == 0) printf("COMMAND: + %02u %02u", op, arg);
    else printf("COMMAND: ! %04X", v);
}

static void print_regs(const SCComputer *sc) {
    mt_gotoXY(14, 2);
    printf("ACC=%+6d  IC=%3u  RUN=%d", sc->accumulator, sc->instruction_counter, g_running ? 1 : 0);
}

static void print_help(void) {
    mt_gotoXY(15, 2);
    printf("Keys: arrows move | Enter edit memory | F5 edit ACC | F6 edit IC | Esc exit | r run/stop | s step");
}

static void printBigCell(const SCComputer *sc) {
    bc_box(17, 2, 26, 55);
    mt_gotoXY(17, 4);
    printf("Big cell @%03d", g_selected);
    bc_print_bigvalue((uint16_t)sc->memory[g_selected], 18, 4);
}

static void redraw(const SCComputer *sc) {
    mt_clrscr();
    mt_setcursorvisible(0);
    mt_gotoXY(1, 2);
    printf("Simple Computer interactive console");
    print_grid(sc);
    print_flags(sc);
    print_command(sc);
    print_regs(sc);
    print_help();
    printBigCell(sc);
    fflush(stdout);
}

static void prompt_edit_int(const char *title, int *out) {
    rk_mytermrestore();
    mt_setcursorvisible(1);
    mt_gotoXY(28, 2);
    mt_delline();
    printf("%s: ", title);
    fflush(stdout);
    int v = 0;
    if (scanf("%d", &v) == 1) *out = v;
    while (getchar() != '\n') {}
    rk_mytermsave();
    rk_mytermregime(1, 1, 0, 0, 1);
}

static void move_sel(ReadKey k) {
    int row = g_selected / 16;
    int col = g_selected % 16;
    if (k == RK_UP && row > 0) row--;
    if (k == RK_DOWN && row < 7) row++;
    if (k == RK_LEFT && col > 0) col--;
    if (k == RK_RIGHT && col < 15) col++;
    g_selected = row * 16 + col;
}

int main(int argc, char **argv) {
    SCComputer sc;
    sc_init(&sc);
    if (argc == 2) sc_memory_load(&sc, argv[1]);

    if (!isatty(fileno(stdout))) {
        fprintf(stderr, "stdout is not a terminal\n");
        return 1;
    }
    int rows = 0, cols = 0;
    if (mt_getscreensize(&rows, &cols) != 0 || rows < 30 || cols < 120) {
        fprintf(stderr, "terminal too small: need 120x30, got %dx%d\n", cols, rows);
        return 1;
    }

    signal(SIGALRM, on_sigalrm);
#ifdef SIGUSR1
    signal(SIGUSR1, on_sigusr1);
#endif
    timer_start();

    rk_mytermsave();
    rk_mytermregime(1, 1, 0, 0, 1);

    int done = 0;
    while (!done) {
        if (g_reset) {
            sc_init(&sc);
            g_reset = 0;
            g_running = 0;
        }

        if (g_running && g_tick) {
            g_tick = 0;
            if (sc_step(&sc) != 0) g_running = 0;
        }

        redraw(&sc);
        ReadKey k = rk_read_key();
        if (k == RK_ESC) {
            done = 1;
            continue;
        }
        if (k == RK_UP || k == RK_DOWN || k == RK_LEFT || k == RK_RIGHT) {
            move_sel(k);
            continue;
        }
        if (k == RK_ENTER) {
            int v = sc.memory[g_selected];
            prompt_edit_int("Memory value", &v);
            sc_memory_set(&sc, g_selected, (int16_t)v);
            continue;
        }
        if (k == RK_F5) {
            int v = sc.accumulator;
            prompt_edit_int("Accumulator", &v);
            sc.accumulator = (int16_t)v;
            continue;
        }
        if (k == RK_F6) {
            int v = sc.instruction_counter;
            prompt_edit_int("Instruction Counter", &v);
            if (v >= 0 && v < SC_MEMORY_SIZE) sc.instruction_counter = (uint8_t)v;
            continue;
        }

        if (k == RK_UNKNOWN) {
            unsigned char ch = 0;
            if (read(0, &ch, 1) == 1) {
                if (ch == 'r' || ch == 'R') g_running = !g_running;
                if (ch == 's' || ch == 'S') sc_step(&sc);
            }
        }
    }

    rk_mytermrestore();
    timer_stop();
    mt_setdefaultcolor();
    mt_setcursorvisible(1);
    mt_gotoXY(rows, 1);
    printf("\n");
    return 0;
}
