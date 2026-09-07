$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Build = Join-Path $Root 'dist/windows-array-smoke'
New-Item -ItemType Directory -Force -Path $Build | Out-Null
$Compiler = if ($env:CC) { $env:CC } else { 'clang' }
$Flags = @('-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-O2', '-Iinclude')

# First compile the three requested translation units separately. This
# distinguishes a Windows compiler/portability error from a linker error.
& $Compiler @Flags '-c' (Join-Path $Root 'src/array.c') '-o' (Join-Path $Build 'array.o')
if ($LASTEXITCODE -ne 0) { throw 'array.c no compila en Windows' }
& $Compiler @Flags '-c' (Join-Path $Root 'src/common.c') '-o' (Join-Path $Build 'common.o')
if ($LASTEXITCODE -ne 0) { throw 'common.c no compila en Windows' }
& $Compiler @Flags '-c' (Join-Path $Root 'src/script.c') '-o' (Join-Path $Build 'script.o')
if ($LASTEXITCODE -ne 0) { throw 'script.c no compila en Windows' }

# The complete script object has legacy analysis dependencies. Link the
# minimal array executable separately so the array/common ABI is isolated.
$Smoke = Join-Path $Build 'array-link-smoke.exe'
& $Compiler @Flags (Join-Path $PSScriptRoot 'array-link-smoke.c') `
    (Join-Path $Root 'src/array.c') (Join-Path $Root 'src/common.c') '-o' $Smoke
if ($LASTEXITCODE -ne 0) { throw 'No se pudo enlazar el smoke test de MilenaArray' }
& $Smoke
if ($LASTEXITCODE -ne 0) { throw 'Falló la ejecución del smoke test de MilenaArray' }
