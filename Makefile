CC = clang
CFLAGS = -Wall -Wextra -std=c11 -pedantic -Iinclude -g
SOURCES = src/main.c src/common.c src/arena.c src/lexer.c \
          src/parser.c src/ast.c src/symbol.c src/semantic.c \
          src/ir.c src/compiler.c src/vm.c src/gc.c \
          src/module.c src/interpreter.c src/assembler.c \
          src/instructions.c src/dataset.c src/analysis.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = mano

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) lex examples/ventas.mano

clean:
	rm -f $(OBJECTS) $(TARGET) reporte_ventas.json
