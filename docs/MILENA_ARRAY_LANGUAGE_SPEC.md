# Especificación inicial de arrays en `.milena`

Esta es la primera superficie del lenguaje para exponer `MilenaArray`. La implementación debe construir arrays mediante el runtime C17 existente y no mediante estructuras paralelas del parser.

## Sintaxis inicial

```milena
array valores = [1, 2, 3, 4];
array vacio = zeros(3);

shape(valores);
sum(valores);
valores + 10;
valores + [10, 20, 30, 40];
```

La palabra `array` introduce una declaración local. Los nombres siguen las reglas normales de identificadores. Las expresiones terminan en `;`.

## Literales 1-D

Un literal 1-D contiene solamente números:

```milena
array enteros = [1, 2, 3];
array reales = [1.5, 2.0, 3.25];
```

Reglas:

- una lista vacía es inválida en esta primera versión;
- todos los elementos deben ser numéricos;
- si todos son enteros, el dtype inicial es `int64`;
- si algún elemento tiene parte decimal, el dtype inicial es `float64`;
- la longitud de la lista es su primera dimensión;
- no se acepta una segunda dimensión hasta implementar literales multidimensionales.

## `zeros`

La primera forma aceptada es:

```milena
array ceros = zeros(5);
```

Devuelve un array 1-D de cinco elementos `float64` inicializados en cero. En una extensión posterior se aceptarán dtype y shape explícitos:

```milena
zeros([2, 3], int64);
```

## Operaciones básicas

La primera superficie pública será:

```milena
shape(array)       // devuelve la dimensión 1-D
ndim(array)        // devuelve 1 en esta etapa
size(array)        // cantidad total de elementos
sum(array)         // reducción escalar
array + array      // broadcasting solamente con shape compatible
array + escalar    // broadcasting del escalar
```

El resultado de una operación aritmética es un nuevo `MilenaArray`, salvo que una operación futura declare explícitamente una salida reutilizable.

## Errores obligatorios

El runtime debe producir errores para:

- lista vacía;
- tokens no numéricos dentro de un literal;
- paréntesis o corchetes sin cerrar;
- coma final no soportada en la primera versión;
- shape incompatible;
- overflow durante la conversión o la operación;
- uso de un identificador no definido;
- llamada a función con cantidad incorrecta de argumentos;
- intento de modificar una vista de solo lectura.

## Ownership

Los arrays creados por una declaración pertenecen al entorno de ejecución. Una expresión que crea un resultado entrega ownership al valor resultante. Las vistas futuras conservarán una referencia al almacenamiento propietario.

No se debe guardar un `MilenaArray` dentro del AST. El AST contiene la descripción de la expresión; la VM/interpreter crea y libera el valor en tiempo de ejecución.

## Orden de implementación

1. tokens `[` y `]`;
2. nodo AST para literal 1-D;
3. nodo AST para declaración `array`;
4. tabla de valores del runtime;
5. llamada `zeros(n)`;
6. `shape`, `ndim`, `size` y `sum`;
7. operadores `+`, `-`, `*` y `/` con scalar y arrays compatibles;
8. tests de lexer, parser, runtime y ejecución `.milena`;
9. integración con VM y compiler sin duplicar kernels.
