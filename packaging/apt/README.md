# Repositorio APT de Mano

Este canal publica los paquetes `.deb` de Mano para que los usuarios puedan instalar desde APT. Termux y Debian/Ubuntu usan índices separados por arquitectura, aunque los paquetes se sirvan desde el mismo sitio.

## Qué necesita el repositorio

```text
dists/stable/Release
dists/stable/InRelease
dists/stable/Release.gpg
dists/stable/main/binary-aarch64/Packages.gz
dists/stable/main/binary-amd64/Packages.gz
pool/main/m/mano/*.deb
mano-archive-keyring.asc
```

El paquete `aarch64` debe ser el construido para Termux. El paquete `amd64` debe ser el construido para Debian/Ubuntu. No se intercambian.

## Secretos de GitHub Actions

Configura en el repositorio:

```text
MANO_GPG_PRIVATE_KEY
MANO_GPG_KEY_ID
```

La clave privada no debe entrar al repositorio. La clave pública se publica como `mano-archive-keyring.asc` para que los usuarios puedan verificar el repositorio.

## Flujo

1. Crear una Release con los archivos `.deb`.
2. Ejecutar el workflow `publish-apt.yml` sobre la etiqueta.
3. Descargar los `.deb` de la Release.
4. Generar índices y firmas.
5. Publicar `dist/apt` en la rama `gh-pages`.
6. Activar GitHub Pages para servir esa rama.
7. Probar el repositorio desde Termux antes de anunciarlo.

La plantilla está en `packaging/ci/publish-apt.yml`. Primero debe copiarse a `.github/workflows/publish-apt.yml` y publicarse con permisos de workflow.
