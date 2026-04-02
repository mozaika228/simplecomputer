CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=

BUILD_DIR := build

CORE_SRC := mySimpleComputer/mySimpleComputer.c
TERM_SRC := myTerm/myTerm.c
READKEY_SRC := myReadKey/myReadKey.c
BIGCHARS_SRC := myBigChars/myBigChars.c
ASM_SRC := simpleassembler/simpleassembler.c
BASIC_SRC := simplebasic/simplebasic.c

CORE_OBJ := $(BUILD_DIR)/mySimpleComputer.o
TERM_OBJ := $(BUILD_DIR)/myTerm.o
READKEY_OBJ := $(BUILD_DIR)/myReadKey.o
BIGCHARS_OBJ := $(BUILD_DIR)/myBigChars.o
ASM_OBJ := $(BUILD_DIR)/simpleassembler_lib.o
BASIC_OBJ := $(BUILD_DIR)/simplebasic_lib.o

CONSOLE_OBJ := $(BUILD_DIR)/console_main.o
SIMPLEASM_MAIN_OBJ := $(BUILD_DIR)/simpleassembler_main.o
SIMPLEBASIC_MAIN_OBJ := $(BUILD_DIR)/simplebasic_main.o

EXE_EXT :=
ifeq ($(OS),Windows_NT)
  EXE_EXT := .exe
endif

CONSOLE_BIN := $(BUILD_DIR)/console$(EXE_EXT)
SIMPLEASM_BIN := $(BUILD_DIR)/simpleassembler$(EXE_EXT)
SIMPLEBASIC_BIN := $(BUILD_DIR)/simplebasic$(EXE_EXT)

.PHONY: all clean run examples

all: $(CONSOLE_BIN) $(SIMPLEASM_BIN) $(SIMPLEBASIC_BIN)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(CORE_OBJ): $(CORE_SRC) include/mySimpleComputer.h include/sc_types.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TERM_OBJ): $(TERM_SRC) include/myTerm.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(READKEY_OBJ): $(READKEY_SRC) include/myReadKey.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIGCHARS_OBJ): $(BIGCHARS_SRC) include/myBigChars.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_OBJ): $(ASM_SRC) include/simpleassembler.h include/mySimpleComputer.h include/sc_types.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BASIC_OBJ): $(BASIC_SRC) include/simplebasic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CONSOLE_OBJ): console/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SIMPLEASM_MAIN_OBJ): simpleassembler/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SIMPLEBASIC_MAIN_OBJ): simplebasic/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CONSOLE_BIN): $(CONSOLE_OBJ) $(CORE_OBJ) $(TERM_OBJ) $(READKEY_OBJ) $(BIGCHARS_OBJ) $(ASM_OBJ) $(BASIC_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(SIMPLEASM_BIN): $(SIMPLEASM_MAIN_OBJ) $(ASM_OBJ) $(CORE_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(SIMPLEBASIC_BIN): $(SIMPLEBASIC_MAIN_OBJ) $(BASIC_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

examples: all
	$(SIMPLEBASIC_BIN) examples/sum.sb examples/sum_generated.sa
	$(SIMPLEASM_BIN) examples/sum_generated.sa examples/sum.sc

run: all
	$(CONSOLE_BIN) examples/sum.sc

clean:
	rm -rf $(BUILD_DIR)
