#ifndef SC_TYPES_H
#define SC_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define SC_MEMORY_SIZE 128
#define SC_WORD_MASK 0x7FFF
#define SC_COMMAND_SIGN_BIT 14
#define SC_OPCODE_SHIFT 7
#define SC_OPCODE_MASK 0x7F
#define SC_OPERAND_MASK 0x7F
#define SC_MIN_VALUE (-16384)
#define SC_MAX_VALUE 16383

typedef enum {
    SC_FLAG_OVERFLOW = 1 << 0,       // P
    SC_FLAG_DIV_BY_ZERO = 1 << 1,    // 0
    SC_FLAG_MEMORY = 1 << 2,         // M
    SC_FLAG_IGNORE_CLOCK = 1 << 3,   // T
    SC_FLAG_COMMAND = 1 << 4         // E
} SCFlag;

typedef enum {
    SC_READ = 10,
    SC_WRITE = 11,
    SC_LOAD = 20,
    SC_STORE = 21,
    SC_ADD = 30,
    SC_SUB = 31,
    SC_DIVIDE = 32,
    SC_MUL = 33,
    SC_JUMP = 40,
    SC_JNEG = 41,
    SC_JZ = 42,
    SC_HALT = 43,
    SC_AND = 52,
    SC_OR = 53,
    SC_XOR = 54,
    SC_NOT = 55,
    SC_SHL = 56,
    SC_SHR = 57
} SCOpcode;

typedef struct {
    bool valid;
    uint8_t tag;
    int16_t data[10];
} SCCacheLine;

#endif
