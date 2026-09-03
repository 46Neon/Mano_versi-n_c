#include "dataset.h"

static char* copiar_texto(const char *texto) {
    if (!texto) return NULL;
    size_t len = strlen(texto) + 1;
    char *copy = (char *)malloc(len);
    if (copy) memcpy(copy, texto, len);
    return copy;
}

static void liberar_campos(char **campos, size_t cantidad) {
    if (!campos) return;
    for (size_t i = 0; i < cantidad; i++) {
        free(campos[i]);
    }
    free(campos);
}

static bool agregar_campo(char ***campos, size_t *cantidad, size_t *capacidad, char *campo) {
    if (!campos || !cantidad || !capacidad) return false;
    
    if (*cantidad >= *capacidad) {
        size_t nueva_capacidad = *capacidad == 0 ? 8 : *capacidad * 2;
        char **nuevos = (char **)realloc(*campos, nueva_capacidad * sizeof(char *));
        if (!nuevos) return false;
        
        *campos = nuevos;
        *capacidad = nueva_capacidad;
    }
    
    (*campos)[(*cantidad)++] = campo;
    return true;
}

static bool analizar_linea_csv(char *linea, char ***campos, size_t *cantidad) {
    if (!linea || !campos || !cantidad) return false;
    
    char **resultado = NULL;
    size_t total = 0;
    size_t capacidad = 0;
    size_t max_campo = strlen(linea) + 1;
    char *p = linea;
    
    while (true) {
        char *campo = (char *)malloc(max_campo);
        if (!campo) {
            liberar_campos(resultado, total);
            return false;
        }
        
        size_t salida = 0;
        bool entre_comillas = false;
        
        if (*p == '"') {
            entre_comillas = true;
            p++;
        }
        
        while (*p != '\0') {
            if (entre_comillas) {
                if (*p == '"' && p[1] == '"') {
                    campo[salida++] = '"';
                    p += 2;
                } else if (*p == '"') {
                    entre_comillas = false;
                    p++;
                    if (*p != ',' && *p != '\0') {
                        free(campo);
                        liberar_campos(resultado, total);
                        return false;
                    }
                    break;
                } else {
                    campo[salida++] = *p;
                    p++;
                }
            } else {
                if (*p == ',') break;
                if (*p == '"') {
                    free(campo);
                    liberar_campos(resultado, total);
                    return false;
                }
                campo[salida++] = *p;
                p++;
            }
        }
        
        if (entre_comillas) {
            free(campo);
            liberar_campos(resultado, total);
            return false;
        }
        
        campo[salida] = '\0';
        
        if (!agregar_campo(&resultado, &total, &capacidad, campo)) {
            free(campo);
            liberar_campos(resultado, total);
            return false;
        }
        
        if (*p == ',') {
            p++;
            if (*p == '\0') {
                campo = (char *)malloc(1);
                if (!campo) {
                    liberar_campos(resultado, total);
                    return false;
                }
                campo[0] = '\0';
                if (!agregar_campo(&resultado, &total, &capacidad, campo)) {
                    free(campo);
                    liberar_campos(resultado, total);
                    return false;
                }
                break;
            }
            continue;
        }
        
        if (*p == '\0') break;
        
        liberar_campos(resultado, total);
        return false;
    }
    
    *campos = resultado;
    *cantidad = total;
    return true;
}

static void quitar_fin_de_linea(char *linea) {
    if (!linea) return;
    size_t len = strlen(linea);
    while (len > 0 && (linea[len-1] == '\n' || linea[len-1] == '\r')) {
        linea[--len] = '\0';
    }
}

bool dataset_cargar_csv(Dataset *dataset, const char *filename) {
    if (!dataset || !filename) return false;
    
    memset(dataset, 0, sizeof(Dataset));
    FILE *archivo = fopen(filename, "r");
    if (!archivo) {
        fprintf(stderr, "No se pudo abrir CSV: %s\n", filename);
        return false;
    }
    
    char linea[8192];
    char **campos = NULL;
    size_t cantidad = 0;
    
    // Leer encabezado
    if (!fgets(linea, sizeof(linea), archivo)) {
        fprintf(stderr, "CSV sin encabezado\n");
        fclose(archivo);
        return false;
    }
    
    quitar_fin_de_linea(linea);
    if (!analizar_linea_csv(linea, &campos, &cantidad) || cantidad == 0) {
        fprintf(stderr, "Encabezado inválido\n");
        liberar_campos(campos, cantidad);
        fclose(archivo);
        return false;
    }
    
    dataset->filename = copiar_texto(filename);
    dataset->headers = campos;
    dataset->column_count = cantidad;
    dataset->row_count = 0;
    dataset->row_capacity = 0;
    dataset->invalid_rows = 0;
    
    // Leer datos
    while (fgets(linea, sizeof(linea), archivo)) {
        quitar_fin_de_linea(linea);
        if (linea[0] == '\0') continue;
        
        char **fila = NULL;
        size_t columnas_leidas = 0;
        
        if (!analizar_linea_csv(linea, &fila, &columnas_leidas) || 
            columnas_leidas != dataset->column_count) {
            dataset->invalid_rows++;
            liberar_campos(fila, columnas_leidas);
            continue;
        }
        
        if (dataset->row_count >= dataset->row_capacity) {
            size_t nueva_capacidad = dataset->row_capacity == 0 ? 16 : dataset->row_capacity * 2;
            char ***nuevas_filas = (char ***)realloc(dataset->rows, nueva_capacidad * sizeof(char **));
            if (!nuevas_filas) {
                liberar_campos(fila, columnas_leidas);
                fclose(archivo);
                dataset_destruir(dataset);
                return false;
            }
            dataset->rows = nuevas_filas;
            dataset->row_capacity = nueva_capacidad;
        }
        
        dataset->rows[dataset->row_count++] = fila;
    }
    
    fclose(archivo);
    return true;
}

bool dataset_cargar_json(Dataset *dataset, const char *filename) {
    (void)dataset;
    (void)filename;
    fprintf(stderr, "Carga JSON no implementada aún\n");
    return false;
}

bool dataset_guardar_json(const Dataset *dataset, const char *filename) {
    if (!dataset || !filename) return false;
    
    FILE *archivo = fopen(filename, "w");
    if (!archivo) {
        fprintf(stderr, "No se pudo escribir JSON: %s\n", filename);
        return false;
    }
    
    fprintf(archivo, "{\n");
    fprintf(archivo, "  \"dataset\": \"%s\",\n", dataset->filename ? dataset->filename : "");
    fprintf(archivo, "  \"columnas\": [");
    
    for (size_t i = 0; i < dataset->column_count; i++) {
        fprintf(archivo, "\"%s\"%s", 
                dataset->headers[i],
                i + 1 < dataset->column_count ? ", " : "");
    }
    
    fprintf(archivo, "],\n");
    fprintf(archivo, "  \"filas\": [\n");
    
    size_t max_filas = dataset->row_count < 100 ? dataset->row_count : 100;
    for (size_t r = 0; r < max_filas; r++) {
        fprintf(archivo, "    [");
        for (size_t c = 0; c < dataset->column_count; c++) {
            fprintf(archivo, "\"%s\"%s",
                    dataset->rows[r][c],
                    c + 1 < dataset->column_count ? ", " : "");
        }
        fprintf(archivo, "]%s\n", r + 1 < max_filas ? "," : "");
    }
    
    if (dataset->row_count > max_filas) {
        fprintf(archivo, "    // ... %zu filas más ...\n", dataset->row_count - max_filas);
    }
    
    fprintf(archivo, "  ]\n");
    fprintf(archivo, "}\n");
    
    fclose(archivo);
    return true;
}

void dataset_destruir(Dataset *dataset) {
    if (!dataset) return;
    
    free(dataset->filename);
    
    for (size_t i = 0; i < dataset->column_count; i++) {
        free(dataset->headers[i]);
    }
    free(dataset->headers);
    
    for (size_t r = 0; r < dataset->row_count; r++) {
        for (size_t c = 0; c < dataset->column_count; c++) {
            free(dataset->rows[r][c]);
        }
        free(dataset->rows[r]);
    }
    free(dataset->rows);
    
    memset(dataset, 0, sizeof(Dataset));
}

void dataset_imprimir(const Dataset *dataset, size_t max_rows) {
    if (!dataset) return;
    
    printf("Dataset: %s\n", dataset->filename ? dataset->filename : "sin nombre");
    printf("Columnas: %zu | Filas: %zu | Inválidas: %zu\n",
           dataset->column_count, dataset->row_count, dataset->invalid_rows);
    
    if (dataset->column_count == 0) return;
    
    for (size_t c = 0; c < dataset->column_count; c++) {
        printf("%s%s", dataset->headers[c], c + 1 < dataset->column_count ? " | " : "\n");
    }
    
    printf("%s\n", "----------------------------------------");
    
    size_t limite = dataset->row_count < max_rows ? dataset->row_count : max_rows;
    for (size_t r = 0; r < limite; r++) {
        for (size_t c = 0; c < dataset->column_count; c++) {
            printf("%s%s", dataset->rows[r][c], c + 1 < dataset->column_count ? " | " : "\n");
        }
    }
}

int dataset_indice_columna(const Dataset *dataset, const char *name) {
    if (!dataset || !name) return -1;
    
    for (size_t i = 0; i < dataset->column_count; i++) {
        if (strcmp(dataset->headers[i], name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool dataset_clean_nulls(Dataset *dataset, const char *strategy) {
    if (!dataset || !strategy) return false;
    
    size_t nuevas_filas = 0;
    for (size_t r = 0; r < dataset->row_count; r++) {
        bool tiene_nulo = false;
        for (size_t c = 0; c < dataset->column_count; c++) {
            if (dataset->rows[r][c] == NULL || dataset->rows[r][c][0] == '\0') {
                tiene_nulo = true;
                break;
            }
        }
        
        if (!tiene_nulo) {
            if (r != nuevas_filas) {
                dataset->rows[nuevas_filas] = dataset->rows[r];
            }
            nuevas_filas++;
        } else {
            for (size_t c = 0; c < dataset->column_count; c++) {
                free(dataset->rows[r][c]);
            }
            free(dataset->rows[r]);
        }
    }
    
    dataset->row_count = nuevas_filas;
    return true;
}

bool dataset_clean_duplicates(Dataset *dataset, const char *strategy) {
    (void)dataset;
    (void)strategy;
    return true;
}

bool dataset_filter_condition(Dataset *dataset, const char *condition) {
    (void)dataset;
    (void)condition;
    return true;
}

bool dataset_transform_total(Dataset *dataset, const char *expression) {
    (void)dataset;
    (void)expression;
    return true;
}

bool dataset_transform_period(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}

bool dataset_group_by(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}

bool dataset_aggregate_sum(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}

bool dataset_aggregate_avg(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}

bool dataset_aggregate_min(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}

bool dataset_aggregate_max(Dataset *dataset, const char *column) {
    (void)dataset;
    (void)column;
    return true;
}
