#include "compiler.h"

bool compiler_init(Compiler *compiler) {
    if (!compiler) return false;
    
    compiler->ast = NULL;
    compiler->ir = ir_program_create();
    if (!compiler->ir) return false;
    
    compiler->symbols = symbol_table_create();
    if (!compiler->symbols) {
        ir_program_destroy(compiler->ir);
        return false;
    }
    
    compiler->has_error = false;
    milena_error_init(&compiler->error);
    
    return true;
}

bool compiler_compile(Compiler *compiler, ASTNode *ast) {
    if (!compiler || !ast) return false;
    
    compiler->ast = ast;
    
    // Generar IR desde el AST
    if (!ir_generate(compiler->ir, ast)) {
        compiler->has_error = true;
        milena_error_set(&compiler->error, MILENA_ERROR_COMPILER, 
                      "Error generando IR", 0, 0);
        return false;
    }
    
    return true;
}

IRProgram* compiler_get_ir(Compiler *compiler) {
    return compiler ? compiler->ir : NULL;
}

void compiler_destroy(Compiler *compiler) {
    if (!compiler) return;
    
    if (compiler->ir) {
        ir_program_destroy(compiler->ir);
    }
    
    if (compiler->symbols) {
        symbol_table_destroy(compiler->symbols);
    }
}
