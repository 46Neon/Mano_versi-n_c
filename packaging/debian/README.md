# Paquete Debian/Ubuntu

Este canal es independiente del paquete Termux. Aunque ambos usan `.deb`, el binario Debian se compila contra el entorno Linux objetivo y se instala bajo `/usr/bin` y `/usr/share`.

## Construcción local

En Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential clang dpkg-dev
./packaging/debian/build-local-deb.sh
```

Resultado esperado:

```text
dist/debian/mano_VERSION_amd64.deb
```

Para ARM64 se necesita un toolchain cruzado o un runner ARM64 y una compilación separada. No se debe reutilizar un `.deb` generado para Termux.
