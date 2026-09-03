#include "symbol.h"

SymbolTable* symbol_table_create(void) {
    SymbolTable *table = (SymbolTable *)calloc(1, sizeof(SymbolTable));
    if (!table) return NULL;
    
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    table->parent = NULL;
    table->scope_level = 0;
    
    return table;
}

void symbol_table_destroy(SymbolTable *table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->count; i++) {
        if (table->symbols[i]) {
            free(table->symbols[i]->name);
            free(table->symbols[i]);
        }
    }
    
    free(table->symbols);
    free(table);
}

bool symbol_table_insert(SymbolTable *table, Symbol *symbol) {
    if (!table || !symbol) return false;
    
    // Verificar si ya existe
    Symbol *existing = symbol_table_lookup_local(table, symbol->name);
    if (existing) return false;
    
    // Expandir si es necesario
    if (table->count >= table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 16 : table->capacity * 2;
        Symbol **new_symbols = (Symbol **)realloc(table->symbols, new_capacity * sizeof(Symbol *));
        if (!new_symbols) return false;
        
        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }
    
    table->symbols[table->count++] = symbol;
    return true;
}

Symbol* symbol_table_lookup(SymbolTable *table, const char *name) {
    if (!table || !name) return NULL;
    
    Symbol *symbol = symbol_table_lookup_local(table, name);
    if (symbol) return symbol;
    
    if (table->parent) {
        return symbol_table_lookup(table->parent, name);
    }
    
    return NULL;
}

Symbol* symbol_table_lookup_local(SymbolTable *table, const char *name) {
    if (!table || !name) return NULL;
    
    for (size_t i = 0; i < table->count; i++) {
        if (table->symbols[i] && strcmp(table->symbols[i]->name, name) == 0) {
            return table->symbols[i];
        }
    }
    
    return NULL;
}

void symbol_table_enter_scope(SymbolTable **table) {
    if (!table) return;
    
    SymbolTable *new_table = symbol_table_create();
    if (!new_table) return;
    
    new_table->parent = *table;
    new_table->scope_level = (*table) ? (*table)->scope_level + 1 : 1;
    *table = new_table;
}

void symbol_table_exit_scope(SymbolTable **table) {
    if (!table || !*table) return;
    
    SymbolTable *parent = (*table)->parent;
    symbol_table_destroy(*table);
    *table = parent;
}

Symbol* symbol_create(const char *name, SymbolType type) {
    Symbol *symbol = (Symbol *)calloc(1, sizeof(Symbol));
    if (!symbol) return NULL;
    
    symbol->name = (char *)malloc(strlen(name) + 1);
    if (!symbol->name) {
        free(symbol);
        return NULL;
    }
    
    strcpy(symbol->name, name);
    symbol->type = type;
    symbol->declaration = NULL;
    symbol->data = NULL;
    symbol->next = NULL;
    symbol->prev = NULL;
    
    return symbol;
}

void symbol_table_print(SymbolTable *table) {
    if (!table) return;
    
    printf("Tabla de símbolos (nivel %d):\n", table->scope_level);
    for (size_t i = 0; i < table->count; i++) {
        Symbol *s = table->symbols[i];
        if (s) {
            printf("  %s (", s->name);
            switch (s->type) {
                case SYMBOL_DATASET: printf("DATASET"); break;
                case SYMBOL_VARIABLE: printf("VARIABLE"); break;
                case SYMBOL_FUNCTION: printf("FUNCTION"); break;
                case SYMBOL_COLUMN: printf("COLUMN"); break;
                case SYMBOL_METRIC: printf("METRIC"); break;
            }
            printf(")\n");
        }
    }
    
    if (table->parent) {
        printf("  -> padre (nivel %d)\n", table->parent->scope_level);
    }
}
