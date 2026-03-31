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
