#ifndef MANO_LEXER_H
#define MANO_LEXER_H

#include "common.h"
#include "token.h"

typedef struct Lexer {
    const char *source;
    size_t position;
    size_t length;
    int line;
    int column;
    Token current_token;
    Token previous_token;
    ManoErrorInfo error;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
Token lexer_peek_token(Lexer *lexer);
void lexer_advance_token(Lexer *lexer);
bool lexer_match(Lexer *lexer, TokenType type);
bool lexer_expect(Lexer *lexer, TokenType type, const char *error_msg);

#endif
