#ifndef MY_SIMPLE_COMPUTER_H
#define MY_SIMPLE_COMPUTER_H

#include "sc_types.h"

typedef struct {
    int16_t memory[SC_MEMORY_SIZE];
    int16_t accumulator;
    uint8_t instruction_counter;
    uint8_t flags;
    SCCacheLine cache[5];
    uint64_t memory_accesses;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t stall_cycles;
    uint64_t io_cycles;
    bool halted;
} SCComputer;

void sc_init(SCComputer *sc);
void sc_reset_cpu(SCComputer *sc);

int sc_memory_set(SCComputer *sc, int address, int16_t value);
int sc_memory_get(SCComputer *sc, int address, int16_t *value);
int sc_memory_save(const SCComputer *sc, const char *filename);
int sc_memory_load(SCComputer *sc, const char *filename);

void sc_flag_set(SCComputer *sc, SCFlag flag, bool enabled);
bool sc_flag_get(const SCComputer *sc, SCFlag flag);

int sc_command_encode(uint8_t opcode, uint8_t operand, int16_t *encoded);
int sc_command_decode(int16_t encoded, uint8_t *opcode, uint8_t *operand);

int sc_step(SCComputer *sc);
void sc_run(SCComputer *sc, int max_steps);

void sc_dump_registers(const SCComputer *sc);
void sc_dump_memory(const SCComputer *sc);
void sc_dump_cache(const SCComputer *sc);

int16_t sc_clamp15(int32_t value, bool *overflowed);

#endif
