#ifndef MANO_COMPILER_H
#define MANO_COMPILER_H

#include "common.h"
#include "ast.h"
#include "ir.h"
#include "symbol.h"

typedef struct Compiler {
    ASTNode *ast;
    IRProgram *ir;
    SymbolTable *symbols;
    ManoErrorInfo error;
    bool has_error;
} Compiler;

bool compiler_init(Compiler *compiler);
bool compiler_compile(Compiler *compiler, ASTNode *ast);
IRProgram* compiler_get_ir(Compiler *compiler);
void compiler_destroy(Compiler *compiler);

#endif
