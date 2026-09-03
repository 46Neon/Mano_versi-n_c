#include "analysis.h"

bool analysis_ventas(const Dataset *dataset,
                    const char *date_column,
                    const char *price_column,
                    const char *quantity_column,
                    const char *output_json) {
    if (!dataset || !date_column || !price_column || !quantity_column || !output_json) {
        fprintf(stderr, "Parámetros inválidos para analysis_ventas\n");
        return false;
    }
    
    int date_idx = dataset_indice_columna(dataset, date_column);
    int price_idx = dataset_indice_columna(dataset, price_column);
    int quantity_idx = dataset_indice_columna(dataset, quantity_column);
    
    if (date_idx < 0 || price_idx < 0 || quantity_idx < 0) {
        fprintf(stderr, "Columnas requeridas no encontradas: %s, %s, %s\n",
                date_column, price_column, quantity_column);
        return false;
    }
    
    printf("\n=== ANÁLISIS DE VENTAS ===\n");
    printf("Fecha: %s, Precio: %s, Cantidad: %s\n\n",
           date_column, price_column, quantity_column);
    
    double total_ventas = 0.0;
    size_t count = 0;
    double max_venta = 0.0;
    double min_venta = 1e9;
    
    for (size_t r = 0; r < dataset->row_count; r++) {
        double precio = atof(dataset->rows[r][price_idx]);
        double cantidad = atof(dataset->rows[r][quantity_idx]);
        double total = precio * cantidad;
        
        if (total > 0) {
            total_ventas += total;
            count++;
            if (total > max_venta) max_venta = total;
            if (total < min_venta) min_venta = total;
            
            printf("  %s: %.2f x %.0f = %.2f\n",
                   dataset->rows[r][date_idx],
                   precio, cantidad, total);
        }
    }
    
    printf("\nRESUMEN:\n");
    printf("  Total ventas: %.2f\n", total_ventas);
    printf("  Promedio: %.2f\n", count > 0 ? total_ventas / count : 0.0);
    printf("  Máxima: %.2f\n", max_venta);
    printf("  Mínima: %.2f\n", min_venta == 1e9 ? 0.0 : min_venta);
    printf("  Registros: %zu\n", count);
    
    // Exportar a JSON
    FILE *archivo = fopen(output_json, "w");
    if (archivo) {
        fprintf(archivo, "{\n");
        fprintf(archivo, "  \"analisis\": \"ventas\",\n");
        fprintf(archivo, "  \"total\": %.2f,\n", total_ventas);
        fprintf(archivo, "  \"promedio\": %.2f,\n", count > 0 ? total_ventas / count : 0.0);
        fprintf(archivo, "  \"maxima\": %.2f,\n", max_venta);
        fprintf(archivo, "  \"minima\": %.2f,\n", min_venta == 1e9 ? 0.0 : min_venta);
        fprintf(archivo, "  \"registros\": %zu\n", count);
        fprintf(archivo, "}\n");
        fclose(archivo);
        printf("\nReporte guardado en: %s\n", output_json);
    } else {
        fprintf(stderr, "No se pudo guardar el reporte en %s\n", output_json);
    }
    
    return true;
}
