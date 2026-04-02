# Simple Computer (SC)

Simple Computer is an educational software model of a minimal computer architecture with three levels:

1. `Simple Basic` (high-level language)
2. `Simple Assembler` (assembly translator)
3. `Simple Computer` (machine code emulator)

Execution pipeline:

`Simple Basic -> Simple Assembler -> Machine code -> Simple Computer`

## Project Structure

```text
simplecomputer/
├── console/
├── mySimpleComputer/
├── myTerm/
├── myReadKey/
├── myBigChars/
├── simpleassembler/
├── simplebasic/
├── include/
└── examples/
```

## SC Architecture

- RAM: 128 cells (`0..127`), 15-bit word.
- Instruction format: `[sign][opcode][operand]`, where:
- `sign` is bit 14
- `opcode` is bits 13..7
- `operand` is bits 6..0
- CPU includes:
- 15-bit accumulator
- instruction counter
- flags: `P`, `0`, `M`, `T`, `E`
- L1 cache model: 5 lines, 10 words per line
- stall cycle counters for memory and I/O delay simulation

## Build (CMake)

```bash
cmake -S . -B build
cmake --build build
```

Build output:

- `console`
- `simpleassembler`
- `simplebasic`

## Build (Makefile, no CMake)

If you have `gcc` and `make`, you can build without CMake:

```bash
make
```

Useful targets:

- `make` - build all binaries into `build/`
- `make examples` - compile `sum.sb -> sum_generated.sa -> sum.sc`
- `make run` - run console with `examples/sum.sc`
- `make clean` - remove build artifacts

## Quick Start

### 1) Compile Simple Basic to Assembly

```bash
build/simplebasic examples/sum.sb examples/sum_generated.sa
```

### 2) Assemble to Machine Code

```bash
build/simpleassembler examples/sum_generated.sa examples/sum.sc
```

### 3) Run in Emulator

```bash
build/console examples/sum.sc
```

Console interactive commands:

- `run` - run program until stop
- `step` - execute one instruction
- `regs` - print registers, flags, and timing stats
- `dump` - print memory dump
- `cache` - print L1 cache content
- `set <addr> <value>` - write memory cell manually
- `save <file.sc>` / `load <file.sc>`
- `asm <in.sa> <out.sc>`
- `basic <in.sb> <out.sa>`

## Supported Assembler Instructions

- `READ`, `WRITE`
- `LOAD`, `STORE`
- `ADD`, `SUB`, `MUL`, `DIVIDE`
- `JUMP`, `JNEG`, `JZ`, `HALT`
- `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR`
- `WORD` (data directive)

## Assembly Example

```asm
READ 100
READ 101
LOAD 100
ADD 101
STORE 102
WRITE 102
HALT
```

## Simple Basic Example

```basic
10 INPUT A
20 INPUT B
30 LET C = A + B
40 PRINT C
50 END
```
