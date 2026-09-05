# Mano SST

Mano es un lenguaje y motor pequeño escrito en C17 para analizar datos SST existentes. Su objetivo es detectar patrones, tendencias, distribuciones y comportamientos estadísticos que puedan apoyar la prevención de accidentes laborales.

> **Alcance:** Mano es una herramienta tecnológica de apoyo. No sustituye al profesional de seguridad y salud laboral, una investigación de accidentes, un sistema oficial, una evaluación legal ni la toma de decisiones profesionales.

## Qué puede hacer

- Cargar CSV con límites operativos para datasets pequeños.
- Validar columnas, valores numéricos, categorías y variables binarias.
- Analizar incidentes, severidad, días de incapacidad y exposición.
- Comparar áreas, turnos, riesgos y grupos.
- Detectar patrones, valores inválidos y señales estadísticas.
- Generar reportes JSON reproducibles para revisión profesional.
- Mantener advertencias explícitas sobre aproximaciones y causalidad.

Mano analiza datos que ya existen; no determina por sí sola la causa de un accidente ni recomienda medidas legales automáticamente.

## Compilar

```bash
./build.sh
```

También se puede compilar con:

```bash
make
```

La compilación utiliza C17 y advertencias estrictas. En un entorno de validación se recomienda ejecutar también GCC y Clang con ASan y UBSan.

## Ejecutar

```bash
./mano analizar datos/ventas.csv reporte.json
./mano perfil datos/clientes_binarios.csv perfil.json
./mano run examples/clasificacion_binaria.mano
./mano inspect datos/clientes_binarios.csv
```

Un script produce el reporte general indicado en `.exportar`. Si contiene operaciones SST, también produce un archivo con el sufijo:

```text
reporte.json.sst.json
```

## Sintaxis de variables

```mano
variable fecha fecha
variable area categorica
variable turno categorica
variable severidad numerica
variable dias_incapacidad numerica
variable horas_exposicion numerica
variable ocurrio_incidente binaria

entrada categorica "area"
salida binaria "ocurrio_incidente"
```

Tipos soportados:

- `numerica`: validación con `strtod`, promedio, mínimo y máximo.
- `categorica`: conteo de valores y nulos.
- `binaria`: reconoce `0/1`, `true/false`, `verdadero/falso`, `yes/no` y `si/no`.
- `fecha` y `texto`: metadata y validación básica.

## Operaciones de limpieza

```mano
#nulos("eliminar")
#duplicados("eliminar")
#total("precio * cantidad")
#periodo extraer("mes de fecha")
#condicion("total > 0")
```

## Operaciones estadísticas SST

```mano
#perfil_numerico("severidad")
#perfil_avanzado("severidad")
#histograma("severidad", bins = 5)
#normalidad("severidad")
#balance("ocurrio_incidente")
#tasa("ocurrio_incidente", "horas_exposicion", factor = 200000)
#poisson("ocurrio_incidente", "horas_exposicion", factor = 200000)
#correlacion("severidad", "dias_incapacidad")
#chi_cuadrado("area", "ocurrio_incidente")
```

### Interpretación de las operaciones

- `#perfil_numerico`: resumen descriptivo de una variable.
- `#perfil_avanzado`: CV, asimetría, kurtosis y percentiles.
- `#histograma`: distribución por intervalos, underflow y overflow.
- `#normalidad`: diagnóstico Jarque-Bera aproximado; no es Shapiro-Wilk exacto.
- `#balance`: conteo de positivos, negativos e inválidos.
- `#tasa`: incidentes divididos entre exposición y multiplicados por un factor.
- `#poisson`: intervalo de tasa para conteos Poisson mediante inversión numérica de la CDF en rangos soportados.
- `#correlacion`: Pearson con pares válidos y control de variación.
- `#chi_cuadrado`: tabla de contingencia, estadístico, grados de libertad y celdas esperadas bajas.

Las tasas siempre deben interpretarse junto con su denominador, factor, periodo y definición de exposición.

## Comandos no soportados

Un comando desconocido no se ignora. Mano produce un error explícito para evitar informes aparentemente completos:

```text
UNSUPPORTED: Comando Mano no reconocido; no se ignorará silenciosamente
```

Las funciones de riesgo relativo, odds ratio, Mann-Whitney y Wilcoxon existen como módulos C en esta etapa, pero su sintaxis `.mano` todavía debe terminar de integrarse y validarse antes de presentarse como operaciones del lenguaje.

## Reportes y advertencias

Los reportes incluyen, según corresponda:

- filas cargadas y filas inválidas;
- columnas y variables declaradas;
- perfiles numéricos;
- categorías y frecuencias;
- valores nulos;
- positivos, negativos e inválidos;
- tasa y denominador;
- tamaño muestral;
- datos excluidos;
- método utilizado;
- indicador `aproximado`;
- advertencias de muestra pequeña;
- advertencia de que asociación estadística no implica causalidad.

Estas advertencias son controles de interpretación, no una certificación legal ni una firma profesional.

## Trazabilidad prevista

Para que un análisis sea reproducible, el flujo profesional debe conservar:

- archivo CSV original;
- script `.mano` utilizado;
- configuración y factor de exposición;
- versión del motor;
- reporte generado;
- filas rechazadas y reglas de limpieza;
- revisión del profesional SST.

El logger del proyecto soporta texto y JSON Lines con `job_id`, timestamp, archivo, línea, función y mensaje. El módulo de métricas soporta contadores de filas y duración. La instrumentación completa de todas las operaciones continúa en evolución.

## Límites por defecto

```text
máximo de filas:          5.000
máximo de columnas:          70
máximo aproximado por campo: 1 MiB
```

Estos límites hacen que el almacenamiento actual en memoria sea razonable para el alcance inicial. Mano no es todavía un sistema distribuido, un motor columnar ni una plataforma Big Data.

## Arquitectura SST

```text
CSV
 ↓
calidad y validación
 ↓
esquema y script Mano
 ↓
operación estadística
 ↓
módulo SST
 ↓
advertencias y trazabilidad
 ↓
reporte JSON
```

Módulos principales:

- `sst_dates`: fechas ISO, comparación, fechas futuras y días desde epoch.
- `sst_model`: eventos SST, riesgos y valores binarios.
- `sst_stats`: media, varianza, desviación estándar y Welford.
- `sst_histogram`: histogramas con underflow, overflow e inválidos.
- `sst_rates`: tasas por exposición y factor configurable.
- `sst_advanced`: CV, asimetría, kurtosis y percentiles.
- `sst_contingency`: tablas de contingencia y chi-cuadrado.
- `sst_correlation`: correlación de Pearson.
- `sst_normality`: diagnóstico Jarque-Bera aproximado.
- `sst_inference`: Poisson, riesgo relativo, odds ratio y pruebas aproximadas.
- `sst_report`: reportes generales.
- `sst_report_advanced`: reportes estadísticos avanzados y advertencias.
- `logger`: logging de texto o JSON Lines.
- `metrics`: duración y contadores de ejecución.

## Uso preventivo correcto

```text
Datos SST existentes
        ↓
Mano detecta patrones y señales
        ↓
Profesional SST interpreta y valida
        ↓
Investigación preventiva
        ↓
Medidas de control y seguimiento
```

Mano puede ayudar a identificar áreas con mayor frecuencia, cambios temporales, distribución de severidad, diferencias entre turnos y posibles relaciones entre variables. No determina responsabilidades, no prueba causalidad y no reemplaza entrevistas, inspecciones, evidencias ni métodos formales de investigación.

## Limitaciones actuales

- No sustituye sistemas oficiales ni formularios regulatorios.
- No certifica cumplimiento legal.
- No determina causalidad.
- No firma digitalmente reportes.
- No gestiona expedientes médicos.
- No propone automáticamente medidas de control.
- Algunos métodos inferenciales son aproximados.
- La integración de riesgo relativo, odds ratio, Mann-Whitney y Wilcoxon con la sintaxis `.mano` sigue pendiente.
- La ejecución con GCC, Clang, ASan, UBSan y herramientas de fugas debe validarse en CI.

## Empaquetado para Termux

El flujo de empaquetado se encuentra en `packaging/termux/`. La primera fase genera un `.deb` local para la arquitectura de Termux donde se ejecuta:

```bash
./packaging/termux/build-local-deb.sh
```

Para ofrecer una instalación pública mediante:

```bash
pkg install mano
```

todavía hay que publicar un repositorio APT con índices `Packages.gz`, metadatos `Release`/`InRelease`, firmas y paquetes separados por arquitectura. GitHub Pages puede servir esos archivos estáticos, pero una CI debe generarlos y firmarlos antes del despliegue.

Un paquete Termux no debe mezclarse con un paquete Debian/Ubuntu ni con un ejecutable Windows. Cada plataforma requiere su propia compilación y distribución. Consulta `packaging/termux/README.md` antes de publicar.

El canal Debian/Ubuntu está en `packaging/debian/` y el canal Windows en `packaging/windows/`. La plantilla de automatización Linux/Windows está en `packaging/ci/build-release.yml`. El job Termux sigue siendo manual porque debe compilarse dentro del entorno Android/Termux y no debe sustituirse por un binario Linux con libc incompatible.

## Próximas mejoras

1. Completar la integración de inferencia con `.mano`.
2. Añadir huellas SHA-256 de datos, scripts y reportes.
3. Instrumentar completamente logger y métricas.
4. Agregar pruebas estadísticas contra valores de referencia.
5. Añadir análisis temporal y comparaciones antes/después.
6. Crear una matriz configurable de peligros, controles y señales preventivas.
7. Incorporar exportación tabular y documentación de métodos.
8. Ejecutar validación con GCC, Clang, ASan, UBSan y pruebas de memoria.

## Nota de responsabilidad

Mano es una solución tecnológica de apoyo para análisis de datos SST y prevención. Las conclusiones que afecten la seguridad de trabajadores deben ser revisadas por personal competente y complementadas con la evidencia operativa correspondiente.
