#include "ast.h"
#include "lexer.h"
#include <assert.h>
#include <string.h>

int main(void) {
    Lexer lexer;
    lexer_init(&lexer, "array valores = [1, 2.5, 3];");
    Token token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFICADOR && strcmp(token.lexeme, "array") == 0);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFICADOR && strcmp(token.lexeme, "valores") == 0);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IGUAL);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_CORCHETE_IZQ);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_NUMERO);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_COMA);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_NUMERO);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_COMA);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_NUMERO);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_CORCHETE_DER);
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_PUNTO_Y_COMA);

    ASTNode *array = ast_create(AST_EXPRESION_ARRAY);
    assert(array);
    ast_add_child(array, ast_create_number(1.0));
    ast_add_child(array, ast_create_number(2.5));
    assert(array->child_count == 2);
    assert(strcmp(ast_type_name(array->type), "EXPRESION_ARRAY") == 0);
    ast_destroy(array);
    return 0;
}
