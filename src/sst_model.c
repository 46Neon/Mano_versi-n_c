#include "sst_model.h"

static bool equal_ci(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

void sst_event_init(SstEvent *event) {
    if (!event) return;
    memset(event, 0, sizeof(*event));
    event->tipo_riesgo = SST_RISK_UNKNOWN;
    event->capacitacion = SST_BINARY_NULL;
    event->uso_epp = SST_BINARY_NULL;
    event->ocurrio_incidente = SST_BINARY_NULL;
    event->perdio_tiempo = SST_BINARY_NULL;
}

void sst_event_destroy(SstEvent *event) {
    if (!event) return;
    free(event->id_evento);
    free(event->area);
    free(event->cargo);
    free(event->turno);
    free(event->tipo_riesgo_original);
    sst_event_init(event);
}

void sst_event_list_init(SstEventList *list) {
    if (!list) return;
    memset(list, 0, sizeof(*list));
}

void sst_event_list_destroy(SstEventList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) sst_event_destroy(&list->items[i]);
    free(list->items);
    sst_event_list_init(list);
}

static MilenaStatus event_copy(SstEvent *destination, const SstEvent *source) {
    sst_event_init(destination);
    *destination = *source;
    destination->id_evento = milena_strdup(source->id_evento);
    destination->area = milena_strdup(source->area);
    destination->cargo = milena_strdup(source->cargo);
    destination->turno = milena_strdup(source->turno);
    destination->tipo_riesgo_original = milena_strdup(source->tipo_riesgo_original);
    if ((source->id_evento && !destination->id_evento) ||
        (source->area && !destination->area) ||
        (source->cargo && !destination->cargo) ||
        (source->turno && !destination->turno) ||
        (source->tipo_riesgo_original && !destination->tipo_riesgo_original)) {
        sst_event_destroy(destination);
        return MILENA_ERR_MEMORY;
    }
    return MILENA_OK;
}

MilenaStatus sst_event_list_append(SstEventList *list, const SstEvent *event,
                                 MilenaError *error) {
    if (!list || !event) return MILENA_ERR_ARGUMENT;
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 64;
        if (next < list->capacity || next > SIZE_MAX / sizeof(*list->items)) {
            milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, list->count,
                           "La lista SST supera su capacidad");
            return MILENA_ERR_OVERFLOW;
        }
        SstEvent *tmp = (SstEvent *)realloc(list->items, next * sizeof(*tmp));
        if (!tmp) {
            milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, list->count,
                           "Sin memoria para evento SST");
            return MILENA_ERR_MEMORY;
        }
        list->items = tmp;
        list->capacity = next;
    }
    MilenaStatus status = event_copy(&list->items[list->count], event);
    if (status != MILENA_OK) {
        milena_error_set(error, status, 0, 0, list->count,
                       "No se pudo copiar evento SST");
        return status;
    }
    list->count++;
    return MILENA_OK;
}

SstRiskType sst_risk_parse(const char *text) {
    if (equal_ci(text, "caida") || equal_ci(text, "caída")) return SST_RISK_FALL;
    if (equal_ci(text, "quimico") || equal_ci(text, "químico")) return SST_RISK_CHEMICAL;
    if (equal_ci(text, "electrico") || equal_ci(text, "eléctrico")) return SST_RISK_ELECTRICAL;
    if (equal_ci(text, "corte")) return SST_RISK_CUT;
    if (equal_ci(text, "ruido")) return SST_RISK_NOISE;
    if (equal_ci(text, "ergonomico") || equal_ci(text, "ergonómico")) return SST_RISK_ERGONOMIC;
    if (equal_ci(text, "biologico") || equal_ci(text, "biológico")) return SST_RISK_BIOLOGICAL;
    return SST_RISK_UNKNOWN;
}

const char *sst_risk_name(SstRiskType risk) {
    switch (risk) {
        case SST_RISK_FALL: return "caida";
        case SST_RISK_CHEMICAL: return "quimico";
        case SST_RISK_ELECTRICAL: return "electrico";
        case SST_RISK_CUT: return "corte";
        case SST_RISK_NOISE: return "ruido";
        case SST_RISK_ERGONOMIC: return "ergonomico";
        case SST_RISK_BIOLOGICAL: return "biologico";
        default: return "desconocido";
    }
}

SstBinaryValue sst_binary_parse(const char *text) {
    if (!text || text[0] == '\0') return SST_BINARY_NULL;
    if (equal_ci(text, "1") || equal_ci(text, "true") ||
        equal_ci(text, "verdadero") || equal_ci(text, "yes") || equal_ci(text, "si")) {
        return SST_BINARY_TRUE;
    }
    if (equal_ci(text, "0") || equal_ci(text, "false") ||
        equal_ci(text, "falso") || equal_ci(text, "no")) {
        return SST_BINARY_FALSE;
    }
    return SST_BINARY_INVALID;
}

const char *sst_binary_name(SstBinaryValue value) {
    switch (value) {
        case SST_BINARY_FALSE: return "false";
        case SST_BINARY_TRUE: return "true";
        case SST_BINARY_NULL: return "null";
        case SST_BINARY_INVALID: return "invalid";
        default: return "unknown";
    }
}
