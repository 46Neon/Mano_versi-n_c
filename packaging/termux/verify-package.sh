#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

PACKAGE="${1:-}"
EXPECTED_ARCH="${TERMUX_PACKAGE_ARCH:-aarch64}"

if [[ -z "$PACKAGE" || ! -f "$PACKAGE" ]]; then
    echo "Uso: $0 ruta/al/paquete.deb" >&2
    exit 2
fi
for command in dpkg-deb dpkg; do
    command -v "$command" >/dev/null 2>&1 || {
        echo "Falta el comando requerido: $command" >&2
        exit 1
    }
done

name="$(dpkg-deb -f "$PACKAGE" Package)"
version="$(dpkg-deb -f "$PACKAGE" Version)"
arch="$(dpkg-deb -f "$PACKAGE" Architecture)"
[[ "$name" == "milena" ]] || { echo "Paquete inesperado: $name" >&2; exit 1; }
[[ "$arch" == "$EXPECTED_ARCH" ]] || {
    echo "Arquitectura inesperada: $arch (esperada: $EXPECTED_ARCH)" >&2
    exit 1
}
dpkg-deb --contents "$PACKAGE" | grep -Fq "./data/data/com.termux/files/usr/bin/milena"
dpkg-deb --contents "$PACKAGE" | grep -Fq "./data/data/com.termux/files/usr/share/doc/milena/README.md"
printf 'Paquete válido: %s %s %s\n' "$name" "$version" "$arch"
