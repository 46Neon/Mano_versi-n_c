# Distribución Windows

Windows no utiliza `apt` ni `pkg`. El canal de distribución será:

1. `milena.exe` compilado en un runner Windows;
2. instalador `.exe` o `.msi`;
3. archivo `.zip` portable;
4. manifest para WinGet después de publicar una versión pública.

El código debe validarse con MSYS2/MinGW o LLVM-MinGW. No se debe asumir que un binario Linux o Termux funciona en Windows.

## Estado

El repositorio contiene el script inicial `build.ps1`, pero el instalador Windows no se publica como listo hasta comprobar la portabilidad de todas las fuentes C y ejecutar las pruebas en Windows.

## WinGet

El manifest real requiere una URL pública de GitHub Release y un SHA-256 definitivo. Por eso se genera después de crear la Release, no antes. `generate-winget-manifest.ps1` evita publicar un manifest con URLs o hashes inventados.
