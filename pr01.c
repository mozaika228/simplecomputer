#include "mySimpleComputer.h"

#include <stdio.h>

static void print_rc(const char *name, int rc) {
    printf("%s -> %s\n", name, rc == 0 ? "OK" : "ERR");
}

int main(void) {
    int rc;
    int value = 0;
    int sign = 0, command = 0, operand = 0;

    puts("=== PR01: mySimpleComputer lab test ===");

    rc = sc_memoryInit();
    print_rc("sc_memoryInit", rc);

    rc = sc_regInit();
    print_rc("sc_regInit", rc);

    rc = sc_accumulatorInit();
    print_rc("sc_accumulatorInit", rc);

    rc = sc_icounterInit();
    print_rc("sc_icounterInit", rc);

    puts("\n-- Memory set/get --");
    print_rc("sc_memorySet(10, 1234)", sc_memorySet(10, 1234));
    print_rc("sc_memoryGet(10, &value)", sc_memoryGet(10, &value));
    printf("memory[10] = %d\n", value);
    print_rc("sc_memorySet(130, 1)", sc_memorySet(130, 1));
    print_rc("sc_memorySet(5, 50000)", sc_memorySet(5, 50000));

    puts("\n-- Flags --");
    print_rc("sc_regSet(SC_P, 1)", sc_regSet(SC_P, 1));
    print_rc("sc_regSet(SC_0, 1)", sc_regSet(SC_0, 1));
    print_rc("sc_regSet(SC_M, 0)", sc_regSet(SC_M, 0));
    printFlags();
    sc_regGet(SC_P, &value);
    printf("SC_P = %d\n", value);

    puts("\n-- Accumulator and counter --");
    print_rc("sc_accumulatorSet(777)", sc_accumulatorSet(777));
    print_rc("sc_icounterSet(42)", sc_icounterSet(42));
    printAccumulator();
    printCounters();

    puts("\n-- Encode/decode --");
    rc = sc_commandEncode(0, SC_LOAD, 15, &value);
    print_rc("sc_commandEncode(+, LOAD, 15)", rc);
    if (rc == 0) {
        printDecodedCommand(value);
        rc = sc_commandDecode(value, &sign, &command, &operand);
        print_rc("sc_commandDecode", rc);
        if (rc == 0) {
            printf("Decoded: sign=%d command=%d operand=%d\n", sign, command, operand);
        }
    }
    print_rc("sc_commandValidate(99)", sc_commandValidate(99));

    puts("\n-- Save/load binary memory --");
    print_rc("sc_memorySave(\"pr01_memory.bin\")", sc_memorySave("pr01_memory.bin"));
    print_rc("sc_memorySet(10, 0)", sc_memorySet(10, 0));
    print_rc("sc_memoryLoad(\"pr01_memory.bin\")", sc_memoryLoad("pr01_memory.bin"));
    print_rc("sc_memoryGet(10, &value)", sc_memoryGet(10, &value));
    printf("memory[10] after load = %d\n", value);
    printCell(10);

    puts("\n=== PR01 done ===");
    return 0;
}
