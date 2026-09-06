#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
project_dir=$(pwd)
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT

cat > "$tmp_dir/ventas.csv" <<'CSV'
fecha,precio,cantidad
2026-01-01,10,2
2026-01-02,5,3
2026-01-03,2,4
CSV

./mano analizar "$tmp_dir/ventas.csv" "$tmp_dir/test-output.json" >/dev/null
test -s "$tmp_dir/test-output.json"
grep -q '"total": 43' "$tmp_dir/test-output.json"

cat > "$tmp_dir/clientes.csv" <<'CSV'
edad,ciudad,canal,visitas,compro
20,Caracas,web,3,1
22,Merida,web,4,1
25,Caracas,tienda,5,0
28,Valencia,web,6,1
31,Merida,tienda,7,0
35,Caracas,web,8,1
40,Valencia,tienda,9,0
45,Merida,web,10,1
CSV

sed \
  -e "s#datos/tu_archivo.csv#$tmp_dir/clientes.csv#g" \
  -e "s#reporte_clientes.json#$tmp_dir/reporte_clientes.json#g" \
  "$project_dir/examples/clasificacion_binaria.mano" \
  > "$tmp_dir/clasificacion_binaria.mano"

./mano run "$tmp_dir/clasificacion_binaria.mano" >/dev/null
test -s "$tmp_dir/reporte_clientes.json"
test -s "$tmp_dir/reporte_clientes.json.sst.json"
grep -q '"salidas_binarias"' "$tmp_dir/reporte_clientes.json"
grep -q '"unos": 5' "$tmp_dir/reporte_clientes.json"
grep -q '"histograma"' "$tmp_dir/reporte_clientes.json.sst.json"
grep -q '"pearson"' "$tmp_dir/reporte_clientes.json.sst.json"
grep -q '"normalidad"' "$tmp_dir/reporte_clientes.json.sst.json"

./mano perfil "$tmp_dir/clientes.csv" "$tmp_dir/perfil.json" >/dev/null
test -s "$tmp_dir/perfil.json"
printf 'OK: pruebas con datos sintéticos temporales completadas\n'
