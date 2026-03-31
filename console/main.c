#include "myBigChars.h"
#include "mySimpleComputer.h"
#include "myTerm.h"
#include "simpleassembler.h"
#include "simplebasic.h"

#include <stdio.h>
#include <string.h>

static void print_help(void) {
    printf("Commands:\n");
    printf("  help\n");
    printf("  load <file.sc>\n");
    printf("  save <file.sc>\n");
    printf("  run\n");
    printf("  step\n");
    printf("  regs\n");
    printf("  dump\n");
    printf("  cache\n");
    printf("  set <addr> <value>\n");
    printf("  asm <input.sa> <output.sc>\n");
    printf("  basic <input.sb> <output.sa>\n");
    printf("  reset\n");
    printf("  exit\n");
}

int main(int argc, char **argv) {
    SCComputer sc;
    sc_init(&sc);

    if (argc == 2) {
        if (sc_memory_load(&sc, argv[1]) != 0) {
            fprintf(stderr, "Cannot load memory file: %s\n", argv[1]);
            return 1;
        }
    }

    mt_clrscr();
    print_help();
    char cmd[256];

    while (1) {
        printf("sc> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        if (strncmp(cmd, "exit", 4) == 0) break;
        if (strncmp(cmd, "help", 4) == 0) {
            print_help();
            continue;
        }
        if (strncmp(cmd, "run", 3) == 0) {
            sc_run(&sc, -1);
            printf("Program stopped.\n");
            continue;
        }
        if (strncmp(cmd, "step", 4) == 0) {
            int rc = sc_step(&sc);
            printf("step rc=%d\n", rc);
            continue;
        }
        if (strncmp(cmd, "regs", 4) == 0) {
            sc_dump_registers(&sc);
            bc_print_word_hex(sc.accumulator);
            continue;
        }
        if (strncmp(cmd, "dump", 4) == 0) {
            sc_dump_memory(&sc);
            continue;
        }
        if (strncmp(cmd, "cache", 5) == 0) {
            sc_dump_cache(&sc);
            continue;
        }
        if (strncmp(cmd, "reset", 5) == 0) {
            sc_init(&sc);
            printf("CPU and memory reset.\n");
            continue;
        }

        char p1[128];
        char p2[128];
        int addr;
        int value;

        if (sscanf(cmd, "load %127s", p1) == 1) {
            if (sc_memory_load(&sc, p1) == 0) printf("Loaded: %s\n", p1);
            else printf("Load error: %s\n", p1);
            continue;
        }
        if (sscanf(cmd, "save %127s", p1) == 1) {
            if (sc_memory_save(&sc, p1) == 0) printf("Saved: %s\n", p1);
            else printf("Save error: %s\n", p1);
            continue;
        }
        if (sscanf(cmd, "set %d %d", &addr, &value) == 2) {
            if (sc_memory_set(&sc, addr, (int16_t)value) == 0) printf("OK\n");
            else printf("Memory set error\n");
            continue;
        }
        if (sscanf(cmd, "asm %127s %127s", p1, p2) == 2) {
            if (sa_assemble_file(p1, p2) == 0) printf("Assembled: %s -> %s\n", p1, p2);
            else printf("Assembler error\n");
            continue;
        }
        if (sscanf(cmd, "basic %127s %127s", p1, p2) == 2) {
            if (sb_compile_file(p1, p2) == 0) printf("Compiled BASIC: %s -> %s\n", p1, p2);
            else printf("BASIC compile error\n");
            continue;
        }

        printf("Unknown command. Type: help\n");
    }

    return 0;
}
