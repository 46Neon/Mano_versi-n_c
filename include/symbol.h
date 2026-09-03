#ifndef MANO_SYMBOL_H
#define MANO_SYMBOL_H

#include "common.h"
#include "ast.h"

typedef enum {
    SYMBOL_DATASET,
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_COLUMN,
    SYMBOL_METRIC
} SymbolType;

typedef struct Symbol {
    char *name;
    SymbolType type;
    ASTNode *declaration;
    void *data;
    struct Symbol *next;
    struct Symbol *prev;
} Symbol;

typedef struct SymbolTable {
    Symbol **symbols;
    size_t count;
    size_t capacity;
    struct SymbolTable *parent;
    int scope_level;
} SymbolTable;

SymbolTable* symbol_table_create(void);
void symbol_table_destroy(SymbolTable *table);
bool symbol_table_insert(SymbolTable *table, Symbol *symbol);
Symbol* symbol_table_lookup(SymbolTable *table, const char *name);
Symbol* symbol_table_lookup_local(SymbolTable *table, const char *name);
void symbol_table_enter_scope(SymbolTable **table);
void symbol_table_exit_scope(SymbolTable **table);
Symbol* symbol_create(const char *name, SymbolType type);
void symbol_table_print(SymbolTable *table);

#endif
