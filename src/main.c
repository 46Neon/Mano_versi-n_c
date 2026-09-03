#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "compiler.h"
#include "vm.h"
#include "interpreter.h"
#include "dataset.h"
#include "analysis.h"

static char* leer_archivo(const char *nombre) {
    FILE *archivo = fopen(nombre, "rb");
    if (!archivo) {
        fprintf(stderr, "No se pudo abrir: %s\n", nombre);
        return NULL;
    }
    
    fseek(archivo, 0, SEEK_END);
    long tamano = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    
    if (tamano < 0) {
        fclose(archivo);
        return NULL;
    }
    
    char *contenido = (char *)malloc(tamano + 1);
    if (!contenido) {
        fclose(archivo);
        return NULL;
    }
    
    size_t leidos = fread(contenido, 1, tamano, archivo);
    fclose(archivo);
    
    if (leidos != (size_t)tamano) {
        free(contenido);
        return NULL;
    }
    
    contenido[tamano] = '\0';
    return contenido;
}

static int ejecutar_lex(const char *nombre) {
    char *fuente = leer_archivo(nombre);
    if (!fuente) return 1;
    
    Lexer lexer;
    lexer_init(&lexer, fuente);
    
    Token token;
    do {
        token = lexer_next_token(&lexer);
        printf("L%03d:C%03d  %-20s  %s\n",
               token.line, token.column,
               token_type_name(token.type),
               token.lexeme);
    } while (token.type != TOKEN_EOF);
    
    free(fuente);
    return 0;
}

static int ejecutar_parser(const char *nombre) {
    char *fuente = leer_archivo(nombre);
    if (!fuente) return 1;
    
    Lexer lexer;
    lexer_init(&lexer, fuente);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    ASTNode *ast = parser_parse(&parser);
    if (!ast || parser.has_error) {
        printf("Error de parsing\n");
        if (parser.error.code != MANO_OK) {
            mano_error_print(&parser.error);
        }
        free(fuente);
        return 1;
    }
    
    printf("AST generado:\n");
    ast_print(ast, 0);
    
    // Análisis semántico
    SemanticAnalyzer semantic;
    semantic_init(&semantic, ast);
    if (!semantic_analyze(&semantic)) {
        printf("Error semántico\n");
        mano_error_print(&semantic.error);
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    // Compilar a IR
    Compiler compiler;
    if (!compiler_init(&compiler)) {
        printf("Error inicializando compilador\n");
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    if (!compiler_compile(&compiler, ast)) {
        printf("Error compilando\n");
        compiler_destroy(&compiler);
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    IRProgram *ir = compiler_get_ir(&compiler);
    printf("\nIR generado:\n");
    ir_print_program(ir);
    
    // Ejecutar en VM
    VirtualMachine vm;
    if (!vm_init(&vm, ir)) {
        printf("Error inicializando VM\n");
        compiler_destroy(&compiler);
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    printf("\nEjecutando en VM:\n");
    if (!vm_run(&vm)) {
        printf("Error en VM\n");
        mano_error_print(&vm.error);
        vm_destroy(&vm);
        compiler_destroy(&compiler);
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    vm_destroy(&vm);
    compiler_destroy(&compiler);
    ast_destroy(ast);
    free(fuente);
    
    return 0;
}

static int ejecutar_interprete(const char *nombre) {
    char *fuente = leer_archivo(nombre);
    if (!fuente) return 1;
    
    Lexer lexer;
    lexer_init(&lexer, fuente);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    ASTNode *ast = parser_parse(&parser);
    if (!ast || parser.has_error) {
        printf("Error de parsing\n");
        free(fuente);
        return 1;
    }
    
    Interpreter interpreter;
    if (!interpreter_init(&interpreter, ast)) {
        printf("Error inicializando intérprete\n");
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    printf("Ejecutando intérprete:\n");
    if (!interpreter_run(&interpreter)) {
        printf("Error en intérprete\n");
        mano_error_print(&interpreter.error);
        interpreter_destroy(&interpreter);
        ast_destroy(ast);
        free(fuente);
        return 1;
    }
    
    interpreter_destroy(&interpreter);
    ast_destroy(ast);
    free(fuente);
    
    return 0;
}

static void imprimir_uso(const char *programa) {
    printf("MANO v%s - Lenguaje de Análisis de Datos\n", MANO_VERSION);
    printf("Uso:\n");
    printf("  %s lex <archivo.mano>        - Tokenizar\n", programa);
    printf("  %s parse <archivo.mano>      - Parsear y mostrar AST\n", programa);
    printf("  %s compile <archivo.mano>    - Compilar a IR\n", programa);
    printf("  %s run <archivo.mano>        - Ejecutar con VM\n", programa);
    printf("  %s interpret <archivo.mano>  - Ejecutar con intérprete\n", programa);
    printf("  %s analizar <csv> <json>     - Análisis directo\n", programa);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        imprimir_uso(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "lex") == 0 && argc == 3) {
        return ejecutar_lex(argv[2]);
    }
    
    if (strcmp(argv[1], "parse") == 0 && argc == 3) {
        return ejecutar_parser(argv[2]);
    }
    
    if (strcmp(argv[1], "compile") == 0 && argc == 3) {
        return ejecutar_parser(argv[2]);
    }
    
    if (strcmp(argv[1], "run") == 0 && argc == 3) {
        return ejecutar_parser(argv[2]);
    }
    
    if (strcmp(argv[1], "interpret") == 0 && argc == 3) {
        return ejecutar_interprete(argv[2]);
    }
    
    if (strcmp(argv[1], "analizar") == 0 && argc == 4) {
        Dataset dataset;
        if (!dataset_cargar_csv(&dataset, argv[2])) {
            fprintf(stderr, "Error cargando CSV: %s\n", argv[2]);
            return 1;
        }
        
        printf("CSV cargado: %s\n", argv[2]);
        dataset_imprimir(&dataset, 5);
        
        bool result = analysis_ventas(&dataset, "fecha", "precio", "cantidad", argv[3]);
        dataset_destruir(&dataset);
        return result ? 0 : 1;
    }
    
    imprimir_uso(argv[0]);
    return 1;
}
