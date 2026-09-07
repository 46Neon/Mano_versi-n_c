# Milena

Milena es un lenguaje y motor nativo para análisis de datos, estadística y computación científica. Está diseñado para trabajar con arreglos, tablas y operaciones reproducibles mediante scripts con extensión `.milena`.

Su objetivo es convertirse en una herramienta clave para el análisis de datos del futuro: clara para las personas, controlable para equipos técnicos y portable entre dispositivos móviles y sistemas de escritorio. Ese objetivo es progresivo; el proyecto se desarrolla por etapas y cada capacidad se valida antes de presentarse como terminada.

## Qué puede hacer hoy

- Crear arreglos numéricos y arreglos de ceros.
- Consultar forma, dimensiones y tamaño.
- Ejecutar operaciones entre arreglos y escalares.
- Aplicar broadcasting en operaciones compatibles.
- Calcular suma, media, mínimo, máximo, varianza y desviación estándar.
- Calcular mediana y percentiles con interpolación lineal.
- Reducir operaciones por eje y conservar dimensiones.
- Trabajar con vistas, strides, reshape y transposición desde el motor de arreglos.
- Analizar archivos tabulares y generar reportes reproducibles.
- Ejecutar módulos de análisis preventivo y estadístico con advertencias explícitas.

La visualización, los sistemas distribuidos y los modelos avanzados todavía forman parte de etapas posteriores.

## Primer ejemplo

```milena
arreglo valores = [1, 2, 3, 4];

forma(valores);
tamaño(valores);
media(valores);
mediana(valores);
percentil(valores, 90);
```

## Arreglos multidimensionales

```milena
arreglo matriz = ceros(2, 3);

media(matriz, eje 0);
mediana(matriz, eje 1);
percentil(matriz, 90, eje 0);
```

Para conservar el eje reducido:

```milena
media(matriz, eje 0, conservar dimensiones);
mediana(matriz, eje 1, conservar dimensiones);
percentil(matriz, 90, eje 0, conservar dimensiones);
```

Para eliminarlo explícitamente:

```milena
media(matriz, eje 0, sin conservar dimensiones);
```

La sintaxis española se está incorporando gradualmente. Durante la transición pueden existir nombres históricos compatibles, pero los ejemplos nuevos deben preferir las palabras españolas.

## Operaciones disponibles

```milena
suma(valores);
media(valores);
minimo(valores);
maximo(valores);
varianza(valores);
desviacion_estandar(valores);
mediana(valores);
percentil(valores, 95);
```

Las operaciones ordenadas devuelven resultados numéricos de precisión doble y no modifican el arreglo original.

## Archivos y ejecución

Los scripts de Milena utilizan exclusivamente la extensión `.milena`.

```bash
./build.sh
./milena run ejemplos/estadistica.milena
./milena analizar datos.csv reporte.json
./milena perfil datos.csv perfil.json
```

La forma exacta de algunos comandos de archivos y reportes continúa evolucionando junto con el lenguaje. Los scripts deben conservar los datos de entrada, las reglas de limpieza y la versión del motor para facilitar la reproducción del análisis.

## Cómo se organiza Milena

```text
script .milena
      ↓
lexer y parser
      ↓
representación semántica
      ↓
motor de arreglos y tablas
      ↓
operaciones estadísticas
      ↓
resultado o reporte
```

El motor separa la forma en que una persona escribe una operación de la implementación interna que la ejecuta. Esta separación permite mejorar la sintaxis sin reescribir los cálculos fundamentales.

## Proceso de desarrollo

Milena avanza en capas:

1. Arreglos, formas, strides y broadcasting.
2. Estadística global y por eje.
3. Pruebas de vistas, errores y resultados reproducibles.
4. Sintaxis española estructurada.
5. Álgebra lineal y memoria optimizada.
6. Árboles de decisión y bosques.
7. Integración numérica y optimización.
8. Diferenciación automática y modelos científicos avanzados.
9. Optimizaciones específicas de cada plataforma.

Cada etapa debe conservar la portabilidad, la trazabilidad y los errores explícitos. Las optimizaciones de hardware se incorporarán como mejoras opcionales, no como requisitos que limiten el uso del lenguaje.

## Limitaciones actuales

Milena todavía no es una plataforma distribuida ni un sistema completo de aprendizaje automático. Algunas funciones estadísticas y partes de la sintaxis siguen en integración. Los resultados deben interpretarse según los datos, el método utilizado y el contexto del análisis.

Milena no sustituye una auditoría, una investigación profesional, una decisión médica, legal o financiera, ni una validación especializada.

## Estado del proyecto

El desarrollo se valida continuamente en entornos Linux y Windows. La compatibilidad con Termux es una prioridad del proyecto, junto con un núcleo pequeño, portable y controlable.

Milena aspira a ser un lenguaje importante para el análisis de datos porque combina una sintaxis progresivamente más clara con un motor especializado en arreglos, estadística reproducible y computación científica. Esa aspiración se construye con resultados verificables, no con promesas de capacidades que todavía no existen.

## Licencia

Milena se distribuye bajo la licencia MIT.
