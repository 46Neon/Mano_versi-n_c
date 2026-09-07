#include "user_functions.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static MilenaFunctionNode *number(double n) { MilenaFunctionNode *x=calloc(1,sizeof *x); x->type=MILENA_FN_NUMBER; x->number=n; return x; }
static MilenaFunctionNode *parameter(size_t i) { MilenaFunctionNode *x=calloc(1,sizeof *x); x->type=MILENA_FN_PARAMETER; x->index=i; return x; }
static MilenaFunctionNode *binary(MilenaFunctionNodeType t, MilenaFunctionNode *a, MilenaFunctionNode *b) { MilenaFunctionNode *x=calloc(1,sizeof *x); x->type=t; x->child_count=2; x->children=calloc(2,sizeof *x->children); x->children[0]=a; x->children[1]=b; return x; }
int main(void) { MilenaFunctionTable t; double args[1]={5}, result; char error[128]; MilenaFunction *f=calloc(1,sizeof *f); milena_function_table_init(&t); f->name=malloc(4); if(!f->name)return 1; snprintf(f->name,4,"dup"); f->parameter_count=1; f->parameters=calloc(1,sizeof *f->parameters); f->body=binary(MILENA_FN_ADD,parameter(0),parameter(0)); assert(milena_function_table_add(&t,f)); assert(milena_function_call(&t,"dup",args,1,&result,error,sizeof error)); assert(result==10); assert(!milena_function_call(&t,"dup",args,0,&result,error,sizeof error)); milena_function_table_release(&t); puts("user functions: ok"); return 0; }
