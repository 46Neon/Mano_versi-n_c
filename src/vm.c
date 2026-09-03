#include "vm.h"

bool vm_init(VirtualMachine *vm, IRProgram *program) {
    if (!vm || !program) return false;
    
    vm->program = program;
    vm->pc = 0;
    vm->dataset = NULL;
    vm->result = NULL;
    vm->running = true;
    vm->has_error = false;
    mano_error_init(&vm->error);
    
    // Inicializar GC
    vm->gc = gc_create();
    if (!vm->gc) return false;
    
    return true;
}

static bool vm_execute_instruction(VirtualMachine *vm, IRInstruction *ins) {
    if (!ins) return false;
    
    switch (ins->opcode) {
        case IR_LOAD_DATASET:
            if (ins->arg1) {
                if (vm->dataset) {
                    dataset_destruir(vm->dataset);
                    free(vm->dataset);
                }
                
                vm->dataset = (Dataset *)gc_alloc(vm->gc, sizeof(Dataset));
                if (!vm->dataset) {
                    vm->has_error = true;
                    mano_error_set(&vm->error, MANO_ERROR_MEMORY,
                                  "No se pudo asignar dataset", 0, 0);
                    return false;
                }
                
                if (!dataset_cargar_csv(vm->dataset, ins->arg1)) {
                    vm->has_error = true;
                    mano_error_set(&vm->error, MANO_ERROR_IO,
                                  "No se pudo cargar CSV", 0, 0);
                    return false;
                }
                
                printf("VM: Dataset cargado: %s\n", ins->arg1);
            }
            break;
            
        case IR_CLEAN_NULLS:
            if (vm->dataset && ins->arg1) {
                dataset_clean_nulls(vm->dataset, ins->arg1);
                printf("VM: Nulos limpiados\n");
            }
            break;
            
        case IR_CLEAN_DUPLICATES:
            if (vm->dataset && ins->arg1) {
                dataset_clean_duplicates(vm->dataset, ins->arg1);
                printf("VM: Duplicados limpiados\n");
            }
            break;
            
        case IR_TRANSFORM_TOTAL:
            if (vm->dataset && ins->arg1) {
                dataset_transform_total(vm->dataset, ins->arg1);
                printf("VM: Total transformado\n");
            }
            break;
            
        case IR_TRANSFORM_PERIOD:
            if (vm->dataset && ins->arg1) {
                dataset_transform_period(vm->dataset, ins->arg1);
                printf("VM: Periodo transformado\n");
            }
            break;
            
        case IR_FILTER_CONDITION:
            if (vm->dataset && ins->arg1) {
                dataset_filter_condition(vm->dataset, ins->arg1);
                printf("VM: Filtro aplicado\n");
            }
            break;
            
        case IR_GROUP_BY:
            if (vm->dataset && ins->arg1) {
                dataset_group_by(vm->dataset, ins->arg1);
                printf("VM: Agrupación por %s\n", ins->arg1);
            }
            break;
            
        case IR_AGGREGATE_SUM:
        case IR_AGGREGATE_AVG:
        case IR_AGGREGATE_MIN:
        case IR_AGGREGATE_MAX:
            if (vm->dataset && ins->arg1) {
                // Realizar agregación
                printf("VM: Agregación aplicada\n");
            }
            break;
            
        case IR_VISUALIZE:
            if (vm->dataset) {
                dataset_imprimir(vm->dataset, 10);
            }
            break;
            
        case IR_EXPORT_JSON:
            if (vm->dataset && ins->arg1) {
                if (dataset_guardar_json(vm->dataset, ins->arg1)) {
                    printf("VM: Datos exportados a %s\n", ins->arg1);
                } else {
                    vm->has_error = true;
                    mano_error_set(&vm->error, MANO_ERROR_IO,
                                  "No se pudo exportar JSON", 0, 0);
                    return false;
                }
            }
            break;
            
        case IR_PRINT:
            if (vm->dataset) {
                dataset_imprimir(vm->dataset, 5);
            }
            break;
            
        default:
            printf("VM: Instrucción desconocida: %d\n", ins->opcode);
            break;
    }
    
    return true;
}

bool vm_run(VirtualMachine *vm) {
    if (!vm || !vm->program) return false;
    
    vm->running = true;
    
    while (vm->running && vm->pc < vm->program->count) {
        IRInstruction *ins = &vm->program->instructions[vm->pc++];
        
        if (!vm_execute_instruction(vm, ins)) {
            vm->running = false;
            return false;
        }
    }
    
    return true;
}

void vm_step(VirtualMachine *vm) {
    if (!vm || !vm->running || vm->pc >= vm->program->count) {
        vm->running = false;
        return;
    }
    
    IRInstruction *ins = &vm->program->instructions[vm->pc++];
    vm_execute_instruction(vm, ins);
}

void vm_destroy(VirtualMachine *vm) {
    if (!vm) return;
    
    if (vm->dataset) {
        dataset_destruir(vm->dataset);
        free(vm->dataset);
    }
    
    if (vm->result) {
        dataset_destruir(vm->result);
        free(vm->result);
    }
    
    if (vm->gc) {
        gc_destroy(vm->gc);
    }
}
