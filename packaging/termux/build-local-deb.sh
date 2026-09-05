#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VERSION="${MANO_VERSION:-0.1.0}"
PREFIX_DIR="${PREFIX:-}"

if [[ -z "$PREFIX_DIR" || "$PREFIX_DIR" != */usr ]]; then
    echo "Este script debe ejecutarse dentro de Termux con PREFIX apuntando a .../usr" >&2
    exit 1
fi

for command in clang make dpkg dpkg-deb; do
    command -v "$command" >/dev/null 2>&1 || {
        echo "Falta el comando requerido: $command" >&2
        exit 1
    }
done

cd "$ROOT_DIR"
make clean
CC=clang make
CC=clang make test

ARCH="$(dpkg --print-architecture)"
DIST_DIR="$ROOT_DIR/dist/termux"
STAGE="$DIST_DIR/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/${PREFIX_DIR#/}/bin" \
         "$STAGE/${PREFIX_DIR#/}/share/doc/mano" \
         "$DIST_DIR"
cleanup() { rm -rf "$STAGE"; }
trap cleanup EXIT

install -m 0755 mano "$STAGE/${PREFIX_DIR#/}/bin/mano"
install -m 0644 README.md "$STAGE/${PREFIX_DIR#/}/share/doc/mano/README.md"
cp -R examples "$STAGE/${PREFIX_DIR#/}/share/doc/mano/"

CONTROL="$STAGE/DEBIAN"
mkdir -p "$CONTROL"
cat > "$CONTROL/control" <<EOF
Package: mano
Version: $VERSION
Section: science
Priority: optional
Architecture: $ARCH
Maintainer: Mano SST <maintainers@mano.invalid>
Description: Mano SST data analysis language
 Mano analyzes existing occupational safety and health data to detect
 statistical patterns and support accident prevention.
EOF

# Termux puede aplicar umask 077; dpkg-deb exige permisos legibles en DEBIAN.
find "$STAGE" -type d -exec chmod 0755 {} +
find "$STAGE" -type f -exec chmod 0644 {} +
chmod 0755 "$STAGE/${PREFIX_DIR#/}/bin/mano"

OUTPUT="$DIST_DIR/mano_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$STAGE" "$OUTPUT" >/dev/null
printf 'Paquete creado: %s\n' "$OUTPUT"
