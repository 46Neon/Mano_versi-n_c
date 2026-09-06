# Referencia NumPy para Milena

## Propósito

NumPy se utilizará como referencia semántica y arquitectónica para el núcleo numérico de Milena. No se incorporará la C-API de Python ni se copiará la implementación completa de NumPy. Milena mantendrá su lexer, parser, AST, IR, VM y runtime nativos en C17.

La meta es que las operaciones equivalentes tengan reglas claras y resultados comparables, mientras Milena añade tipos tabulares, decimales financieros y módulos de dominio.

## Mapa conceptual

| NumPy | Milena | Responsabilidad |
|---|---|---|
| `ndarray` | `MilenaArray` | Datos homogéneos multidimensionales |
| `dtype` | `MilenaDType` | Tipo y representación de cada elemento |
| `shape` | `shape` | Tamaño de cada eje |
| `strides` | `strides` | Salto de memoria por eje |
| `ufunc` | registro de kernels | Operación vectorizada y despacho de tipos |
| `axis` | eje | Dimensión sobre la que se calcula |
| broadcasting | planificador de formas | Compatibilidad de operandos |
| vista | array no propietario | Memoria compartida con un propietario |
| `out` | salida opcional | Reutilización controlada de memoria |
| `nan*` | operaciones con NaN | Tratamiento explícito de NaN |
| `np.linalg` | `milena.linalg` | Álgebra lineal |
| `np.random.Generator` | `milena.random` | Aleatoriedad reproducible |

## MilenaArray v1

```c
typedef struct {
    void *data;
    MilenaDType dtype;
    size_t ndim;
    size_t *shape;
    ptrdiff_t *strides;
    size_t itemsize;
    size_t size;
    size_t byte_offset;
    unsigned flags;
    struct MilenaArray *owner;
} MilenaArray;
```

### Reglas

1. `shape` tiene `ndim` elementos.
2. `strides` se expresan en bytes.
3. `size` es el número total de elementos, no el tamaño en bytes.
4. Un array propietario libera `data`, `shape` y `strides`.
5. Una vista conserva un puntero `owner` y no libera la memoria compartida.
6. Un stride cero representa una dimensión broadcast.
7. Las escrituras sobre vistas broadcast se rechazan salvo que exista una operación explícita y segura.
8. Los desbordamientos de tamaño se detectan antes de reservar memoria.
9. La forma `[]` representa un escalar 0-D; `[n]` representa un vector 1-D.
10. La ausencia de un valor se representa con una máscara de validez, no confundida automáticamente con NaN.

## Tipos iniciales

### Enteros y booleanos

```text
bool
int8, int16, int32, int64
uint8, uint16, uint32, uint64
```

### Reales y complejos

```text
float32
float64
complex64
complex128
```

### Tipos de dominio

```text
text
date
null
decimal
money
currency
```

`decimal`, `money` y `currency` no forman parte del primer kernel numérico, pero deben reservarse en el diseño de `MilenaValue` para no bloquear la lógica financiera.

## Broadcasting v1

Dos formas son compatibles si, al alinearlas desde el último eje, cada par cumple una de estas condiciones:

```text
igualdad
uno de los tamaños es 1
el eje no existe en el operando menor
```

Ejemplos:

```text
[3, 4] + [4]    -> [3, 4]
[3, 1] + [1, 4] -> [3, 4]
[3, 4] + [3]    -> error
```

El planificador debe devolver una forma resultado y strides lógicos para cada operando. No debe crear copias solo para simular la expansión.

## Registro de kernels

Cada operación debe registrarse con:

```text
nombre
número de entradas
número de salidas
reglas de dtype
reglas de casting
soporte de broadcasting
kernel por dtype
```

Primer grupo:

```text
add, subtract, multiply, divide
negative, abs, sign
sqrt, power
minimum, maximum
isnan, isfinite
```

Después:

```text
sin, cos, tan
exp, log, log10
floor, ceil, round
```

## Reducciones

Todas las reducciones compartirán una infraestructura común:

```text
reduce(operation, array, axis, keepdims, dtype, out)
```

Operaciones iniciales:

```text
sum
prod
min
max
mean
std
var
```

Reglas que deben definirse antes de implementarlas:

- resultado de array vacío;
- acumulación de enteros;
- dtype de salida;
- NaN y valores faltantes;
- eje negativo;
- `axis = none`;
- `keepdims`;
- división por cero;
- precisión y tolerancia.

## Vistas y transformaciones

Primera etapa:

```text
reshape
ravel
flatten
transpose
swapaxes
squeeze
expand_dims
slice
```

`reshape`, `ravel`, `transpose`, `slice` y `squeeze` intentarán devolver vistas cuando la distribución de memoria lo permita. `flatten` siempre devolverá una copia propietaria.

## Indexación

La implementación se dividirá en tres niveles:

1. índices escalares;
2. slices por eje;
3. máscaras y arrays de índices.

No se implementará toda la indexación avanzada en un único paso. Cada nivel tendrá validación de forma, límites y propiedad de memoria.

## MilenaTable

`MilenaTable` será diferente de `MilenaArray`:

```text
MilenaTable
  schema
  column_names
  columns[] -> MilenaArray
  validity_masks[]
  row_count
```

Una tabla puede tener columnas heterogéneas. Un array debe mantener un dtype homogéneo por operación.

Operaciones de tabla prioritarias:

```text
select
filter
sort
group_by
aggregate
join
pivot
fill_null
drop_null
```

## Finanzas sobre el núcleo numérico

La lógica financiera no usará directamente `float64` para dinero.

```text
MilenaDecimal
MilenaMoney
MilenaRate
MilenaCashFlow
```

Operaciones iniciales:

```text
money
round_money
simple_interest
compound_interest
present_value
future_value
net_present_value
internal_rate_of_return
payment
amortization
```

Cada resultado financiero debe conservar moneda, periodo, regla de redondeo y advertencias relevantes.

## Pruebas diferenciales

Para cada operación compatible se prepararán casos equivalentes en NumPy y Milena.

### Igualdad exacta

```text
bool
enteros
formas
índices
conteos
```

### Tolerancia numérica

```text
float32
float64
trigonometría
álgebra lineal
```

La tolerancia debe registrarse por operación. No se debe ocultar una pérdida de precisión usando una tolerancia excesiva.

### Casos obligatorios

```text
arrays vacíos
arrays 0-D
arrays 1-D y 2-D
formas incompatibles
strides no contiguos
NaN e infinito
valores faltantes
overflow
índices fuera de rango
memoria insuficiente
```

## Orden de implementación

1. `MilenaDType` y cálculo seguro de tamaños.
2. `MilenaArray` propietario.
3. vistas, strides y reshape.
4. broadcasting.
5. kernels aritméticos.
6. reducciones por eje.
7. slicing y máscaras.
8. `MilenaTable`.
9. estadística general.
10. decimal y dinero.
11. álgebra lineal.
12. módulos de inteligencia artificial.

## Criterio de finalización de la primera etapa

La primera etapa estará completa cuando Milena pueda:

```text
crear arrays tipados
calcular shape y strides
sumar array + array
sumar array + escalar
aplicar broadcasting
hacer reshape sin copiar cuando sea posible
crear una vista
reducir por eje
rechazar formas incompatibles
liberar correctamente propietario y vistas
```

No se añadirá una gran cantidad de funciones hasta que estas garantías estén cubiertas por pruebas.
