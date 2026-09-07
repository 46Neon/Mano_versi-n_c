#include "interpreter.h"
#include <math.h>

typedef struct { char *name; double value; } Binding;
typedef struct Runtime { Binding *items; size_t count, capacity; struct Runtime *parent; unsigned depth; } Runtime;

static bool lookup(Runtime *r, const char *name, double *out) {
    for (; r; r = r->parent) for (size_t n = r->count; n > 0; --n)
        if (strcmp(r->items[n-1].name, name) == 0) { *out = r->items[n-1].value; return true; }
    return false;
}
static bool assign(Runtime *r, const char *name, double value) {
    for (; r; r = r->parent) for (size_t n = r->count; n > 0; --n)
        if (strcmp(r->items[n-1].name, name) == 0) { r->items[n-1].value = value; return true; }
    return false;
}
static bool bind(Runtime *r, const char *name, double value) {
    Binding *b;
    if (!name) return false;
    b = (Binding *)realloc(r->items, (r->count + 1) * sizeof *b);
    if (!b) return false;
    r->items = b; r->items[r->count].name = milena_strdup(name);
    if (!r->items[r->count].name) return false;
    r->items[r->count++].value = value; return true;
}
static bool eval_expr(Interpreter *, ASTNode *, Runtime *, double *);
static bool exec_stmt(Interpreter *, ASTNode *, Runtime *, bool *, double *);

static bool invoke(Interpreter *in, ASTNode *fn, ASTNode *call, Runtime *parent, double *out) {
    Runtime local = {0}; bool returned = false; double value = 0;
    if (!fn || fn->child_count != 2 || !call || fn->children[0]->child_count != call->child_count) return false;
    if (parent->depth >= 1000) return false;
    local.parent = parent; local.depth = parent->depth + 1;
    for (size_t k=0; k<call->child_count; ++k) {
        if (!eval_expr(in, call->children[k], parent, &value) ||
            !bind(&local, fn->children[0]->children[k]->value, value)) goto fail;
    }
    for (size_t k=0; k<fn->children[1]->child_count && !returned; ++k)
        if (!exec_stmt(in, fn->children[1]->children[k], &local, &returned, &value)) goto fail;
    if (!returned) value = 0;
    *out = value;
    for (size_t k=0;k<local.count;k++) free(local.items[k].name); free(local.items); return true;
fail:
    for (size_t k=0;k<local.count;k++) free(local.items[k].name); free(local.items); return false;
}
static bool eval_expr(Interpreter *in, ASTNode *n, Runtime *r, double *out) {
    double a,b;
    if (!n || !out) return false;
    switch (n->type) {
    case AST_EXPRESION_LITERAL: *out=n->number_value; return true;
    case AST_EXPRESION_IDENTIFICADOR: return lookup(r,n->value,out);
    case AST_EXPRESION_LLAMADA: {
        Symbol *s=symbol_table_lookup(in->symbols,n->value);
        return s && s->declaration && invoke(in,s->declaration,n,r,out);
    }
    case AST_EXPRESION_OPERACION:
        if (n->child_count != 2 || !eval_expr(in,n->children[0],r,&a) || !eval_expr(in,n->children[1],r,&b)) return false;
        if (!strcmp(n->value,"+"))*out=a+b; else if(!strcmp(n->value,"-"))*out=a-b;
        else if(!strcmp(n->value,"*"))*out=a*b; else if(!strcmp(n->value,"/")){if(b==0)return false;*out=a/b;}
        else if(!strcmp(n->value,"=="))*out=(a==b); else if(!strcmp(n->value,"!="))*out=(a!=b);
        else if(!strcmp(n->value,">"))*out=(a>b); else if(!strcmp(n->value,">="))*out=(a>=b);
        else if(!strcmp(n->value,"<"))*out=(a<b); else if(!strcmp(n->value,"<="))*out=(a<=b); else return false;
        return isfinite(*out);
    default: return false;
    }
}
static bool exec_stmt(Interpreter *in, ASTNode *n, Runtime *r, bool *returned, double *ret) {
    double v;
    if (!n) return true;
    if (n->type==AST_COMANDO_RETORNAR) { if(n->child_count!=1||!eval_expr(in,n->children[0],r,&v))return false;*ret=v;*returned=true;return true; }
    if (n->type==AST_DECLARACION_VARIABLE) { if(!eval_expr(in,n->children[0],r,&v)||!bind(r,n->value,v))return false; return true; }
    if (n->type==AST_ASIGNACION_VARIABLE) { if(!eval_expr(in,n->children[0],r,&v)||!assign(r,n->value,v))return false; return true; }
    if (n->type==AST_CONDICION_SI) {
        if(n->child_count<1||!eval_expr(in,n->children[0],r,&v))return false;
        size_t first=v?1:n->child_count; size_t last=first;
        if(!v && n->child_count>1 && n->children[n->child_count-1]->type==AST_BLOQUE_FUNCION){first=n->child_count-1;last=n->child_count;}
        else if(v && last>first && n->children[last-1]->type==AST_BLOQUE_FUNCION)--last;
        for(size_t k=first;k<last && !*returned;k++) if(!exec_stmt(in,n->children[k],r,returned,ret))return false;
        return true;
    }
    if (n->type==AST_BLOQUE_FUNCION) { for(size_t k=0;k<n->child_count&&!*returned;k++)if(!exec_stmt(in,n->children[k],r,returned,ret))return false; return true; }
    return true;
}
static bool register_functions(Interpreter *in, ASTNode *p) {
    for(size_t k=0;k<p->child_count;k++) if(p->children[k]->type==AST_DECLARACION_FUNCION){ASTNode*f=p->children[k];Symbol*s=symbol_table_lookup_local(in->symbols,f->value);if(!s){s=symbol_create(f->value,SYMBOL_FUNCTION);if(!s||!symbol_table_insert(in->symbols,s))return false;}s->declaration=f;} return true;
}
bool interpreter_init(Interpreter *in, ASTNode *ast){if(!in)return false;memset(in,0,sizeof *in);in->ast=ast;in->symbols=symbol_table_create();if(!in->symbols)return false;in->runtime=calloc(1,sizeof(Runtime));if(!in->runtime){symbol_table_destroy(in->symbols);in->symbols=NULL;return false;}milena_error_init(&in->error);return true;}
bool interpreter_run(Interpreter *in){Runtime*r;bool returned=false;double ret=0;if(!in||!in->ast)return false;r=(Runtime*)in->runtime;if(in->ast->type==AST_PROGRAMA&&!register_functions(in,in->ast))return false;for(size_t k=0;k<in->ast->child_count;k++){ASTNode*n=in->ast->children[k];if(n->type==AST_DECLARACION_FUNCION)continue;if(!exec_stmt(in,n,r,&returned,&ret))return false;}return !returned||true;}
bool interpreter_get_number(const Interpreter *in,const char *name,double *value){return in&&name&&value&&in->runtime&&lookup((Runtime*)in->runtime,name,value);}
void interpreter_destroy(Interpreter *in){if(!in)return;if(in->dataset){dataset_destruir(in->dataset);free(in->dataset);}if(in->symbols)symbol_table_destroy(in->symbols);Runtime*r=(Runtime*)in->runtime;if(r){for(size_t k=0;k<r->count;k++)free(r->items[k].name);free(r->items);free(r);}in->runtime=NULL;}
