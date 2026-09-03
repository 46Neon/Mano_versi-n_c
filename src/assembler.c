#include "assembler.h"

Assembler* assembler_create(void) {
    Assembler *assembler = (Assembler *)calloc(1, sizeof(Assembler));
    if (!assembler) return NULL;
    
    assembler->instructions = NULL;
    assembler->count = 0;
    assembler->capacity = 0;
    
    return assembler;
}

void assembler_destroy(Assembler *assembler) {
    if (!assembler) return;
    
    free(assembler->instructions);
    free(assembler);
}

static OpCode ir_to_opcode(IROpCode ir_op) {
    switch (ir_op) {
        case IR_LOAD_DATASET: return OP_LOAD;
        case IR_CLEAN_NULLS: return OP_LOAD;
        case IR_CLEAN_DUPLICATES: return OP_LOAD;
        case IR_TRANSFORM_TOTAL: return OP_LOAD;
        case IR_TRANSFORM_PERIOD: return OP_LOAD;
        case IR_FILTER_CONDITION: return OP_LOAD;
        case IR_GROUP_BY: return OP_LOAD;
        case IR_AGGREGATE_SUM: return OP_ADD;
        case IR_AGGREGATE_AVG: return OP_LOAD;
        case IR_AGGREGATE_MIN: return OP_LOAD;
        case IR_AGGREGATE_MAX: return OP_LOAD;
        case IR_VISUALIZE: return OP_PRINT;
        case IR_EXPORT_JSON: return OP_LOAD;
        case IR_PRINT: return OP_PRINT;
        default: return OP_HALT;
    }
}

bool assembler_assemble(Assembler *assembler, IRProgram *ir) {
    if (!assembler || !ir) return false;
    
    // Limpiar instrucciones anteriores
    free(assembler->instructions);
    assembler->instructions = NULL;
    assembler->count = 0;
    assembler->capacity = 0;
    
    // Convertir cada instrucción IR a instrucción de máquina
    for (size_t i = 0; i < ir->count; i++) {
        IRInstruction *ir_ins = &ir->instructions[i];
        
        if (assembler->count >= assembler->capacity) {
            size_t new_capacity = assembler->capacity == 0 ? 16 : assembler->capacity * 2;
            Instruction *new_ins = (Instruction *)realloc(assembler->instructions,
                                                          new_capacity * sizeof(Instruction));
            if (!new_ins) return false;
            
            assembler->instructions = new_ins;
            assembler->capacity = new_capacity;
        }
        
        Instruction *ins = &assembler->instructions[assembler->count++];
        ins->opcode = ir_to_opcode(ir_ins->opcode);
        ins->operand1 = 0;
        ins->operand2 = 0;
        ins->operand3 = 0;
        
        // Asignar operandos según tipo
        if (ir_ins->arg1) {
            ins->operand1 = (int)strlen(ir_ins->arg1);
        }
        if (ir_ins->arg2) {
            ins->operand2 = (int)strlen(ir_ins->arg2);
        }
        if (ir_ins->num_arg1 != 0.0) {
            ins->operand1 = (int)ir_ins->num_arg1;
        }
    }
    
    return true;
}

void assembler_print(Assembler *assembler) {
    if (!assembler) return;
    
    printf("Código ensamblado (%zu instrucciones):\n", assembler->count);
    for (size_t i = 0; i < assembler->count; i++) {
        Instruction *ins = &assembler->instructions[i];
        printf("[%03zu] ", i);
        
        switch (ins->opcode) {
            case OP_LOAD: printf("LOAD"); break;
            case OP_STORE: printf("STORE"); break;
            case OP_ADD: printf("ADD"); break;
            case OP_SUB: printf("SUB"); break;
            case OP_MUL: printf("MUL"); break;
            case OP_DIV: printf("DIV"); break;
            case OP_CMP: printf("CMP"); break;
            case OP_JMP: printf("JMP"); break;
            case OP_JZ: printf("JZ"); break;
            case OP_JNZ: printf("JNZ"); break;
            case OP_CALL: printf("CALL"); break;
            case OP_RET: printf("RET"); break;
            case OP_HALT: printf("HALT"); break;
            case OP_PRINT: printf("PRINT"); break;
            default: printf("UNKNOWN"); break;
        }
        
        printf(" %d %d %d\n", ins->operand1, ins->operand2, ins->operand3);
    }
}

Instruction* assembler_get_instructions(Assembler *assembler, size_t *count) {
    if (!assembler || !count) return NULL;
    
    *count = assembler->count;
    return assembler->instructions;
}
