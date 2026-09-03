#!/bin/bash

echo "=== MANO - Lenguaje de Análisis de Datos ==="
echo "Compilando..."

# Detectar compilador
if command -v clang >/dev/null 2>&1; then
    CC=clang
elif command -v gcc >/dev/null 2>&1; then
    CC=gcc
else
    echo "Error: No se encontró compilador (clang o gcc)"
    echo "Instala con: pkg install clang"
    exit 1
fi

echo "Usando compilador: $CC"

# Compilar
$CC -Wall -Wextra -std=c11 -pedantic -Iinclude \
    src/main.c \
    src/common.c \
    src/arena.c \
    src/lexer.c \
    src/parser.c \
    src/ast.c \
    src/symbol.c \
    src/semantic.c \
    src/ir.c \
    src/compiler.c \
    src/vm.c \
    src/gc.c \
    src/module.c \
    src/interpreter.c \
    src/assembler.c \
    src/instructions.c \
    src/dataset.c \
    src/analysis.c \
    -o mano

if [ $? -eq 0 ]; then
    echo "✓ Compilación exitosa"
    echo ""
    echo "=== Probando MANO ==="
    echo ""
    echo "1. Tokenización:"
    ./mano lex examples/ventas.mano
    echo ""
    echo "2. Parseo (AST):"
    ./mano parse examples/ventas.mano
    echo ""
    echo "3. Interpretación:"
    ./mano interpret examples/ventas.mano
    echo ""
    echo "=== MANO listo para usar ==="
    echo "Comandos disponibles:"
    echo "  ./mano lex <archivo.mano>      - Tokenizar"
    echo "  ./mano parse <archivo.mano>    - Parsear y mostrar AST"
    echo "  ./mano interpret <archivo.mano> - Ejecutar con intérprete"
    echo "  ./mano run <archivo.mano>      - Ejecutar con VM"
    echo "  ./mano analizar <csv> <json>   - Análisis directo de CSV"
else
    echo "✗ Error en compilación"
    exit 1
fi
