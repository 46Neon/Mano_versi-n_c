#include "user_functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static char *copy_text(const char *text) {
    size_t n; char *copy;
    if (!text) return NULL;
    n = strlen(text) + 1; copy = (char *)malloc(n);
    if (copy) memcpy(copy, text, n);
    return copy;
}
static void set_error(char *error, size_t size, const char *message) {
    if (error && size) { snprintf(error, size, "%s", message); }
}
void milena_function_table_init(MilenaFunctionTable *table) { if (table) memset(table, 0, sizeof *table); }
void milena_function_destroy(MilenaFunction *fn) {
    size_t i;
    if (!fn) return;
    free(fn->name);
    for (i = 0; i < fn->parameter_count; ++i) free(fn->parameters[i].name);
    free(fn->parameters); free(fn);
}
void milena_function_table_release(MilenaFunctionTable *table) {
    size_t i; if (!table) return;
    for (i = 0; i < table->count; ++i) milena_function_destroy(table->items[i]);
    free(table->items); milena_function_table_init(table);
}
MilenaFunction *milena_function_table_find(const MilenaFunctionTable *table, const char *name) {
    size_t i;
    if (!table || !name) return NULL;
    for (i = 0; i < table->count; ++i) if (strcmp(table->items[i]->name, name) == 0) return table->items[i];
    return NULL;
}
bool milena_function_table_add(MilenaFunctionTable *table, MilenaFunction *fn) {
    MilenaFunction **new_items; size_t capacity;
    if (!table || !fn || !fn->name || milena_function_table_find(table, fn->name)) return false;
    if (table->count == table->capacity) {
        capacity = table->capacity ? table->capacity * 2 : 8;
        new_items = (MilenaFunction **)realloc(table->items, capacity * sizeof *new_items);
        if (!new_items) return false;
        table->items = new_items; table->capacity = capacity;
    }
    table->items[table->count++] = fn; return true;
}
static bool eval(const MilenaFunctionTable *table, const MilenaFunctionNode *node,
                 const MilenaFunction *fn, const double *args, size_t argc,
                 unsigned depth, double *out, char *error, size_t error_size) {
    double a, b; size_t i;
    if (!node || !out) { set_error(error,error_size,"Nodo de función inválido"); return false; }
    if (depth > 1024) { set_error(error,error_size,"Profundidad máxima de recursión excedida"); return false; }
    switch (node->type) {
    case MILENA_FN_NUMBER: *out = node->number; return true;
    case MILENA_FN_PARAMETER:
        if (node->index >= argc) { set_error(error,error_size,"Parámetro fuera de rango"); return false; }
        *out = args[node->index]; return true;
    case MILENA_FN_RETURN:
        return node->child_count == 1 && eval(table,node->children[0],fn,args,argc,depth,out,error,error_size);
    case MILENA_FN_ADD: case MILENA_FN_SUB: case MILENA_FN_MUL: case MILENA_FN_DIV:
        if (node->child_count != 2 || !eval(table,node->children[0],fn,args,argc,depth+1,&a,error,error_size) ||
            !eval(table,node->children[1],fn,args,argc,depth+1,&b,error,error_size)) return false;
        if (node->type == MILENA_FN_ADD) *out = a+b;
        else if (node->type == MILENA_FN_SUB) *out = a-b;
        else if (node->type == MILENA_FN_MUL) *out = a*b;
        else { if (b == 0.0) { set_error(error,error_size,"División por cero"); return false; } *out = a/b; }
        return isfinite(*out);
    case MILENA_FN_CALL: {
        double *values;
        const MilenaFunction *callee = milena_function_table_find(table,node->name);
        if (!callee || node->child_count != callee->parameter_count) { set_error(error,error_size,"Función o aridad inválida"); return false; }
        values = (double *)calloc(node->child_count, sizeof *values); if (!values) { set_error(error,error_size,"Memoria insuficiente"); return false; }
        for (i=0;i<node->child_count;i++) if (!eval(table,node->children[i],fn,args,argc,depth+1,&values[i],error,error_size)) { free(values); return false; }
        { bool ok = eval(table,callee->body,callee,values,node->child_count,depth+1,out,error,error_size); free(values); return ok; }
    }
    default: set_error(error,error_size,"Operación no soportada"); return false;
    }
}
bool milena_function_call(const MilenaFunctionTable *table, const char *name, const double *arguments,
                          size_t argument_count, double *result, char *error, size_t error_size) {
    const MilenaFunction *fn = milena_function_table_find(table,name);
    if (!fn || fn->parameter_count != argument_count || (!arguments && argument_count)) { set_error(error,error_size,"Función desconocida o cantidad de argumentos incorrecta"); return false; }
    return eval(table,fn->body,fn,arguments,argument_count,0,result,error,error_size);
}
