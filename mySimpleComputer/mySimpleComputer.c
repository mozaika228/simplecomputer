#include "mySimpleComputer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int in_bounds(int address) {
    return address >= 0 && address < SC_MEMORY_SIZE;
}

static int cache_touch(SCComputer *sc, int address) {
    int block = address / 10;
    int offset = address % 10;
    int line_index = block % 5;
    SCCacheLine *line = &sc->cache[line_index];

    sc->memory_accesses++;
    if (line->valid && line->tag == (uint8_t)block) {
        sc->cache_hits++;
        sc->stall_cycles += 1;
        return offset;
    }

    sc->cache_misses++;
    sc->stall_cycles += 5;
    line->valid = true;
    line->tag = (uint8_t)block;
    for (int i = 0; i < 10; ++i) {
        int addr = block * 10 + i;
        line->data[i] = (addr < SC_MEMORY_SIZE) ? sc->memory[addr] : 0;
    }
    return offset;
}

int16_t sc_clamp15(int32_t value, bool *overflowed) {
    if (value > SC_MAX_VALUE) {
        if (overflowed) *overflowed = true;
        return SC_MAX_VALUE;
    }
    if (value < SC_MIN_VALUE) {
        if (overflowed) *overflowed = true;
        return SC_MIN_VALUE;
    }
    if (overflowed) *overflowed = false;
    return (int16_t)value;
}

void sc_init(SCComputer *sc) {
    memset(sc, 0, sizeof(*sc));
}

void sc_reset_cpu(SCComputer *sc) {
    sc->accumulator = 0;
    sc->instruction_counter = 0;
    sc->flags = 0;
    sc->halted = false;
}

void sc_flag_set(SCComputer *sc, SCFlag flag, bool enabled) {
    if (enabled) sc->flags |= (uint8_t)flag;
    else sc->flags &= (uint8_t)~flag;
}

bool sc_flag_get(const SCComputer *sc, SCFlag flag) {
    return (sc->flags & (uint8_t)flag) != 0;
}

int sc_memory_set(SCComputer *sc, int address, int16_t value) {
    if (!in_bounds(address)) {
        sc_flag_set(sc, SC_FLAG_MEMORY, true);
        return -1;
    }
    int offset = cache_touch(sc, address);
    int block = address / 10;
    int line_index = block % 5;
    sc->memory[address] = value;
    sc->cache[line_index].data[offset] = value;
    return 0;
}

int sc_memory_get(SCComputer *sc, int address, int16_t *value) {
    if (!in_bounds(address)) {
        sc_flag_set(sc, SC_FLAG_MEMORY, true);
        return -1;
    }
    int offset = cache_touch(sc, address);
    int block = address / 10;
    int line_index = block % 5;
    *value = sc->cache[line_index].data[offset];
    return 0;
}

int sc_memory_save(const SCComputer *sc, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        fprintf(f, "%d %d\n", i, sc->memory[i]);
    }
    fclose(f);
    return 0;
}

int sc_memory_load(SCComputer *sc, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        sc->memory[i] = 0;
    }
    for (int i = 0; i < 5; ++i) {
        sc->cache[i].valid = false;
        sc->cache[i].tag = 0;
        for (int j = 0; j < 10; ++j) sc->cache[i].data[j] = 0;
    }

    int addr;
    int val;
    while (fscanf(f, "%d %d", &addr, &val) == 2) {
        if (in_bounds(addr)) sc->memory[addr] = (int16_t)val;
    }
    fclose(f);
    return 0;
}

int sc_command_encode(uint8_t opcode, uint8_t operand, int16_t *encoded) {
    if (!encoded) return -1;
    if (opcode > SC_OPCODE_MASK || operand > SC_OPERAND_MASK) return -1;
    uint16_t word = (1u << SC_COMMAND_SIGN_BIT) |
                    ((uint16_t)opcode << SC_OPCODE_SHIFT) |
                    (uint16_t)operand;
    *encoded = (int16_t)word;
    return 0;
}

int sc_command_decode(int16_t encoded, uint8_t *opcode, uint8_t *operand) {
    uint16_t word = (uint16_t)encoded & SC_WORD_MASK;
    if (((word >> SC_COMMAND_SIGN_BIT) & 1u) == 0u) return -1;
    if (opcode) *opcode = (uint8_t)((word >> SC_OPCODE_SHIFT) & SC_OPCODE_MASK);
    if (operand) *operand = (uint8_t)(word & SC_OPERAND_MASK);
    return 0;
}

static int exec_binary_math(SCComputer *sc, uint8_t operand, char op) {
    int16_t v = 0;
    if (sc_memory_get(sc, operand, &v) != 0) return -1;

    int32_t res = 0;
    if (op == '+') res = (int32_t)sc->accumulator + (int32_t)v;
    if (op == '-') res = (int32_t)sc->accumulator - (int32_t)v;
    if (op == '*') res = (int32_t)sc->accumulator * (int32_t)v;
    if (op == '/') {
        if (v == 0) {
            sc_flag_set(sc, SC_FLAG_DIV_BY_ZERO, true);
            sc->halted = true;
            return -1;
        }
        res = (int32_t)sc->accumulator / (int32_t)v;
    }

    bool overflowed = false;
    sc->accumulator = sc_clamp15(res, &overflowed);
    sc_flag_set(sc, SC_FLAG_OVERFLOW, overflowed);
    return 0;
}

int sc_step(SCComputer *sc) {
    if (sc->halted) return 1;

    if (!in_bounds(sc->instruction_counter)) {
        sc_flag_set(sc, SC_FLAG_MEMORY, true);
        return -1;
    }

    int16_t raw = 0;
    if (sc_memory_get(sc, sc->instruction_counter, &raw) != 0) return -1;

    uint8_t opcode = 0;
    uint8_t operand = 0;
    if (sc_command_decode(raw, &opcode, &operand) != 0) {
        sc_flag_set(sc, SC_FLAG_COMMAND, true);
        return -1;
    }

    int jumped = 0;
    switch (opcode) {
        case SC_READ: {
            int v = 0;
            printf("READ [%u] > ", operand);
            if (scanf("%d", &v) != 1) return -1;
            sc->io_cycles += 10;
            bool overflowed = false;
            sc_memory_set(sc, operand, sc_clamp15(v, &overflowed));
            sc_flag_set(sc, SC_FLAG_OVERFLOW, overflowed);
            break;
        }
        case SC_WRITE: {
            int16_t v = 0;
            sc_memory_get(sc, operand, &v);
            printf("WRITE [%u] = %d\n", operand, v);
            sc->io_cycles += 10;
            break;
        }
        case SC_LOAD:
            sc_memory_get(sc, operand, &sc->accumulator);
            break;
        case SC_STORE:
            sc_memory_set(sc, operand, sc->accumulator);
            break;
        case SC_ADD:
            if (exec_binary_math(sc, operand, '+') != 0) return -1;
            break;
        case SC_SUB:
            if (exec_binary_math(sc, operand, '-') != 0) return -1;
            break;
        case SC_DIVIDE:
            if (exec_binary_math(sc, operand, '/') != 0) return -1;
            break;
        case SC_MUL:
            if (exec_binary_math(sc, operand, '*') != 0) return -1;
            break;
        case SC_AND: {
            int16_t v = 0;
            sc_memory_get(sc, operand, &v);
            sc->accumulator = (int16_t)((sc->accumulator & v) & SC_WORD_MASK);
            break;
        }
        case SC_OR: {
            int16_t v = 0;
            sc_memory_get(sc, operand, &v);
            sc->accumulator = (int16_t)((sc->accumulator | v) & SC_WORD_MASK);
            break;
        }
        case SC_XOR: {
            int16_t v = 0;
            sc_memory_get(sc, operand, &v);
            sc->accumulator = (int16_t)((sc->accumulator ^ v) & SC_WORD_MASK);
            break;
        }
        case SC_NOT:
            sc->accumulator = (int16_t)(~sc->accumulator & SC_WORD_MASK);
            break;
        case SC_SHL: {
            int shift = operand & 0x0F;
            int32_t v = ((int32_t)sc->accumulator) << shift;
            bool overflowed = false;
            sc->accumulator = sc_clamp15(v, &overflowed);
            sc_flag_set(sc, SC_FLAG_OVERFLOW, overflowed);
            break;
        }
        case SC_SHR: {
            int shift = operand & 0x0F;
            sc->accumulator = (int16_t)(sc->accumulator >> shift);
            break;
        }
        case SC_JUMP:
            sc->instruction_counter = operand;
            jumped = 1;
            break;
        case SC_JNEG:
            if (sc->accumulator < 0) {
                sc->instruction_counter = operand;
                jumped = 1;
            }
            break;
        case SC_JZ:
            if (sc->accumulator == 0) {
                sc->instruction_counter = operand;
                jumped = 1;
            }
            break;
        case SC_HALT:
            sc->halted = true;
            return 1;
        default:
            sc_flag_set(sc, SC_FLAG_COMMAND, true);
            return -1;
    }

    if (!jumped) sc->instruction_counter++;
    return sc->halted ? 1 : 0;
}

void sc_run(SCComputer *sc, int max_steps) {
    for (int i = 0; i < max_steps || max_steps < 0; ++i) {
        int rc = sc_step(sc);
        if (rc != 0) break;
    }
}

void sc_dump_registers(const SCComputer *sc) {
    printf("ACC=%d IC=%u FLAGS=[P:%d Z:%d M:%d T:%d E:%d]\n",
           sc->accumulator,
           sc->instruction_counter,
           (sc->flags & SC_FLAG_OVERFLOW) ? 1 : 0,
           (sc->flags & SC_FLAG_DIV_BY_ZERO) ? 1 : 0,
           (sc->flags & SC_FLAG_MEMORY) ? 1 : 0,
           (sc->flags & SC_FLAG_IGNORE_CLOCK) ? 1 : 0,
           (sc->flags & SC_FLAG_COMMAND) ? 1 : 0);
    printf("MEM_ACCESS=%llu HIT=%llu MISS=%llu STALL=%llu IO=%llu\n",
           (unsigned long long)sc->memory_accesses,
           (unsigned long long)sc->cache_hits,
           (unsigned long long)sc->cache_misses,
           (unsigned long long)sc->stall_cycles,
           (unsigned long long)sc->io_cycles);
}

void sc_dump_memory(const SCComputer *sc) {
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        printf("%03d:%6d  ", i, sc->memory[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
}

void sc_dump_cache(const SCComputer *sc) {
    for (int i = 0; i < 5; ++i) {
        const SCCacheLine *line = &sc->cache[i];
        printf("L%d valid=%d tag=%u |", i, line->valid ? 1 : 0, line->tag);
        for (int j = 0; j < 10; ++j) printf(" %d", line->data[j]);
        printf("\n");
    }
}

// ---------------------------------------------------------------------------
// Lab-style API with global state (memory/registers/command codec/diagnostics)
// ---------------------------------------------------------------------------

static int g_memory[SC_MEMORY_SIZE];
static int g_accumulator = 0;
static int g_icounter = 0;
static int g_flags = 0;

static int g_in_bounds(int address) {
    return address >= 0 && address < SC_MEMORY_SIZE;
}

static int g_fit_15bit(int value) {
    return value >= SC_MIN_VALUE && value <= SC_MAX_VALUE;
}

int sc_memoryInit(void) {
    for (int i = 0; i < SC_MEMORY_SIZE; ++i) g_memory[i] = 0;
    return 0;
}

int sc_memorySet(int address, int value) {
    if (!g_in_bounds(address)) {
        sc_regSet(SC_M, 1);
        return -1;
    }
    if (!g_fit_15bit(value)) return -1;
    g_memory[address] = value;
    return 0;
}

int sc_memoryGet(int address, int *value) {
    if (!value) return -1;
    if (!g_in_bounds(address)) {
        sc_regSet(SC_M, 1);
        return -1;
    }
    *value = g_memory[address];
    return 0;
}

int sc_memorySave(const char *filename) {
    if (!filename) return -1;
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    size_t wr = fwrite(g_memory, sizeof(g_memory[0]), SC_MEMORY_SIZE, f);
    fclose(f);
    return (wr == SC_MEMORY_SIZE) ? 0 : -1;
}

int sc_memoryLoad(const char *filename) {
    if (!filename) return -1;
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    int temp[SC_MEMORY_SIZE];
    size_t rd = fread(temp, sizeof(temp[0]), SC_MEMORY_SIZE, f);
    fclose(f);
    if (rd != SC_MEMORY_SIZE) return -1;

    for (int i = 0; i < SC_MEMORY_SIZE; ++i) {
        if (!g_fit_15bit(temp[i])) return -1;
    }
    memcpy(g_memory, temp, sizeof(g_memory));
    return 0;
}

int sc_regInit(void) {
    g_flags = 0;
    return 0;
}

int sc_regSet(int reg, int value) {
    if (!(reg == SC_P || reg == SC_0 || reg == SC_M || reg == SC_T || reg == SC_E)) return -1;
    if (!(value == 0 || value == 1)) return -1;

    if (value) g_flags |= reg;
    else g_flags &= ~reg;
    return 0;
}

int sc_regGet(int reg, int *value) {
    if (!value) return -1;
    if (!(reg == SC_P || reg == SC_0 || reg == SC_M || reg == SC_T || reg == SC_E)) return -1;
    *value = (g_flags & reg) ? 1 : 0;
    return 0;
}

int sc_accumulatorInit(void) {
    g_accumulator = 0;
    return 0;
}

int sc_accumulatorSet(int value) {
    if (!g_fit_15bit(value)) return -1;
    g_accumulator = value;
    return 0;
}

int sc_accumulatorGet(int *value) {
    if (!value) return -1;
    *value = g_accumulator;
    return 0;
}

int sc_icounterInit(void) {
    g_icounter = 0;
    return 0;
}

int sc_icounterSet(int value) {
    if (!g_in_bounds(value)) return -1;
    g_icounter = value;
    return 0;
}

int sc_icounterGet(int *value) {
    if (!value) return -1;
    *value = g_icounter;
    return 0;
}

int sc_commandValidate(int command) {
    static const int valid[] = {
        SC_READ, SC_WRITE, SC_LOAD, SC_STORE, SC_ADD, SC_SUB, SC_DIVIDE, SC_MUL,
        SC_JUMP, SC_JNEG, SC_JZ, SC_HALT, SC_AND, SC_OR, SC_XOR, SC_NOT, SC_SHL, SC_SHR
    };
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        if (valid[i] == command) return 0;
    }
    return -1;
}

int sc_commandEncode(int sign, int command, int operand, int *value) {
    if (!value) return -1;
    if (!(sign == 0 || sign == 1)) return -1;
    if (sc_commandValidate(command) != 0) return -1;
    if (operand < 0 || operand > SC_OPERAND_MASK) return -1;

    int encoded = ((sign & 0x1) << SC_COMMAND_SIGN_BIT) |
                  ((command & SC_OPCODE_MASK) << SC_OPCODE_SHIFT) |
                  (operand & SC_OPERAND_MASK);
    *value = encoded;
    return 0;
}

int sc_commandDecode(int value, int *sign, int *command, int *operand) {
    int masked = value & SC_WORD_MASK;
    int s = (masked >> SC_COMMAND_SIGN_BIT) & 0x1;
    int c = (masked >> SC_OPCODE_SHIFT) & SC_OPCODE_MASK;
    int o = masked & SC_OPERAND_MASK;

    if (sc_commandValidate(c) != 0) return -1;

    if (sign) *sign = s;
    if (command) *command = c;
    if (operand) *operand = o;
    return 0;
}

static void print_binary15(int value) {
    for (int i = 14; i >= 0; --i) {
        putchar(((value >> i) & 1) ? '1' : '0');
        if (i % 4 == 0 && i != 0) putchar(' ');
    }
}

void printCell(int address) {
    int value = 0;
    if (sc_memoryGet(address, &value) != 0) {
        printf("[%03d] <invalid address>\n", address);
        return;
    }

    int sign = 0, cmd = 0, op = 0;
    if (sc_commandDecode(value, &sign, &cmd, &op) == 0) {
        printf("[%03d] CMD %c %02d %02d (raw=%d)\n", address, sign ? '-' : '+', cmd, op, value);
    } else {
        printf("[%03d] DATA %d\n", address, value);
    }
}

void printFlags(void) {
    int p = 0, z = 0, m = 0, t = 0, e = 0;
    sc_regGet(SC_P, &p);
    sc_regGet(SC_0, &z);
    sc_regGet(SC_M, &m);
    sc_regGet(SC_T, &t);
    sc_regGet(SC_E, &e);
    printf("%c %c %c %c %c\n",
           p ? 'P' : '_',
           z ? '0' : '_',
           m ? 'M' : '_',
           t ? 'T' : '_',
           e ? 'E' : '_');
}

void printDecodedCommand(int value) {
    int masked = value & SC_WORD_MASK;
    printf("dec=%d oct=%o hex=%04X bin=", value, masked, masked);
    print_binary15(masked);
    printf("\n");
}

void printAccumulator(void) {
    printf("ACC=%d\n", g_accumulator);
}

void printCounters(void) {
    printf("IC=%d\n", g_icounter);
}
