#ifndef MANO_INSTRUCTIONS_H
#define MANO_INSTRUCTIONS_H

#include "common.h"

typedef enum {
    INSTR_LOAD = 0x01,
    INSTR_STORE = 0x02,
    INSTR_ADD = 0x03,
    INSTR_SUB = 0x04,
    INSTR_MUL = 0x05,
    INSTR_DIV = 0x06,
    INSTR_CMP = 0x07,
    INSTR_JMP = 0x08,
    INSTR_JZ = 0x09,
    INSTR_JNZ = 0x0A,
    INSTR_CALL = 0x0B,
    INSTR_RET = 0x0C,
    INSTR_HALT = 0x0D,
    INSTR_PRINT = 0x0E,
    INSTR_PUSH = 0x0F,
    INSTR_POP = 0x10
} InstrOpcode;

typedef struct MachineInstruction {
    unsigned char opcode;
    unsigned char operand1;
    unsigned char operand2;
    unsigned char operand3;
} MachineInstruction;

size_t instruction_size(void);
const char* instruction_name(InstrOpcode opcode);

#endif
