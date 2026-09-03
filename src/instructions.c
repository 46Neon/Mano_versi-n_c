#include "instructions.h"

size_t instruction_size(void) {
    return sizeof(MachineInstruction);
}

const char* instruction_name(InstrOpcode opcode) {
    static const char *names[] = {
        "LOAD", "STORE", "ADD", "SUB", "MUL", "DIV",
        "CMP", "JMP", "JZ", "JNZ", "CALL", "RET",
        "HALT", "PRINT", "PUSH", "POP"
    };
    
    size_t count = sizeof(names) / sizeof(names[0]);
    if ((size_t)opcode >= count) return "UNKNOWN";
    return names[(size_t)opcode];
}
