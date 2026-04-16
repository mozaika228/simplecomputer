#include "mySimpleComputer.h"
#include "myTerm.h"
#include "simpleassembler.h"
#include "simplebasic.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#define MEM_TOP 3
#define MEM_LEFT 2
#define TERM_TOP 17
#define TERM_ROWS 6
#define TERM_COLS 92

static char g_term_lines[TERM_ROWS][TERM_COLS + 1];
static int g_term_count = 0;

static void term_pushf(const char *fmt, ...) {
    char line[TERM_COLS + 1];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (g_term_count < TERM_ROWS) {
        strncpy(g_term_lines[g_term_count], line, TERM_COLS);
        g_term_lines[g_term_count][TERM_COLS] = '\0';
        g_term_count++;
        return;
    }

    for (int i = 1; i < TERM_ROWS; ++i) {
        strncpy(g_term_lines[i - 1], g_term_lines[i], TERM_COLS + 1);
    }
    strncpy(g_term_lines[TERM_ROWS - 1], line, TERM_COLS);
    g_term_lines[TERM_ROWS - 1][TERM_COLS] = '\0';
}

static void print_binary15(uint16_t value) {
    for (int i = 14; i >= 0; --i) {
        putchar((value >> i) & 1u ? '1' : '0');
        if (i % 4 == 0 && i != 0) putchar(' ');
    }
}

static void printCell(const SCComputer *sc, int address, int selected) {
    int row = MEM_TOP + (address / 16);
    int col = MEM_LEFT + (address % 16) * 7;
    uint16_t cell = (uint16_t)sc->memory[address] & SC_WORD_MASK;
    uint8_t opcode = 0, operand = 0;
    int is_cmd = sc_command_decode((int16_t)cell, &opcode, &operand) == 0;

    mt_gotoXY(row, col);
    if (selected) {
        mt_setfgcolor(MT_COLOR_BLACK);
        mt_setbgcolor(MT_COLOR_CYAN);
    } else if (is_cmd) {
        mt_setfgcolor(MT_COLOR_GREEN);
        mt_setbgcolor(MT_COLOR_BLACK);
    } else {
        mt_setfgcolor(MT_COLOR_WHITE);
        mt_setbgcolor(MT_COLOR_BLACK);
    }
    printf("+%04X ", cell);
    mt_setdefaultcolor();
}

static void printFlags(const SCComputer *sc) {
    mt_gotoXY(12, 2);
    printf("FLAGS: %c %c %c %c %c",
           (sc->flags & SC_FLAG_OVERFLOW) ? 'P' : '_',
           (sc->flags & SC_FLAG_DIV_BY_ZERO) ? '0' : '_',
           (sc->flags & SC_FLAG_MEMORY) ? 'M' : '_',
           (sc->flags & SC_FLAG_IGNORE_CLOCK) ? 'T' : '_',
           (sc->flags & SC_FLAG_COMMAND) ? 'E' : '_');
}

static void printDecodedCommand(const SCComputer *sc) {
    uint16_t cell = (uint16_t)sc->memory[sc->instruction_counter] & SC_WORD_MASK;
    mt_gotoXY(13, 2);
    printf("DECODED: dec=%5u oct=%06o hex=%04X bin=", cell, cell, cell);
    print_binary15(cell);
}

static void printAccumulator(const SCComputer *sc) {
    mt_gotoXY(14, 2);
    printf("ACCUMULATOR: %+6d", sc->accumulator);
}

static void printCounters(const SCComputer *sc) {
    mt_gotoXY(15, 2);
    printf("COUNTERS: IC=%3u  MEM=%llu  HIT=%llu  MISS=%llu  STALL=%llu  IO=%llu",
           sc->instruction_counter,
           (unsigned long long)sc->memory_accesses,
           (unsigned long long)sc->cache_hits,
           (unsigned long long)sc->cache_misses,
           (unsigned long long)sc->stall_cycles,
           (unsigned long long)sc->io_cycles);
}

static void printCommand(const SCComputer *sc) {
    uint16_t cell = (uint16_t)sc->memory[sc->instruction_counter] & SC_WORD_MASK;
    uint8_t opcode = 0, operand = 0;

    mt_gotoXY(16, 2);
    if (sc_command_decode((int16_t)cell, &opcode, &operand) == 0) {
        printf("COMMAND: + %02u %02u", opcode, operand);
    } else {
        printf("COMMAND: data word (%u)", cell);
    }
}

static void printTerm(void) {
    mt_gotoXY(TERM_TOP - 1, 2);
    printf("IN-OUT:");
    for (int i = 0; i < TERM_ROWS; ++i) {
        mt_gotoXY(TERM_TOP + i, 2);
        mt_delLine();
        mt_gotoXY(TERM_TOP + i, 2);
        if (i < g_term_count) printf("%s", g_term_lines[i]);
    }
}

static void draw_console(const SCComputer *sc) {
    mt_clrscr();
    mt_setcursorvisible(0);
    mt_gotoXY(1, 2);
    printf("Simple Computer Console  |  commands: run step reset load save asm basic set help exit");

    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        int selected = (i == (int)sc->instruction_counter);
        printCell(sc, i, selected);
    }
    printFlags(sc);
    printDecodedCommand(sc);
    printAccumulator(sc);
    printCounters(sc);
    printCommand(sc);
    printTerm();
    fflush(stdout);
}

static void print_help_message(void) {
    term_pushf("help: run | step | reset | load <f> | save <f> | asm <in.sa> <out.sc> | basic <in.sb> <out.sa> | set <a> <v> | exit");
}

int main(int argc, char **argv) {
    SCComputer sc;
    sc_init(&sc);

    if (!isatty(fileno(stdout))) {
        fprintf(stderr, "console: stdout is not a terminal (isatty failed)\n");
        return 1;
    }

    int rows = 0;
    int cols = 0;
    if (mt_getscreensize(&rows, &cols) != 0) {
        fprintf(stderr, "console: cannot detect terminal size\n");
        return 1;
    }
    if (rows < 26 || cols < 110) {
        fprintf(stderr, "console: terminal is too small, need at least 110x26, got %dx%d\n", cols, rows);
        return 1;
    }

    if (argc == 2) {
        if (sc_memory_load(&sc, argv[1]) != 0) {
            fprintf(stderr, "Cannot load memory file: %s\n", argv[1]);
            return 1;
        }
        term_pushf("loaded memory: %s", argv[1]);
    } else {
        term_pushf("memory initialized");
    }
    print_help_message();

    char cmd[256];
    while (1) {
        draw_console(&sc);
        mt_setcursorvisible(1);
        mt_gotoXY(rows, 1);
        mt_delLine();
        printf("sc> ");
        fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        cmd[strcspn(cmd, "\r\n")] = '\0';

        if (strncmp(cmd, "exit", 4) == 0) {
            term_pushf("exit requested");
            break;
        }
        if (strncmp(cmd, "help", 4) == 0) {
            print_help_message();
            continue;
        }
        if (strncmp(cmd, "run", 3) == 0) {
            sc_run(&sc, -1);
            term_pushf("run: program stopped");
            continue;
        }
        if (strncmp(cmd, "step", 4) == 0) {
            int rc = sc_step(&sc);
            term_pushf("step: rc=%d ic=%u", rc, sc.instruction_counter);
            continue;
        }
        if (strncmp(cmd, "reset", 5) == 0) {
            sc_init(&sc);
            term_pushf("reset: cpu and memory cleared");
            continue;
        }

        char p1[128];
        char p2[128];
        int addr = 0;
        int value = 0;

        if (sscanf(cmd, "load %127s", p1) == 1) {
            if (sc_memory_load(&sc, p1) == 0) term_pushf("load: %s", p1);
            else term_pushf("load error: %s", p1);
            continue;
        }
        if (sscanf(cmd, "save %127s", p1) == 1) {
            if (sc_memory_save(&sc, p1) == 0) term_pushf("save: %s", p1);
            else term_pushf("save error: %s", p1);
            continue;
        }
        if (sscanf(cmd, "set %d %d", &addr, &value) == 2) {
            if (sc_memory_set(&sc, addr, (int16_t)value) == 0) term_pushf("set: mem[%d]=%d", addr, value);
            else term_pushf("set error: addr=%d val=%d", addr, value);
            continue;
        }
        if (sscanf(cmd, "asm %127s %127s", p1, p2) == 2) {
            if (sa_assemble_file(p1, p2) == 0) term_pushf("asm: %s -> %s", p1, p2);
            else term_pushf("asm error");
            continue;
        }
        if (sscanf(cmd, "basic %127s %127s", p1, p2) == 2) {
            if (sb_compile_file(p1, p2) == 0) term_pushf("basic: %s -> %s", p1, p2);
            else term_pushf("basic error");
            continue;
        }

        term_pushf("unknown command: %s", cmd);
    }

    mt_setdefaultcolor();
    mt_setcursorvisible(1);
    mt_gotoXY(rows, 1);
    mt_delLine();
    printf("\n");
    return 0;
}
