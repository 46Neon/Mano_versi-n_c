#ifndef MILENA_SEMANTIC_H
#define MILENA_SEMANTIC_H

#include "common.h"
#include "ast.h"
#include "symbol.h"

typedef struct SemanticAnalyzer {
    SymbolTable *symbols;
    ASTNode *ast;
    MilenaErrorInfo error;
    bool has_error;
} SemanticAnalyzer;

void semantic_init(SemanticAnalyzer *analyzer, ASTNode *ast);
bool semantic_analyze(SemanticAnalyzer *analyzer);
void semantic_destroy(SemanticAnalyzer *analyzer);

#endif
