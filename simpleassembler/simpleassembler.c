#include "simpleassembler.h"

#include "mySimpleComputer.h"
#include "sc_types.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 512
#define MAX_LABELS 256

typedef struct {
    char name[64];
    int address;
} Label;

typedef struct {
    char text[256];
    int address;
    int source_line;
} SourceLine;

static void trim(char *s) {
    while (isspace((unsigned char)*s)) memmove(s, s + 1, strlen(s));
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static void strip_comment(char *s) {
    for (size_t i = 0; s[i]; ++i) {
        if (s[i] == ';' || s[i] == '#') {
            s[i] = '\0';
            return;
        }
    }
}

static int find_label(const Label *labels, int count, const char *name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].address;
    }
    return -1;
}

static int opcode_by_name(const char *mn) {
    static const struct { const char *name; int code; } map[] = {
        {"READ", SC_READ}, {"WRITE", SC_WRITE}, {"LOAD", SC_LOAD}, {"STORE", SC_STORE},
        {"ADD", SC_ADD}, {"SUB", SC_SUB}, {"DIVIDE", SC_DIVIDE}, {"DIV", SC_DIVIDE},
        {"MUL", SC_MUL}, {"JUMP", SC_JUMP}, {"JNEG", SC_JNEG}, {"JZ", SC_JZ},
        {"HALT", SC_HALT}, {"AND", SC_AND}, {"OR", SC_OR}, {"XOR", SC_XOR},
        {"NOT", SC_NOT}, {"SHL", SC_SHL}, {"SHR", SC_SHR}
    };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
        if (strcmp(map[i].name, mn) == 0) return map[i].code;
    }
    return -1;
}

static int parse_value_or_label(const char *token, const Label *labels, int label_count, int *value) {
    if (!token || !*token) return -1;
    char *end = NULL;
    long v = strtol(token, &end, 10);
    if (end && *end == '\0') {
        *value = (int)v;
        return 0;
    }
    int addr = find_label(labels, label_count, token);
    if (addr >= 0) {
        *value = addr;
        return 0;
    }
    return -1;
}

int sa_assemble_file(const char *input_path, const char *output_path) {
    FILE *in = fopen(input_path, "r");
    if (!in) return -1;

    SourceLine lines[MAX_LINES];
    Label labels[MAX_LABELS];
    int line_count = 0;
    int label_count = 0;
    int addr = 0;

    char buf[256];
    int src_line = 0;
    while (fgets(buf, sizeof(buf), in)) {
        src_line++;
        strip_comment(buf);
        trim(buf);
        if (buf[0] == '\0') continue;

        char local[256];
        strncpy(local, buf, sizeof(local) - 1);
        local[sizeof(local) - 1] = '\0';

        char *colon = strchr(local, ':');
        if (colon) {
            *colon = '\0';
            trim(local);
            if (*local) {
                if (label_count >= MAX_LABELS) {
                    fclose(in);
                    return -1;
                }
                if (find_label(labels, label_count, local) >= 0) {
                    fclose(in);
                    return -1;
                }
                strncpy(labels[label_count].name, local, sizeof(labels[label_count].name) - 1);
                labels[label_count].name[sizeof(labels[label_count].name) - 1] = '\0';
                labels[label_count].address = addr;
                label_count++;
            }
            memmove(buf, colon + 1, strlen(colon + 1) + 1);
            trim(buf);
            if (!*buf) continue;
        }

        if (line_count >= MAX_LINES || addr >= SC_MEMORY_SIZE) {
            fclose(in);
            return -1;
        }
        strncpy(lines[line_count].text, buf, sizeof(lines[line_count].text) - 1);
        lines[line_count].text[sizeof(lines[line_count].text) - 1] = '\0';
        lines[line_count].address = addr++;
        lines[line_count].source_line = src_line;
        line_count++;
    }
    fclose(in);

    int16_t outmem[SC_MEMORY_SIZE];
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) outmem[i] = 0;

    for (int i = 0; i < line_count; ++i) {
        char text[256];
        strncpy(text, lines[i].text, sizeof(text) - 1);
        text[sizeof(text) - 1] = '\0';

        char *mn = strtok(text, " \t\r\n");
        char *op = strtok(NULL, " \t\r\n");

        if (!mn) continue;
        for (char *p = mn; *p; ++p) *p = (char)toupper((unsigned char)*p);

        if (strcmp(mn, "WORD") == 0) {
            if (!op) return -1;
            int v;
            if (parse_value_or_label(op, labels, label_count, &v) != 0) return -1;
            if (v < SC_MIN_VALUE || v > SC_MAX_VALUE) return -1;
            outmem[lines[i].address] = (int16_t)v;
            continue;
        }

        int opcode = opcode_by_name(mn);
        if (opcode < 0) return -1;

        int operand = 0;
        if (opcode != SC_HALT && opcode != SC_NOT) {
            if (!op) return -1;
            if (parse_value_or_label(op, labels, label_count, &operand) != 0) return -1;
            if (operand < 0 || operand >= SC_MEMORY_SIZE) return -1;
        }

        int16_t encoded = 0;
        if (sc_command_encode((uint8_t)opcode, (uint8_t)operand, &encoded) != 0) return -1;
        outmem[lines[i].address] = encoded;
    }

    FILE *out = fopen(output_path, "w");
    if (!out) return -1;
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        fprintf(out, "%d %d\n", i, outmem[i]);
    }
    fclose(out);
    return 0;
}
