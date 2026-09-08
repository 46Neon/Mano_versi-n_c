#include "parser.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    const char *source = "mediana(valores); percentil(valores, 90);";
    Lexer lexer;
    Parser parser;
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    ASTNode *program = parser_parse(&parser);
    assert(program);
    if (parser.has_error) {
        fprintf(stderr, "parser diagnostic: code=%d line=%zu column=%zu message=%s\n",
                parser.error.code, parser.error.line, parser.error.column, parser.error.message);
    }
    assert(!parser.has_error);
    assert(program->child_count == 2);
    assert(program->children[0]->type == AST_OPERACION_ESTADISTICA);
    assert(program->children[0]->statistical_operation == AST_ESTADISTICA_MEDIANA);
    assert(program->children[1]->type == AST_OPERACION_ESTADISTICA);
    assert(program->children[1]->statistical_operation == AST_ESTADISTICA_PERCENTIL);
    assert(program->children[0]->children[0]->type == AST_EXPRESION_IDENTIFICADOR);
    assert(strcmp(program->children[0]->children[0]->value, "valores") == 0);
    assert(program->children[1]->percentile == 90.0);
    ast_destroy(program);
    return 0;
}
