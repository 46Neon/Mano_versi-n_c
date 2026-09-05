# Empaquetado de Mano

El código fuente y el empaquetado se mantienen conceptualmente separados:

- `Mano_versi-n_c`: motor, lenguaje, módulos SST y pruebas.
- `packaging/termux`: construcción del paquete para Termux.

El primer objetivo es construir y probar un `.deb` nativo de Termux. Todavía no se publica un repositorio APT ni se afirma que `pkg install mano` esté disponible para terceros.

## Plataformas

- **Termux:** paquete `.deb` construido con Clang y rutas bajo `$PREFIX`.
- **Debian/Ubuntu:** paquete separado, compilado contra su libc y dependencias.
- **Windows:** binario `.exe` y, posteriormente, instalador o manifest de WinGet.

Un `.deb` de Termux no es intercambiable con uno de Debian/Ubuntu. Cada plataforma y arquitectura requiere una compilación independiente.
