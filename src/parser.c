#include "parser.h"

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
    parser->previous = parser->current;
    parser->has_error = false;
    mano_error_init(&parser->error);
}

void parser_error(Parser *parser, const char *msg) {
    mano_error_set(&parser->error, MANO_ERROR_SYNTAX, msg,
                  parser->current.line, parser->current.column);
    parser->has_error = true;
}

void parser_advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
}

bool parser_match(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

bool parser_expect(Parser *parser, TokenType type, const char *msg) {
    if (!parser_match(parser, type)) {
        parser_error(parser, msg);
        return false;
    }
    parser_advance(parser);
    return true;
}

static ASTNode* parse_bloque_analisis(Parser *parser) {
    if (!parser_expect(parser, TOKEN_PUNTO, "Se esperaba '.'")) return NULL;
    if (!parser_expect(parser, TOKEN_KW_ANALISIS, "Se esperaba 'analisis'")) return NULL;
    if (!parser_expect(parser, TOKEN_IDENTIFICADOR, "Se esperaba nombre")) return NULL;
    
    ASTNode *node = ast_create_leaf(AST_BLOQUE_ANALISIS, parser->previous.lexeme);
    if (!node) return NULL;
    
    if (!parser_expect(parser, TOKEN_LLAVE_IZQ, "Se esperaba '{'")) {
        ast_destroy(node);
        return NULL;
    }
    
    // Parsear contenido del bloque
    while (!parser_match(parser, TOKEN_LLAVE_DER) && !parser_match(parser, TOKEN_EOF)) {
        if (parser_match(parser, TOKEN_NUMERAL)) {
            parser_advance(parser);
            if (parser_match(parser, TOKEN_KW_DATOS)) {
                parser_advance(parser);
                ast_add_child(node, ast_create(AST_DECLARACION_DATOS));
            } else if (parser_match(parser, TOKEN_KW_ESTADISTICA)) {
                parser_advance(parser);
                ast_add_child(node, ast_create(AST_DECLARACION_ESTADISTICA));
            }
        } else if (parser_match(parser, TOKEN_KW_DATASET)) {
            parser_advance(parser);
            if (parser_match(parser, TOKEN_KW_CARGAR)) {
                parser_advance(parser);
                if (parser_match(parser, TOKEN_KW_DATOS)) {
                    parser_advance(parser);
                }
                if (parser_expect(parser, TOKEN_PAR_IZQ, "Se esperaba '('")) {
                    if (parser_expect(parser, TOKEN_CADENA, "Se esperaba archivo")) {
                        ASTNode *cargar = ast_create_leaf(AST_LLAMADA_CARGAR, parser->previous.lexeme);
                        if (cargar) ast_add_child(node, cargar);
                        parser_expect(parser, TOKEN_PAR_DER, "Se esperaba ')'");
                    }
                }
            }
        } else if (parser_match(parser, TOKEN_PUNTO)) {
            parser_advance(parser);
            if (parser_match(parser, TOKEN_KW_LIMPIAR)) {
                parser_advance(parser);
                if (parser_match(parser, TOKEN_KW_DATASET)) parser_advance(parser);
                if (parser_expect(parser, TOKEN_LLAVE_IZQ, "Se esperaba '{'")) {
                    ASTNode *limpiar = ast_create(AST_BLOQUE_LIMPIAR);
                    while (!parser_match(parser, TOKEN_LLAVE_DER) && !parser_match(parser, TOKEN_EOF)) {
                        if (parser_match(parser, TOKEN_NUMERAL)) {
                            parser_advance(parser);
                            if (parser_match(parser, TOKEN_KW_NULOS)) {
                                parser_advance(parser);
                                if (parser_expect(parser, TOKEN_PAR_IZQ, "Se esperaba '('")) {
                                    if (parser_expect(parser, TOKEN_CADENA, "Se esperaba cadena")) {
                                        ast_add_child(limpiar, ast_create_leaf(AST_COMANDO_NULOS, parser->previous.lexeme));
                                        parser_expect(parser, TOKEN_PAR_DER, "Se esperaba ')'");
                                    }
                                }
                            } else if (parser_match(parser, TOKEN_KW_DUPLICADOS)) {
                                parser_advance(parser);
                                if (parser_expect(parser, TOKEN_PAR_IZQ, "Se esperaba '('")) {
                                    if (parser_expect(parser, TOKEN_CADENA, "Se esperaba cadena")) {
                                        ast_add_child(limpiar, ast_create_leaf(AST_COMANDO_DUPLICADOS, parser->previous.lexeme));
                                        parser_expect(parser, TOKEN_PAR_DER, "Se esperaba ')'");
                                    }
                                }
                            }
                        }
                    }
                    parser_expect(parser, TOKEN_LLAVE_DER, "Se esperaba '}'");
                    ast_add_child(node, limpiar);
                }
            } else if (parser_match(parser, TOKEN_KW_TRANSFORMAR)) {
                parser_advance(parser);
                if (parser_match(parser, TOKEN_KW_DATASET)) parser_advance(parser);
                if (parser_expect(parser, TOKEN_LLAVE_IZQ, "Se esperaba '{'")) {
                    ASTNode *transformar = ast_create(AST_BLOQUE_TRANSFORMAR);
                    while (!parser_match(parser, TOKEN_LLAVE_DER) && !parser_match(parser, TOKEN_EOF)) {
                        if (parser_match(parser, TOKEN_NUMERAL)) {
                            parser_advance(parser);
                            if (parser_match(parser, TOKEN_KW_TOTAL)) {
                                parser_advance(parser);
                                if (parser_expect(parser, TOKEN_PAR_IZQ, "Se esperaba '('")) {
                                    if (parser_expect(parser, TOKEN_CADENA, "Se esperaba cadena")) {
                                        ast_add_child(transformar, ast_create_leaf(AST_COMANDO_TOTAL, parser->previous.lexeme));
                                        parser_expect(parser, TOKEN_PAR_DER, "Se esperaba ')'");
                                    }
                                }
                            } else if (parser_match(parser, TOKEN_KW_PERIODO)) {
                                parser_advance(parser);
                                if (parser_match(parser, TOKEN_KW_EXTRAER)) {
                                    parser_advance(parser);
                                    if (parser_expect(parser, TOKEN_PAR_IZQ, "Se esperaba '('")) {
                                        if (parser_expect(parser, TOKEN_CADENA, "Se esperaba cadena")) {
                                            ast_add_child(transformar, ast_create_leaf(AST_COMANDO_PERIODO, parser->previous.lexeme));
                                            parser_expect(parser, TOKEN_PAR_DER, "Se esperaba ')'");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    parser_expect(parser, TOKEN_LLAVE_DER, "Se esperaba '}'");
                    ast_add_child(node, transformar);
                }
            } else if (parser_match(parser, TOKEN_KW_EXPORTAR)) {
                parser_advance(parser);
                if (parser_expect(parser, TOKEN_LLAVE_IZQ, "Se esperaba '{'")) {
                    // Saltar contenido
                    while (!parser_match(parser, TOKEN_LLAVE_DER) && !parser_match(parser, TOKEN_EOF)) {
                        parser_advance(parser);
                    }
                    parser_expect(parser, TOKEN_LLAVE_DER, "Se esperaba '}'");
                }
            } else {
                // Otros bloques
                if (parser_match(parser, TOKEN_IDENTIFICADOR)) {
                    parser_advance(parser);
                    if (parser_match(parser, TOKEN_LLAVE_IZQ)) {
                        parser_advance(parser);
                        while (!parser_match(parser, TOKEN_LLAVE_DER) && !parser_match(parser, TOKEN_EOF)) {
                            parser_advance(parser);
                        }
                        parser_expect(parser, TOKEN_LLAVE_DER, "Se esperaba '}'");
                    }
                }
            }
        } else {
            parser_advance(parser);
        }
    }
    
    parser_expect(parser, TOKEN_LLAVE_DER, "Se esperaba '}'");
    return node;
}

ASTNode* parser_parse(Parser *parser) {
    ASTNode *program = ast_create(AST_PROGRAMA);
    if (!program) {
        parser_error(parser, "Error de memoria");
        return NULL;
    }
    
    ASTNode *analisis = parse_bloque_analisis(parser);
    if (analisis) {
        ast_add_child(program, analisis);
    }
    
    if (!parser_match(parser, TOKEN_EOF)) {
        parser_error(parser, "Se esperaba fin de archivo");
    }
    
    return program;
}
