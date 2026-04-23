CC ?= gcc
AR ?= ar
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
PR01_OBJ := $(BUILD_DIR)/pr01.o

EXE_EXT :=
ifeq ($(OS),Windows_NT)
  EXE_EXT := .exe
endif

CONSOLE_BIN := $(BUILD_DIR)/console$(EXE_EXT)
SIMPLEASM_BIN := $(BUILD_DIR)/simpleassembler$(EXE_EXT)
SIMPLEBASIC_BIN := $(BUILD_DIR)/simplebasic$(EXE_EXT)
PR01_BIN := $(BUILD_DIR)/pr01$(EXE_EXT)
MYTERM_LIB := myTerm/libmyTerm.a

.PHONY: all clean run examples pr01

all: $(MYTERM_LIB) $(CONSOLE_BIN) $(SIMPLEASM_BIN) $(SIMPLEBASIC_BIN) $(PR01_BIN)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(CORE_OBJ): $(CORE_SRC) include/mySimpleComputer.h include/sc_types.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TERM_OBJ): $(TERM_SRC) include/myTerm.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MYTERM_LIB): $(TERM_OBJ)
	$(AR) rcs $@ $^

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

$(PR01_OBJ): pr01.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CONSOLE_BIN): $(CONSOLE_OBJ) $(CORE_OBJ) $(MYTERM_LIB) $(READKEY_OBJ) $(BIGCHARS_OBJ) $(ASM_OBJ) $(BASIC_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(SIMPLEASM_BIN): $(SIMPLEASM_MAIN_OBJ) $(ASM_OBJ) $(CORE_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(SIMPLEBASIC_BIN): $(SIMPLEBASIC_MAIN_OBJ) $(BASIC_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(PR01_BIN): $(PR01_OBJ) $(CORE_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

examples: all
	$(SIMPLEBASIC_BIN) examples/sum.sb examples/sum_generated.sa
	$(SIMPLEASM_BIN) examples/sum_generated.sa examples/sum.sc

pr01: $(PR01_BIN)
	$(PR01_BIN)

run: all
	$(CONSOLE_BIN) examples/sum.sc

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(MYTERM_LIB)
