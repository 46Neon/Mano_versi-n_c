#include "interpreter.h"
#include <math.h>

typedef struct { char *name; double value; } Binding;
typedef struct Runtime { Binding *items; size_t count; struct Runtime *parent; unsigned depth; } Runtime;
static bool rt_value(Runtime*r,const char*n,bool*ok){for(;r;r=r->parent)for(size_t i=r->count;i>0;i--)if(strcmp(r->items[i-1].name,n)==0){*ok=true;return r->items[i-1].value;}*ok=false;return 0;}
static bool eval_expr(Interpreter*i,ASTNode*n,Runtime*r,double*out);
static bool invoke(Interpreter*i,ASTNode*f,ASTNode*call,Runtime*parent,double*out){
    if(!f||f->child_count<2||f->children[0]->child_count!=call->child_count)return false;
    if(parent->depth>=1000)return false;
    Runtime child={0};child.parent=parent;child.depth=parent->depth+1;size_t pc=f->children[0]->child_count;
    child.items=calloc(pc?pc:1,sizeof(Binding));if(!child.items&&pc)return false;child.count=pc;
    for(size_t k=0;k<pc;k++){double v;if(!eval_expr(i,call->children[k],parent,&v))goto fail;child.items[k].name=milena_strdup(f->children[0]->children[k]->value);if(!child.items[k].name)goto fail;child.items[k].value=v;}
    for(size_t k=0;k<f->children[1]->child_count;k++){ASTNode*x=f->children[1]->children[k];
        if(x->type==AST_COMANDO_RETORNAR){if(!eval_expr(i,x->children[0],&child,out))goto fail;goto done;}
        if(x->type==AST_CONDICION_SI){
            double c;if(!eval_expr(i,x->children[0],&child,&c))goto fail;
            ASTNode *branch=NULL; size_t begin=0, branch_count=0;
            if(c){
                branch=x;
                begin=1;
                /* The final child is the optional else block, not part of
                 * the true branch.  Keeping it out is important when the
                 * true branch does not return (for example, local setup). */
                branch_count=x->child_count;
                if(branch_count>1 &&
                   x->children[branch_count-1]->type==AST_BLOQUE_FUNCION)
                    --branch_count;
            } else if(x->child_count>1&&x->children[x->child_count-1]->type==AST_BLOQUE_FUNCION){
                branch=x->children[x->child_count-1];
                branch_count=branch->child_count;
            }
            if(branch) for(size_t j=begin;j<branch_count;j++){ASTNode*y=branch->children[j];
                if(y->type==AST_COMANDO_RETORNAR){if(!eval_expr(i,y->children[0],&child,out))goto fail;goto done;}
                if(y->type==AST_DECLARACION_VARIABLE||y->type==AST_ASIGNACION_VARIABLE){double v;if(!eval_expr(i,y->children[0],&child,&v))goto fail;Binding*z=realloc(child.items,(child.count+1)*sizeof(*z));if(!z)goto fail;child.items=z;child.items[child.count].name=milena_strdup(y->value);if(!child.items[child.count].name)goto fail;child.items[child.count++].value=v;}
            }
        } else if(x->type==AST_DECLARACION_VARIABLE||x->type==AST_ASIGNACION_VARIABLE){double v;if(!eval_expr(i,x->children[0],&child,&v))goto fail;Binding*z=realloc(child.items,(child.count+1)*sizeof(*z));if(!z)goto fail;child.items=z;child.items[child.count].name=milena_strdup(x->value);if(!child.items[child.count].name)goto fail;child.items[child.count++].value=v;}
    }
    *out=0;
done: for(size_t k=0;k<child.count;k++)free(child.items[k].name);free(child.items);return true;
fail: for(size_t k=0;k<child.count;k++)free(child.items[k].name);free(child.items);return false;
}
static bool eval_expr(Interpreter*i,ASTNode*n,Runtime*r,double*out){if(!n||!out)return false;
    if(n->type==AST_EXPRESION_LITERAL){*out=n->number_value;return true;} if(n->type==AST_EXPRESION_IDENTIFICADOR){bool ok;*out=rt_value(r,n->value,&ok);return ok;}
    if(n->type==AST_EXPRESION_OPERACION){double a,b;if(!eval_expr(i,n->children[0],r,&a)||!eval_expr(i,n->children[1],r,&b))return false;const char*o=n->value;if(strcmp(o,"+")==0)*out=a+b;else if(strcmp(o,"-")==0)*out=a-b;else if(strcmp(o,"*")==0)*out=a*b;else if(strcmp(o,"/")==0){if(b==0)return false;*out=a/b;}else if(strcmp(o,"==")==0)*out=a==b;else if(strcmp(o,"!=")==0)*out=a!=b;else if(strcmp(o,">")==0)*out=a>b;else if(strcmp(o,">=")==0)*out=a>=b;else if(strcmp(o,"<")==0)*out=a<b;else if(strcmp(o,"<=")==0)*out=a<=b;else return false;return true;}
    if(n->type==AST_EXPRESION_LLAMADA){Symbol*s=symbol_table_lookup(i->symbols,n->value);return s&&s->declaration&&invoke(i,s->declaration,n,r,out);} return false;
}

bool interpreter_init(Interpreter *interpreter, ASTNode *ast) {
    if (!interpreter) return false;
    
    interpreter->ast = ast;
    interpreter->symbols = symbol_table_create();
    if (!interpreter->symbols) return false;
    
    interpreter->dataset = NULL;
    interpreter->has_error = false;
    interpreter->runtime = calloc(1, sizeof(Runtime));
    milena_error_init(&interpreter->error);
    
    return true;
}

static bool interpreter_execute_node(Interpreter *interpreter, ASTNode *node);

static bool interpreter_execute_program(Interpreter *interpreter, ASTNode *node) {
    if (!node) return true;
    for(size_t i=0;i<node->child_count;i++) if(node->children[i]->type==AST_DECLARACION_FUNCION) { ASTNode*f=node->children[i]; if(!symbol_table_lookup_local(interpreter->symbols,f->value)){Symbol*s=symbol_create(f->value,SYMBOL_FUNCTION);if(!s||!symbol_table_insert(interpreter->symbols,s))return false;s->declaration=f;} }
    for (size_t i = 0; i < node->child_count; i++) {
        if (!interpreter_execute_node(interpreter, node->children[i])) {
            return false;
        }
    }
    
    return true;
}

static bool interpreter_execute_bloque(Interpreter *interpreter, ASTNode *node) {
    if (!node) return true;
    
    for (size_t i = 0; i < node->child_count; i++) {
        if (!interpreter_execute_node(interpreter, node->children[i])) {
            return false;
        }
    }
    
    return true;
}

static bool interpreter_execute_cargar(Interpreter *interpreter, ASTNode *node) {
    if (!node || !node->value) {
        interpreter->has_error = true;
        milena_error_set(&interpreter->error, MILENA_ERROR_RUNTIME,
                      0, 0, 0, "Cargar requiere nombre de archivo");
        return false;
    }
    
    if (interpreter->dataset) {
        dataset_destruir(interpreter->dataset);
        free(interpreter->dataset);
    }
    
    interpreter->dataset = (Dataset *)calloc(1, sizeof(Dataset));
    if (!interpreter->dataset) {
        interpreter->has_error = true;
        milena_error_set(&interpreter->error, MILENA_ERROR_MEMORY,
                      0, 0, 0, "No se pudo crear dataset");
        return false;
    }
    
    if (!dataset_cargar_csv(interpreter->dataset, node->value)) {
        interpreter->has_error = true;
        milena_error_set(&interpreter->error, MILENA_ERROR_IO,
                      0, 0, 0, "No se pudo cargar CSV");
        return false;
    }
    
    printf("Dataset cargado: %s (%zu filas, %zu columnas)\n",
           node->value, interpreter->dataset->row_count, 
           interpreter->dataset->column_count);
    
    return true;
}

static bool interpreter_execute_exportar(Interpreter *interpreter, ASTNode *node) {
    if (!interpreter->dataset || !node->value) {
        interpreter->has_error = true;
        milena_error_set(&interpreter->error, MILENA_ERROR_RUNTIME,
                      0, 0, 0, "No hay dataset o nombre de archivo para exportar");
        return false;
    }
    
    if (!dataset_guardar_json(interpreter->dataset, node->value)) {
        interpreter->has_error = true;
        milena_error_set(&interpreter->error, MILENA_ERROR_IO,
                      0, 0, 0, "No se pudo exportar JSON");
        return false;
    }
    
    printf("Datos exportados a: %s\n", node->value);
    return true;
}

static bool interpreter_execute_node(Interpreter *interpreter, ASTNode *node) {
    if (!node) return true;
    Runtime *r=(Runtime*)interpreter->runtime;
    if(node->type==AST_DECLARACION_FUNCION){Symbol*s=symbol_table_lookup_local(interpreter->symbols,node->value);if(!s){s=symbol_create(node->value,SYMBOL_FUNCTION);if(!s||!symbol_table_insert(interpreter->symbols,s)){free(s);return false;}}s->declaration=node;return true;}
    if(node->type==AST_DECLARACION_VARIABLE||node->type==AST_ASIGNACION_VARIABLE){double v;if(!eval_expr(interpreter,node->children[0],r,&v))return false;Symbol*s=symbol_table_lookup(interpreter->symbols,node->value);if(!s){s=symbol_create(node->value,SYMBOL_VARIABLE);if(!s||!symbol_table_insert(interpreter->symbols,s)){free(s);return false;}}if(r->count==0||strcmp(r->items[r->count-1].name,node->value)!=0){r->items=realloc(r->items,(r->count+1)*sizeof(Binding));r->items[r->count].name=milena_strdup(node->value);r->count++;}r->items[r->count-1].value=v;return true;}
    switch (node->type) {
        case AST_PROGRAMA:
            return interpreter_execute_program(interpreter, node);
            
        case AST_BLOQUE_ANALISIS:
        case AST_BLOQUE_LIMPIAR:
        case AST_BLOQUE_TRANSFORMAR:
        case AST_BLOQUE_FILTRAR:
        case AST_BLOQUE_AGRUPAR:
        case AST_BLOQUE_RESUMIR:
        case AST_BLOQUE_VISUALIZAR:
            return interpreter_execute_bloque(interpreter, node);
            
        case AST_LLAMADA_CARGAR:
            return interpreter_execute_cargar(interpreter, node);
            
        case AST_BLOQUE_EXPORTAR:
            return interpreter_execute_exportar(interpreter, node);
            
        default:
            for (size_t i = 0; i < node->child_count; i++) {
                if (!interpreter_execute_node(interpreter, node->children[i])) {
                    return false;
                }
            }
            return true;
    }
}

bool interpreter_run(Interpreter *interpreter) {
    if (!interpreter || !interpreter->ast) return false;
    
    return interpreter_execute_node(interpreter, interpreter->ast);
}

bool interpreter_get_number(const Interpreter *interpreter, const char *name, double *value) {
    if (!interpreter || !name || !value || !interpreter->runtime) return false;
    bool ok = false; *value = rt_value((Runtime *)interpreter->runtime, name, &ok); return ok;
}

void interpreter_destroy(Interpreter *interpreter) {
    if (!interpreter) return;
    
    if (interpreter->dataset) {
        dataset_destruir(interpreter->dataset);
        free(interpreter->dataset);
    }
    
    if (interpreter->symbols) {
        symbol_table_destroy(interpreter->symbols);
    }
    Runtime *r=(Runtime*)interpreter->runtime;
    if(r){for(size_t i=0;i<r->count;i++)free(r->items[i].name);free(r->items);free(r);}
    interpreter->runtime=NULL;
}

