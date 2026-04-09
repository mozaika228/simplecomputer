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

// Lab-style API (global SC state)
#define SC_P 0x01
#define SC_0 0x02
#define SC_M 0x04
#define SC_T 0x08
#define SC_E 0x10

int sc_memoryInit(void);
int sc_memorySet(int address, int value);
int sc_memoryGet(int address, int *value);
int sc_memorySave(const char *filename);
int sc_memoryLoad(const char *filename);

int sc_regInit(void);
int sc_regSet(int reg, int value);
int sc_regGet(int reg, int *value);

int sc_accumulatorInit(void);
int sc_accumulatorSet(int value);
int sc_accumulatorGet(int *value);

int sc_icounterInit(void);
int sc_icounterSet(int value);
int sc_icounterGet(int *value);

int sc_commandEncode(int sign, int command, int operand, int *value);
int sc_commandDecode(int value, int *sign, int *command, int *operand);
int sc_commandValidate(int command);

void printCell(int address);
void printFlags(void);
void printDecodedCommand(int value);
void printAccumulator(void);
void printCounters(void);

#endif
