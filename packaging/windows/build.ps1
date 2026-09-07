$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Version = if ($env:MANO_VERSION) { $env:MANO_VERSION } else { '0.1.1' }
$OutputDir = Join-Path $Root 'dist/windows'
$Output = Join-Path $OutputDir 'milena.exe'

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$SourceNames = @(
    'analysis.c', 'array.c', 'common.c', 'dataset.c', 'logger.c', 'main.c', 'metrics.c',
    'schema.c', 'script.c', 'sst_advanced.c', 'sst_contingency.c',
    'sst_correlation.c', 'sst_dates.c', 'sst_histogram.c', 'sst_inference.c',
    'sst_model.c', 'sst_normality.c', 'sst_rates.c', 'sst_report.c',
    'sst_report_advanced.c', 'sst_stats.c'
)
$Sources = $SourceNames | ForEach-Object { Join-Path $Root "src/$_" }

$Compiler = if ($env:CC) { $env:CC } else { 'clang' }
& $Compiler -std=c17 -Wall -Wextra -Wpedantic -O2 `
    '-Iinclude' $Sources '-o' $Output
if ($LASTEXITCODE -ne 0) { throw "No se pudo compilar Milena para Windows" }

Write-Host "Ejecutable creado: $Output"
Write-Host "Pendiente: crear instalador firmado y ejecutar las pruebas Windows."
