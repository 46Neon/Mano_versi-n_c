#ifndef MILENA_SST_MODEL_H
#define MILENA_SST_MODEL_H

#include "common.h"
#include "sst_dates.h"

typedef enum {
    SST_RISK_UNKNOWN,
    SST_RISK_FALL,
    SST_RISK_CHEMICAL,
    SST_RISK_ELECTRICAL,
    SST_RISK_CUT,
    SST_RISK_NOISE,
    SST_RISK_ERGONOMIC,
    SST_RISK_BIOLOGICAL
} SstRiskType;

typedef enum {
    SST_BINARY_FALSE = 0,
    SST_BINARY_TRUE = 1,
    SST_BINARY_NULL = 2,
    SST_BINARY_INVALID = 3
} SstBinaryValue;

typedef struct {
    char *id_evento;
    MilenaDate fecha;
    char *area;
    char *cargo;
    char *turno;
    char *tipo_riesgo_original;
    SstRiskType tipo_riesgo;
    double severidad;
    double dias_incapacidad;
    double horas_exposicion;
    bool has_severidad;
    bool has_dias_incapacidad;
    bool has_horas_exposicion;
    SstBinaryValue capacitacion;
    SstBinaryValue uso_epp;
    SstBinaryValue ocurrio_incidente;
    SstBinaryValue perdio_tiempo;
} SstEvent;

typedef struct {
    SstEvent *items;
    size_t count;
    size_t capacity;
} SstEventList;

void sst_event_init(SstEvent *event);
void sst_event_destroy(SstEvent *event);
void sst_event_list_init(SstEventList *list);
void sst_event_list_destroy(SstEventList *list);
MilenaStatus sst_event_list_append(SstEventList *list, const SstEvent *event,
                                 MilenaError *error);
SstRiskType sst_risk_parse(const char *text);
const char *sst_risk_name(SstRiskType risk);
SstBinaryValue sst_binary_parse(const char *text);
const char *sst_binary_name(SstBinaryValue value);

#endif
