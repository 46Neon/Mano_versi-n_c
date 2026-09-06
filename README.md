# Milena

Milena es un lenguaje y motor de análisis de datos escrito en C17. Su objetivo es trabajar con datos generales de forma reproducible: cargar tablas, validar tipos y calidad, transformar columnas, calcular estadísticas y generar reportes que puedan ser revisados por personas y por otras herramientas.

El soporte SST es el primer dominio desarrollado, no el límite del proyecto. La lógica financiera, los análisis estadísticos generales y los módulos opcionales de inteligencia artificial se incorporarán de forma progresiva. La visualización no forma parte del núcleo actual.

> **Alcance:** Milena es una herramienta de apoyo. No sustituye a profesionales, auditorías, investigaciones, sistemas oficiales, asesoría legal, contable o médica, ni convierte correlaciones en causalidad.

## Estado actual

- Carga y validación de CSV con límites operativos.
- Perfilado de columnas, valores numéricos, categorías y variables binarias.
- Limpieza de nulos y duplicados.
- Estadística descriptiva, correlación, histogramas, normalidad aproximada y pruebas de contingencia.
- Módulos iniciales para análisis SST.
- Reportes JSON reproducibles.
- Compilación C17 con advertencias estrictas y pruebas automatizadas.

Milena analiza datos existentes y debe mostrar advertencias cuando un método sea aproximado o cuando no permita extraer conclusiones causales.

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
./milena analizar mi_archivo.csv reporte_milena.json
./milena perfil mi_archivo.csv perfil.json
./milena run examples/clasificacion_binaria.mano
./milena inspect mi_archivo.csv
```

Un script produce el reporte general indicado en `.exportar`. Si contiene operaciones SST, también produce un archivo con el sufijo:

```text
reporte_milena.json.sst.json
```

## Sintaxis de variables

```milena
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

```milena
#nulos("eliminar")
#duplicados("eliminar")
#total("precio * cantidad")
#periodo extraer("mes de fecha")
#condicion("total > 0")
```

## Operaciones estadísticas SST

```milena
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

Un comando desconocido no se ignora. Milena produce un error explícito para evitar informes aparentemente completos:

```text
UNSUPPORTED: Comando Milena no reconocido; no se ignorará silenciosamente
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

Estos límites hacen que el almacenamiento actual en memoria sea razonable para el alcance inicial. Milena no es todavía un sistema distribuido, un motor columnar ni una plataforma Big Data.

## Arquitectura SST

```text
CSV
 ↓
calidad y validación
 ↓
esquema y script Milena
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
Milena detecta patrones y señales
        ↓
Profesional SST interpreta y valida
        ↓
Investigación preventiva
        ↓
Medidas de control y seguimiento
```

Milena puede ayudar a identificar áreas con mayor frecuencia, cambios temporales, distribución de severidad, diferencias entre turnos y posibles relaciones entre variables. No determina responsabilidades, no prueba causalidad y no reemplaza entrevistas, inspecciones, evidencias ni métodos formales de investigación.

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

## Instalación en Termux mediante APT

Una vez publicado el repositorio APT, la instalación para una persona usuaria no requiere clonar el código ni compilar Milena. Se necesita la dirección pública del sitio APT configurado para la distribución y la clave pública del repositorio.

```bash
pkg install curl gnupg
mkdir -p "$PREFIX/etc/apt/keyrings" "$PREFIX/etc/apt/sources.list.d"
NETLIFY_REPO_URL="PEGA_AQUI_LA_DIRECCION_PUBLICA_DEL_REPOSITORIO"
curl -fsSL "$NETLIFY_REPO_URL/milena-archive-keyring.asc" \
  | gpg --dearmor \
  > "$PREFIX/etc/apt/keyrings/milena-archive.gpg"
printf 'deb [signed-by=%s] %s stable main\n' \
  "$PREFIX/etc/apt/keyrings/milena-archive.gpg" \
  "$NETLIFY_REPO_URL" \
  > "$PREFIX/etc/apt/sources.list.d/milena.list"
apt-get update --allow-releaseinfo-change
pkg install milena
```

Para actualizar:

```bash
pkg update
pkg upgrade milena
```

Para desinstalar solamente Milena:

```bash
apt remove milena
```

La dirección pública del repositorio debe copiarse desde el despliegue de Netlify; no debe confundirse con el panel privado de administración.

## Empaquetado para Termux

El flujo de empaquetado se encuentra en `packaging/termux/`. La primera fase genera un `.deb` local para la arquitectura de Termux donde se ejecuta:

```bash
./packaging/termux/build-local-deb.sh
```

Para ofrecer una instalación pública mediante:

```bash
pkg install milena
```

El repositorio APT se genera con índices `Packages.gz`, metadatos `Release`/`InRelease`, firmas y paquetes separados por arquitectura. GitHub Actions lo publica en un sitio estático de Netlify; la clave privada GPG permanece en los secretos de Actions.

Un paquete Termux no debe mezclarse con un paquete Debian/Ubuntu ni con un ejecutable Windows. Cada plataforma requiere su propia compilación y distribución. Consulta `packaging/termux/README.md` antes de publicar.

El canal Debian/Ubuntu está en `packaging/debian/` y el canal Windows en `packaging/windows/`. La plantilla de automatización Linux/Windows está en `packaging/ci/build-release.yml`. El job Termux sigue siendo manual porque debe compilarse dentro del entorno Android/Termux y no debe sustituirse por un binario Linux con libc incompatible.

## Próximas mejoras

La hoja de ruta completa está en [`PLAN_MILENA.md`](PLAN_MILENA.md). El orden acordado es:

1. Consolidar la identidad Milena y la compatibilidad de la sintaxis actual.
2. Ampliar el núcleo de tablas y análisis de datos generales.
3. Incorporar lógica financiera con representación monetaria segura.
4. Mejorar rendimiento, memoria y arquitectura modular.
5. Añadir módulos de inteligencia artificial reproducibles y explicables.
6. Revisar la sintaxis cuando el modelo de datos esté estable.
7. Aumentar documentación, colaboración, adopción y madurez antes de solicitar nuevamente Termux.

## Nota de responsabilidad

Milena es una solución tecnológica de apoyo para análisis de datos SST y prevención. Las conclusiones que afecten la seguridad de trabajadores deben ser revisadas por personal competente y complementadas con la evidencia operativa correspondiente.
