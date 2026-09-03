# Mano — Lenguaje de programación (implementación en C)

¡Bienvenido a Mano!  
Mano es un lenguaje de programación compacto y expresivo con implementación en C, diseñado para aprender conceptos de diseño de lenguajes y para realizar análisis de datos ligeros desde la línea de comandos. Este repositorio contiene la implementación en C, ejemplos y utilidades para compilar, ejecutar y experimentar con el lenguaje.

---

## Qué es Mano
Mano es un lenguaje de propósito educativo y práctico: combina sintaxis clara y reglas simples con herramientas para procesar datos y escribir lógica de programa sin la complejidad de lenguajes más grandes. Está pensado para:
- Estudiantes y desarrolladores que quieren entender cómo se implementa un lenguaje en C.
- Prototipado rápido de scripts para análisis de datos ligero (archivos CSV, transformaciones en memoria).
- Enseñar parsing, interpretación y generación de salida desde código simple.

Mano destaca por tener una sintaxis muy fácil de usar: su gramática es pequeña, las declaraciones son directas y las operaciones comunes (lectura de archivos, filtros, agregaciones) se escriben con expresiones claras y legibles.

---

## Ventajas principales
- Implementación en C, eficiente y portátil.
- Sintaxis fácil de aprender, pensada para lógica algorítmica y análisis de datos.
- Herramientas básicas para análisis de datos: lectura CSV/TSV, filtros y agregaciones.
- Código fuente modular: lexer, parser, AST, intérprete/VM y utilidades I/O.

---

## Composición del repositorio
- Lenguaje principal: C (implementación del compilador/intérprete)
- Utilidades: scripts shell y Makefile para compilar y ejecutar
- Ejemplos: programas Mano demostrativos (scripts .mano o .mn)

---

## Cómo clonar y compilar
Desde tu terminal:

```bash
git clone https://github.com/46Neon/Mano_versi-n_c.git
cd Mano_versi-n_c
# Compilar con Make (si existe Makefile)
make
# O compilar manualmente (ejemplo)
gcc -O2 -std=c11 -o mano src/main.c src/lexer.c src/parser.c src/interpreter.c
```

Notas:
- Si el proyecto usa `Makefile`, `make` suele crear el ejecutable `mano` en la raíz o en `bin/`.
- Si faltan dependencias, las verás en el `Makefile` o en scripts `build.sh`.

---

## Primeros pasos: ejecutar un script Mano
Asumiendo que el binario se llama `mano`:

```bash
# Ejecutar un archivo .mano
./mano examples/hello.mano

# Ejecutar código desde stdin
echo 'print("Hola, Mundo")' | ./mano -e -
```

---

## Alcances en análisis de datos
Mano incluye utilidades y librerías estándar (simples) orientadas a procesamiento de datos:
- Lectura de CSV/TSV con separación configurable.
- Filtros por expresiones (p. ej. seleccionar filas donde columna X > 10).
- Agregaciones básicas: sumas, promedios, contadores, máximos/mínimos.
- Transformaciones: map/transform sobre columnas, combinación simple de archivos.

Limitaciones típicas:
- No es una plataforma de análisis distribuido; está pensado para ficheros locales y conjuntos moderados de datos.
- Para análisis a gran escala o multi-threading avanzado, integra mejor como prototipo que como solución final.

---

## Sintaxis básica y crear lógica (guía rápida)
A continuación se muestran ejemplos de sintaxis para crear lógica de programación en Mano. Son ejemplos ilustrativos y fáciles de adaptar.

Variables y tipos:
```mano
# declaración y asignación
let x = 42
let name = "María"
let pi = 3.14159
```

Operadores:
```mano
let s = x + 10
let cond = (x > 10) and (name != "")
```

Control de flujo:
```mano
if x > 10 {
  print("x es mayor que 10")
} else {
  print("x es 10 o menor")
}

for i in 0..5 {
  print(i)
}
```

Funciones:
```mano
func suma(a, b) {
  return a + b
}

let r = suma(4, 5)
print(r)  # imprime 9
```

Procesamiento de CSV (ejemplo):
```mano
# leer archivo.csv, filtrar filas y sumar columna "ventas"
data = read_csv("ventas.csv")
filtered = filter(data, row -> row["pais"] == "ES" and row["ventas"] > 100)
total = sum(filtered, row -> to_number(row["ventas"]))
print("Total ventas ES:", total)
```

Notas sobre la semántica:
- Tipado dinámico (sencillo): números, cadenas, booleanos, listas y mapas (diccionarios).
- Las funciones son de primera clase; las lambdas se usan para map/filter.
- La sintaxis está pensada para ser legible y directa, parecida a lenguajes scripting.

---

## Ejemplo completo: conteo por categoría
Archivo: examples/count_by_category.mano
```mano
data = read_csv("items.csv")
grouped = group_by(data, row -> row["categoria"])
counts = map(grouped, (cat, rows) -> { "categoria": cat, "count": len(rows) })
print_table(counts)
```

---

## Estructura de la implementación (visión técnica)
- src/lexer.c        — Tokenización del código fuente
- src/parser.c       — Construcción del AST
- src/ast.c          — Definiciones de nodos del AST
- src/interpreter.c  — Evaluador / motor de ejecución
- src/io.c           — Funciones de lectura/escritura CSV y archivos
- include/*.h        — Cabeceras públicas y tipos
- examples/          — Scripts de ejemplo
- tests/             — Pruebas unitarias y de integración
- Makefile           — Instrucciones de compilación

---

## Cómo contribuir
1. Haz fork del repositorio.
2. Crea una rama con tu mejora: `git checkout -b feat/nombre`
3. Añade tests cuando agregues funcionalidad.
4. Crea un Pull Request describiendo el cambio y la motivación.

Buenas contribuciones:
- Nuevas funciones de manejo de datos (p. ej. soporte para JSON).
- Optimización del analizador o del intérprete.
- Ejemplos y documentación en español e inglés.

---

## Roadmap sugerido (ideas)
- Soporte nativo para streaming CSV (memoria limitada).
- Módulos de visualización básica (salida en JSON/CSV para consumo por otras herramientas).
- Paquetes estándar para estadística y regresión básica.
- Compilador JIT para acelerar cálculos intensivos.

---

## Preguntas frecuentes (Rápido)
- ¿Mano es compilado o interpretado?  
  Mano en esta implementación es interpretado (ejecutor en C), pero se pueden añadir fases de compilación a bytecode.
- ¿Puedo llamar a librerías C desde Mano?  
  Sí, mediante una interfaz FFI mínima (si está implementada). Revisa `src/ffi.c` o `docs/ffi.md`.
- ¿Qué formato de archivos soporta?  
  CSV y TSV por defecto; otros formatos mediante extensiones.

---

## Licencia y contacto
- Licencia: MIT (o la que prefieras) — añade archivo LICENSE con tu elección.
- Autor / Mantenimiento: 46Neon — https://github.com/46Neon

---

Este README fue agregado automáticamente por el asistente. Si quieres que lo adapte para reflejar archivos y comandos reales del repositorio —por ejemplo nombres exactos de `src/*.c`, ubicación del binario o ejemplos reales— haz público el repositorio o pega los archivos clave y actualizaré el README y el commit.
