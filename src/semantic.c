#include "semantic.h"

static bool semantic_analyze_node(SemanticAnalyzer *analyzer, ASTNode *node);
static bool semantic_analyze_program(SemanticAnalyzer *analyzer, ASTNode *node);
static bool semantic_analyze_bloque(SemanticAnalyzer *analyzer, ASTNode *node);
static bool semantic_analyze_declaracion(SemanticAnalyzer *analyzer, ASTNode *node);
static bool semantic_analyze_asignacion(SemanticAnalyzer *analyzer, ASTNode *node);

void semantic_init(SemanticAnalyzer *analyzer, ASTNode *ast) {
    analyzer->ast = ast;
    analyzer->symbols = symbol_table_create();
    analyzer->has_error = false;
    milena_error_init(&analyzer->error);
}

static void semantic_error(SemanticAnalyzer *analyzer, const char *msg, int line, int column) {
    milena_error_set(&analyzer->error, MILENA_ERROR_SEMANTIC, msg, line, column);
    analyzer->has_error = true;
}

static bool semantic_analyze_program(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    for (size_t i = 0; i < node->child_count; i++) {
        if (!semantic_analyze_node(analyzer, node->children[i])) {
            return false;
        }
    }
    
    return true;
}

static bool semantic_analyze_bloque(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    // Entrar en nuevo scope
    symbol_table_enter_scope(&analyzer->symbols);
    
    // Analizar nombre del bloque
    if (node->value) {
        Symbol *sym = symbol_create(node->value, SYMBOL_VARIABLE);
        if (!symbol_table_insert(analyzer->symbols, sym)) {
            semantic_error(analyzer, "Símbolo duplicado", node->line, node->column);
            return false;
        }
    }
    
    // Analizar hijos
    for (size_t i = 0; i < node->child_count; i++) {
        if (!semantic_analyze_node(analyzer, node->children[i])) {
            return false;
        }
    }
    
    // Salir del scope
    symbol_table_exit_scope(&analyzer->symbols);
    
    return true;
}

static bool semantic_analyze_declaracion(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    switch (node->type) {
        case AST_DECLARACION_DATOS:
            // #datos es una declaración especial, no necesita validación
            return true;
            
        case AST_DECLARACION_ESTADISTICA:
            // #estadistica es una declaración especial
            return true;
            
        case AST_BLOQUE_LIMPIAR:
        case AST_BLOQUE_TRANSFORMAR:
        case AST_BLOQUE_FILTRAR:
        case AST_BLOQUE_AGRUPAR:
        case AST_BLOQUE_RESUMIR:
        case AST_BLOQUE_VISUALIZAR:
        case AST_BLOQUE_EXPORTAR:
            // Analizar hijos del bloque
            for (size_t i = 0; i < node->child_count; i++) {
                if (!semantic_analyze_node(analyzer, node->children[i])) {
                    return false;
                }
            }
            return true;
            
        case AST_ASIGNACION_DATASET:
            return semantic_analyze_asignacion(analyzer, node);
            
        default:
            // Para otros nodos, analizar hijos
            for (size_t i = 0; i < node->child_count; i++) {
                if (!semantic_analyze_node(analyzer, node->children[i])) {
                    return false;
                }
            }
            return true;
    }
}

static bool semantic_analyze_asignacion(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    // Verificar que el dataset sea válido
    if (node->value) {
        Symbol *sym = symbol_table_lookup(analyzer->symbols, node->value);
        if (!sym) {
            Symbol *new_sym = symbol_create(node->value, SYMBOL_DATASET);
            if (!symbol_table_insert(analyzer->symbols, new_sym)) {
                semantic_error(analyzer, "No se pudo registrar dataset", node->line, node->column);
                return false;
            }
        }
    }
    
    // Analizar hijos (argumentos de cargar)
    for (size_t i = 0; i < node->child_count; i++) {
        if (!semantic_analyze_node(analyzer, node->children[i])) {
            return false;
        }
    }
    
    return true;
}

static bool semantic_analyze_comando(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    // Validar que los comandos tengan argumentos válidos
    switch (node->type) {
        case AST_COMANDO_NULOS:
        case AST_COMANDO_DUPLICADOS:
        case AST_COMANDO_CONDICION:
        case AST_COMANDO_EXTRAER:
        case AST_COMANDO_TOTAL:
        case AST_COMANDO_PERIODO:
            // Verificar que tenga valor
            if (!node->value) {
                semantic_error(analyzer, "Comando sin argumento", node->line, node->column);
                return false;
            }
            return true;
            
        default:
            return true;
    }
}

static bool semantic_analyze_node(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return true;
    
    // Analizar según el tipo de nodo
    switch (node->type) {
        case AST_PROGRAMA:
            return semantic_analyze_program(analyzer, node);
            
        case AST_BLOQUE_ANALISIS:
            return semantic_analyze_bloque(analyzer, node);
            
        case AST_DECLARACION_DATOS:
        case AST_DECLARACION_ESTADISTICA:
        case AST_ASIGNACION_DATASET:
        case AST_LLAMADA_CARGAR:
        case AST_BLOQUE_LIMPIAR:
        case AST_BLOQUE_TRANSFORMAR:
        case AST_BLOQUE_FILTRAR:
        case AST_BLOQUE_AGRUPAR:
        case AST_BLOQUE_RESUMIR:
        case AST_BLOQUE_VISUALIZAR:
        case AST_BLOQUE_EXPORTAR:
            return semantic_analyze_declaracion(analyzer, node);
            
        case AST_COMANDO_NULOS:
        case AST_COMANDO_DUPLICADOS:
        case AST_COMANDO_CONDICION:
        case AST_COMANDO_EXTRAER:
        case AST_COMANDO_TOTAL:
        case AST_COMANDO_PERIODO:
            return semantic_analyze_comando(analyzer, node);
            
        case AST_EXPRESION_LITERAL:
        case AST_EXPRESION_IDENTIFICADOR:
        case AST_EXPRESION_FUNCION:
        case AST_EXPRESION_OPERACION:
            // Analizar subexpresiones
            for (size_t i = 0; i < node->child_count; i++) {
                if (!semantic_analyze_node(analyzer, node->children[i])) {
                    return false;
                }
            }
            return true;
            
        default:
            // Para nodos desconocidos, solo analizar hijos
            for (size_t i = 0; i < node->child_count; i++) {
                if (!semantic_analyze_node(analyzer, node->children[i])) {
                    return false;
                }
            }
            return true;
    }
}

bool semantic_analyze(SemanticAnalyzer *analyzer) {
    if (!analyzer || !analyzer->ast) return false;
    
    // Analizar el AST completo
    if (!semantic_analyze_node(analyzer, analyzer->ast)) {
        return false;
    }
    
    return !analyzer->has_error;
}

void semantic_destroy(SemanticAnalyzer *analyzer) {
    if (!analyzer) return;
    
    if (analyzer->symbols) {
        symbol_table_destroy(analyzer->symbols);
        analyzer->symbols = NULL;
    }
}
