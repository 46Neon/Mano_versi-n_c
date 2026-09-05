#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
rm -f test-output.json reporte_dataset.json reporte_clientes.json reporte_clientes.json.sst.json perfil.json
./mano analizar datos/ventas.csv test-output.json >/dev/null
test -s test-output.json
grep -q '"total": 1950.5' test-output.json
./mano run examples/clasificacion_binaria.mano >/dev/null
test -s reporte_clientes.json
test -s reporte_clientes.json.sst.json
grep -q '"salidas_binarias"' reporte_clientes.json
grep -q '"unos": 5' reporte_clientes.json
grep -q '"histograma"' reporte_clientes.json.sst.json
grep -q '"pearson"' reporte_clientes.json.sst.json
grep -q '"normalidad"' reporte_clientes.json.sst.json
./mano perfil datos/clientes_binarios.csv perfil.json >/dev/null
test -s perfil.json
printf 'OK: pruebas de ventas, esquema binario, perfil y operaciones SST completadas\n'
