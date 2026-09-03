#ifndef MANO_VM_H
#define MANO_VM_H

#include "common.h"
#include "ir.h"
#include "dataset.h"
#include "gc.h"

typedef struct VirtualMachine {
    IRProgram *program;
    size_t pc;
    Dataset *dataset;
    Dataset *result;
    GC *gc;
    bool running;
    bool has_error;
    ManoErrorInfo error;
} VirtualMachine;

bool vm_init(VirtualMachine *vm, IRProgram *program);
bool vm_run(VirtualMachine *vm);
void vm_step(VirtualMachine *vm);
void vm_destroy(VirtualMachine *vm);

#endif
