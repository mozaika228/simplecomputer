#include "simplebasic.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BASIC_LINES 256
#define MAX_VARS 128
#define MAX_CONSTS 128
#define MAX_ASM_LINES 512

typedef struct {
    int number;
    char stmt[256];
} BasicLine;

typedef struct {
    char name[64];
} Symbol;

typedef struct {
    int value;
} ConstValue;

static void trim(char *s) {
    while (isspace((unsigned char)*s)) memmove(s, s + 1, strlen(s));
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static int cmp_lines(const void *a, const void *b) {
    const BasicLine *la = (const BasicLine *)a;
    const BasicLine *lb = (const BasicLine *)b;
    return (la->number > lb->number) - (la->number < lb->number);
}

static int ensure_var(Symbol *vars, int *count, const char *name) {
    for (int i = 0; i < *count; ++i) {
        if (strcmp(vars[i].name, name) == 0) return i;
    }
    if (*count >= MAX_VARS) return -1;
    strncpy(vars[*count].name, name, sizeof(vars[*count].name) - 1);
    vars[*count].name[sizeof(vars[*count].name) - 1] = '\0';
    return (*count)++;
}

static int ensure_const(ConstValue *consts, int *count, int value) {
    for (int i = 0; i < *count; ++i) {
        if (consts[i].value == value) return i;
    }
    if (*count >= MAX_CONSTS) return -1;
    consts[*count].value = value;
    return (*count)++;
}

static int is_number_token(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '+' || *s == '-') s++;
    if (!*s) return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

static void sym_ref(const char *token, char *out, size_t out_sz, Symbol *vars, int *var_count, ConstValue *consts, int *const_count) {
    if (is_number_token(token)) {
        int idx = ensure_const(consts, const_count, atoi(token));
        snprintf(out, out_sz, "CONST_%d", idx);
    } else {
        ensure_var(vars, var_count, token);
        snprintf(out, out_sz, "VAR_%s", token);
    }
}

static int append_line(char asm_lines[][128], int *asm_count, const char *line) {
    if (*asm_count >= MAX_ASM_LINES) return -1;
    strncpy(asm_lines[*asm_count], line, 127);
    asm_lines[*asm_count][127] = '\0';
    (*asm_count)++;
    return 0;
}

int sb_compile_file(const char *input_path, const char *output_asm_path) {
    FILE *in = fopen(input_path, "r");
    if (!in) return -1;

    BasicLine lines[MAX_BASIC_LINES];
    Symbol vars[MAX_VARS];
    ConstValue consts[MAX_CONSTS];
    int line_count = 0;
    int var_count = 0;
    int const_count = 0;

    char buf[256];
    while (fgets(buf, sizeof(buf), in)) {
        trim(buf);
        if (!*buf) continue;
        int no = 0;
        int offset = 0;
        if (sscanf(buf, "%d %n", &no, &offset) != 1) {
            fclose(in);
            return -1;
        }
        if (line_count >= MAX_BASIC_LINES) {
            fclose(in);
            return -1;
        }
        lines[line_count].number = no;
        strncpy(lines[line_count].stmt, buf + offset, sizeof(lines[line_count].stmt) - 1);
        lines[line_count].stmt[sizeof(lines[line_count].stmt) - 1] = '\0';
        line_count++;
    }
    fclose(in);
    qsort(lines, line_count, sizeof(lines[0]), cmp_lines);

    char asm_lines[MAX_ASM_LINES][128];
    int asm_count = 0;
    int generated_labels = 0;

    for (int i = 0; i < line_count; ++i) {
        char line_head[64];
        snprintf(line_head, sizeof(line_head), "L%d:", lines[i].number);
        if (append_line(asm_lines, &asm_count, line_head) != 0) return -1;

        char stmt[256];
        strncpy(stmt, lines[i].stmt, sizeof(stmt) - 1);
        stmt[sizeof(stmt) - 1] = '\0';

        char *kw = strtok(stmt, " \t\r\n");
        if (!kw) continue;
        for (char *p = kw; *p; ++p) *p = (char)toupper((unsigned char)*p);

        if (strcmp(kw, "INPUT") == 0) {
            char *v = strtok(NULL, " \t\r\n");
            if (!v) return -1;
            ensure_var(vars, &var_count, v);
            char out[128];
            snprintf(out, sizeof(out), "READ VAR_%s", v);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            continue;
        }

        if (strcmp(kw, "PRINT") == 0) {
            char *v = strtok(NULL, " \t\r\n");
            if (!v) return -1;
            char ref[64];
            sym_ref(v, ref, sizeof(ref), vars, &var_count, consts, &const_count);
            char out[128];
            snprintf(out, sizeof(out), "WRITE %s", ref);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            continue;
        }

        if (strcmp(kw, "LET") == 0) {
            char *lhs = strtok(NULL, " \t\r\n");
            char *eq = strtok(NULL, " \t\r\n");
            char *a = strtok(NULL, " \t\r\n");
            char *op = strtok(NULL, " \t\r\n");
            char *b = strtok(NULL, " \t\r\n");
            if (!lhs || !eq || strcmp(eq, "=") != 0 || !a) return -1;
            ensure_var(vars, &var_count, lhs);

            char aref[64];
            sym_ref(a, aref, sizeof(aref), vars, &var_count, consts, &const_count);
            char out[128];
            snprintf(out, sizeof(out), "LOAD %s", aref);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;

            if (op && b) {
                char bref[64];
                sym_ref(b, bref, sizeof(bref), vars, &var_count, consts, &const_count);
                if (strcmp(op, "+") == 0) snprintf(out, sizeof(out), "ADD %s", bref);
                else if (strcmp(op, "-") == 0) snprintf(out, sizeof(out), "SUB %s", bref);
                else if (strcmp(op, "*") == 0) snprintf(out, sizeof(out), "MUL %s", bref);
                else if (strcmp(op, "/") == 0) snprintf(out, sizeof(out), "DIVIDE %s", bref);
                else return -1;
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            }
            snprintf(out, sizeof(out), "STORE VAR_%s", lhs);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            continue;
        }

        if (strcmp(kw, "GOTO") == 0) {
            char *ln = strtok(NULL, " \t\r\n");
            if (!ln) return -1;
            char out[128];
            snprintf(out, sizeof(out), "JUMP L%s", ln);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            continue;
        }

        if (strcmp(kw, "IF") == 0) {
            char *a = strtok(NULL, " \t\r\n");
            char *cmp = strtok(NULL, " \t\r\n");
            char *b = strtok(NULL, " \t\r\n");
            char *then_kw = strtok(NULL, " \t\r\n");
            char *ln = strtok(NULL, " \t\r\n");
            if (!a || !cmp || !b || !then_kw || !ln) return -1;
            for (char *p = then_kw; *p; ++p) *p = (char)toupper((unsigned char)*p);
            if (strcmp(then_kw, "THEN") != 0) return -1;

            char aref[64], bref[64], out[128];
            sym_ref(a, aref, sizeof(aref), vars, &var_count, consts, &const_count);
            sym_ref(b, bref, sizeof(bref), vars, &var_count, consts, &const_count);
            snprintf(out, sizeof(out), "LOAD %s", aref);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            snprintf(out, sizeof(out), "SUB %s", bref);
            if (append_line(asm_lines, &asm_count, out) != 0) return -1;

            if (strcmp(cmp, "<") == 0) {
                snprintf(out, sizeof(out), "JNEG L%s", ln);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            } else if (strcmp(cmp, "=") == 0) {
                snprintf(out, sizeof(out), "JZ L%s", ln);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            } else if (strcmp(cmp, ">") == 0) {
                int skip_id = generated_labels++;
                snprintf(out, sizeof(out), "JNEG GT_SKIP_%d", skip_id);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
                snprintf(out, sizeof(out), "JZ GT_SKIP_%d", skip_id);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
                snprintf(out, sizeof(out), "JUMP L%s", ln);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
                snprintf(out, sizeof(out), "GT_SKIP_%d:", skip_id);
                if (append_line(asm_lines, &asm_count, out) != 0) return -1;
            } else {
                return -1;
            }
            continue;
        }

        if (strcmp(kw, "END") == 0) {
            if (append_line(asm_lines, &asm_count, "HALT") != 0) return -1;
            continue;
        }

        return -1;
    }

    FILE *out = fopen(output_asm_path, "w");
    if (!out) return -1;

    for (int i = 0; i < asm_count; ++i) fprintf(out, "%s\n", asm_lines[i]);
    for (int i = 0; i < var_count; ++i) fprintf(out, "VAR_%s: WORD 0\n", vars[i].name);
    for (int i = 0; i < const_count; ++i) fprintf(out, "CONST_%d: WORD %d\n", i, consts[i].value);

    fclose(out);
    return 0;
}
