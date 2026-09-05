#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VERSION="${MANO_VERSION:-0.1.0}"
CC_BIN="${CC:-cc}"
ARCH="${MANO_DEB_ARCH:-$(dpkg --print-architecture)}"
DIST_DIR="$ROOT_DIR/dist/debian"
STAGE="$DIST_DIR/stage"

for command in "$CC_BIN" make dpkg dpkg-deb; do
    command -v "$command" >/dev/null 2>&1 || {
        echo "Falta el comando requerido: $command" >&2
        exit 1
    }
done

cd "$ROOT_DIR"
make clean
make CC="$CC_BIN"
CC="$CC_BIN" make test

rm -rf "$STAGE"
mkdir -p "$STAGE/usr/bin" "$STAGE/usr/share/doc/mano"
cleanup() { rm -rf "$STAGE"; }
trap cleanup EXIT
install -m 0755 mano "$STAGE/usr/bin/mano"
install -m 0644 README.md "$STAGE/usr/share/doc/mano/README.md"
cp -R examples "$STAGE/usr/share/doc/mano/"

mkdir -p "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<EOF
Package: mano
Version: $VERSION
Section: science
Priority: optional
Architecture: $ARCH
Maintainer: Mano SST <maintainers@mano.invalid>
Description: Mano SST data analysis language
 Mano detects statistical patterns in existing occupational safety and health data.
EOF

find "$STAGE" -type d -exec chmod 0755 {} +
find "$STAGE" -type f -exec chmod 0644 {} +
chmod 0755 "$STAGE/usr/bin/mano"

mkdir -p "$DIST_DIR"
OUTPUT="$DIST_DIR/mano_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$STAGE" "$OUTPUT" >/dev/null
rm -rf "$STAGE"
printf 'Paquete Debian creado: %s\n' "$OUTPUT"
