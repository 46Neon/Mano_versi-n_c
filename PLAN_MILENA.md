# Plan de evolución de Milena

## Objetivo

Convertir Milena en un lenguaje y motor C17 de análisis de datos general, reproducible y eficiente, sin limitarlo al dominio SST. La visualización queda fuera de esta fase; Milena producirá datos tabulares, estadísticas, modelos y reportes que otras herramientas podrán visualizar.

## Fase 0 — Identidad y base estable

- Renombrar el proyecto, ejecutable, paquete, documentación y símbolos públicos a `Milena`.
- Mantener temporalmente la extensión `.mano` para no cambiar todavía la sintaxis ni romper scripts existentes.
- Crear una versión de transición con pruebas de regresión antes de añadir funciones nuevas.
- Actualizar la distribución y los instaladores después de validar el nuevo nombre.
- Mantener licencias, historial limpio, compilación reproducible y pruebas con GCC, Clang, ASan y UBSan.

## Fase 1 — Núcleo de datos general

- Tipos primitivos: entero, real, booleano, texto, fecha/hora y nulo.
- Tablas con nombres de columnas, tipos, índices y metadatos.
- Carga y exportación de CSV y JSON; después formatos columnares si la implementación lo justifica.
- Selección, filtrado, ordenamiento, unión, agrupación, agregación, pivote y transformación de columnas.
- Valores faltantes, duplicados, conversiones seguras y errores explicables.
- Límites de memoria, lectura por lotes y procesamiento incremental para archivos grandes.

## Fase 2 — Estadística y calidad

- Estadística descriptiva y cuantiles.
- Covarianza, correlación y tablas de contingencia.
- Pruebas inferenciales con supuestos y advertencias explícitas.
- Muestreo, intervalos de confianza y detección de valores atípicos.
- Pruebas de referencia contra resultados conocidos y comparación de precisión numérica.

## Fase 3 — Lógica financiera

- Decimal exacto o representación monetaria segura, separada de `double`.
- Monedas, redondeo, impuestos, descuentos y tasas con reglas explícitas.
- Fechas de pago, flujos de caja, amortización, valor presente y valor futuro.
- Validación de unidades, periodos y convenciones contables.
- Casos de prueba con valores de referencia y advertencias sobre límites legales o contables.

## Fase 4 — Rendimiento y arquitectura

- Separar el lenguaje, el motor de tablas, la estadística y los módulos de dominio.
- Evaluar almacenamiento columnar y ejecución vectorizada sin copiar datos innecesariamente.
- Paralelismo controlado cuando la semántica sea determinista.
- Benchmarks públicos frente a cargas pequeñas, medianas y grandes.
- API modular para agregar nuevos formatos y algoritmos.

## Fase 5 — Inteligencia artificial responsable

- Estadísticas de diagnóstico antes de usar modelos.
- Regresión, clasificación, agrupamiento y reducción de dimensión como módulos separados.
- Semillas reproducibles, particiones de datos y métricas de evaluación.
- Explicación de variables, advertencias de sesgo y detección de fuga de información.
- Integración opcional con modelos externos; el núcleo debe seguir funcionando sin conexión ni servicios propietarios.
- No presentar predicciones como causalidad ni como decisiones profesionales automáticas.

## Fase 6 — Sintaxis

La sintaxis se revisará después de estabilizar el modelo de datos y la lógica financiera. En ese momento se decidirá si conviene mantener compatibilidad con `.mano`, introducir `.milena` o soportar ambas extensiones durante una transición documentada.

## Fase 7 — Madurez para Termux

- Documentación clara de instalación y ejemplos generales.
- Releases regulares y changelog.
- Pruebas automáticas para aarch64, arm, i686 y x86_64 cuando estén disponibles.
- Issues reproducibles, colaboradores externos y revisiones públicas.
- Paquete reproducible sin root, sin binarios incluidos en el repositorio y con licencia MIT.
- Solicitar nuevamente la revisión oficial cuando Milena tenga actividad y adopción más allá de un proyecto personal.

## Principios

1. No prometer funciones que todavía no estén implementadas.
2. Preferir resultados reproducibles sobre automatismos opacos.
3. Separar el núcleo general de los módulos SST y financiero.
4. No incluir visualización en el núcleo durante esta etapa.
5. Mantener compatibilidad documentada durante las migraciones.
