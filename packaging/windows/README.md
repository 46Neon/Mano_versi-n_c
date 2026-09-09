# Distribución Windows

Windows no utiliza `apt` ni `pkg`. El canal de distribución será:

1. `milena.exe` compilado en un runner Windows;
2. instalador `.exe` o `.msi` cuando exista uno real y probado;
3. archivo `.zip` portable;
4. manifest para WinGet después de publicar una versión pública.

El código debe validarse con LLVM/Clang en Windows. No se debe asumir que un binario Linux o Termux funciona en Windows.

## Estado

El ejecutable Windows y sus pruebas se validan mediante CI/CD. La distribución final todavía requiere un paquete portable o instalador publicado y una Release verificable.

## WinGet

El manifest requiere una URL HTTPS pública de GitHub Release y el SHA-256 definitivo del artefacto. No se deben inventar URLs, hashes ni switches de instalación.

Mientras el artefacto sea un ejecutable directo sin instalador, el generador usa `InstallerType: portable` y declara el comando `milena`. Esto representa correctamente que WinGet debe colocar el ejecutable portable y no tratarlo como un instalador Inno Setup.

Para generar el manifest después de publicar el artefacto:

```powershell
$hash = (Get-FileHash .\milena.exe -Algorithm SHA256).Hash
.\generate-winget-manifest.ps1 `
  -Version '0.1.1' `
  -InstallerUrl 'https://github.com/46Neon/Milena/releases/download/v0.1.1/milena.exe' `
  -InstallerSha256 $hash
```

El modo `-InstallerType exe` solo debe usarse cuando exista un instalador real y se conozcan sus switches silenciosos. En ese caso es obligatorio proporcionar `-SilentSwitch` explícitamente.

Antes de proponer el paquete a WinGet hay que ejecutar `winget validate` sobre los tres YAML generados y verificar la instalación desde cero.
