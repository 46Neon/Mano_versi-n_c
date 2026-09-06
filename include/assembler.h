#ifndef MILENA_ASSEMBLER_H
#define MILENA_ASSEMBLER_H

#include "common.h"
#include "ir.h"

typedef enum {
    OP_LOAD,
    OP_STORE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CMP,
    OP_JMP,
    OP_JZ,
    OP_JNZ,
    OP_CALL,
    OP_RET,
    OP_HALT,
    OP_PRINT
} OpCode;

typedef struct Instruction {
    OpCode opcode;
    int operand1;
    int operand2;
    int operand3;
} Instruction;

typedef struct Assembler {
    Instruction *instructions;
    size_t count;
    size_t capacity;
} Assembler;

Assembler* assembler_create(void);
void assembler_destroy(Assembler *assembler);
bool assembler_assemble(Assembler *assembler, IRProgram *ir);
void assembler_print(Assembler *assembler);
Instruction* assembler_get_instructions(Assembler *assembler, size_t *count);

#endif
