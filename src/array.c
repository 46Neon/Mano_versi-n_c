#include "array.h"

#include <complex.h>
#include <inttypes.h>

struct MilenaArrayStorage {
    unsigned char *data;
    size_t nbytes;
    size_t references;
};

static void array_error(MilenaError *error, MilenaStatus code,
                        const char *message) {
    if (error) milena_error_set(error, code, 0, 0, 0, message);
}

const char *milena_dtype_name(MilenaDType dtype) {
    switch (dtype) {
        case MILENA_DTYPE_BOOL: return "bool";
        case MILENA_DTYPE_INT8: return "int8";
        case MILENA_DTYPE_INT16: return "int16";
        case MILENA_DTYPE_INT32: return "int32";
        case MILENA_DTYPE_INT64: return "int64";
        case MILENA_DTYPE_UINT8: return "uint8";
        case MILENA_DTYPE_UINT16: return "uint16";
        case MILENA_DTYPE_UINT32: return "uint32";
        case MILENA_DTYPE_UINT64: return "uint64";
        case MILENA_DTYPE_FLOAT32: return "float32";
        case MILENA_DTYPE_FLOAT64: return "float64";
        case MILENA_DTYPE_COMPLEX64: return "complex64";
        case MILENA_DTYPE_COMPLEX128: return "complex128";
        default: return "unknown";
    }
}

size_t milena_dtype_size(MilenaDType dtype) {
    switch (dtype) {
        case MILENA_DTYPE_BOOL: return sizeof(bool);
        case MILENA_DTYPE_INT8: return sizeof(int8_t);
        case MILENA_DTYPE_INT16: return sizeof(int16_t);
        case MILENA_DTYPE_INT32: return sizeof(int32_t);
        case MILENA_DTYPE_INT64: return sizeof(int64_t);
        case MILENA_DTYPE_UINT8: return sizeof(uint8_t);
        case MILENA_DTYPE_UINT16: return sizeof(uint16_t);
        case MILENA_DTYPE_UINT32: return sizeof(uint32_t);
        case MILENA_DTYPE_UINT64: return sizeof(uint64_t);
        case MILENA_DTYPE_FLOAT32: return sizeof(float);
        case MILENA_DTYPE_FLOAT64: return sizeof(double);
        case MILENA_DTYPE_COMPLEX64: return sizeof(float complex);
        case MILENA_DTYPE_COMPLEX128: return sizeof(double complex);
        default: return 0;
    }
}

static bool valid_dtype(MilenaDType dtype) {
    return milena_dtype_size(dtype) != 0;
}

static MilenaStatus array_size(size_t ndim, const size_t *shape,
                               size_t *size, MilenaError *error) {
    if (ndim > 0 && !shape) {
        array_error(error, MILENA_ERR_ARGUMENT, "La forma requiere un arreglo de dimensiones");
        return MILENA_ERR_ARGUMENT;
    }

    size_t result = 1;
    for (size_t i = 0; i < ndim; i++) {
        if (!milena_size_mul(result, shape[i], &result)) {
            array_error(error, MILENA_ERR_OVERFLOW, "La forma del array desborda size_t");
            return MILENA_ERR_OVERFLOW;
        }
    }
    *size = result;
    return MILENA_OK;
}

static MilenaStatus make_contiguous_strides(size_t ndim, size_t itemsize,
                                            const size_t *shape,
                                            ptrdiff_t *strides,
                                            MilenaError *error) {
    if (ndim == 0) return MILENA_OK;
    strides[ndim - 1] = (ptrdiff_t)itemsize;
    for (size_t i = ndim - 1; i > 0; i--) {
        size_t stride = 0;
        if (!milena_size_mul((size_t)strides[i], shape[i], &stride) ||
            stride > (size_t)PTRDIFF_MAX) {
            array_error(error, MILENA_ERR_OVERFLOW, "Los strides del array desbordan ptrdiff_t");
            return MILENA_ERR_OVERFLOW;
        }
        strides[i - 1] = (ptrdiff_t)stride;
    }
    return MILENA_OK;
}

static MilenaStatus allocate_array(MilenaArray *out, MilenaDType dtype,
                                   size_t ndim, const size_t *shape,
                                   MilenaError *error) {
    if (!out || !valid_dtype(dtype)) {
        array_error(error, MILENA_ERR_ARGUMENT, "Argumentos inválidos al crear el array");
        return MILENA_ERR_ARGUMENT;
    }

    size_t size = 0;
    MilenaStatus status = array_size(ndim, shape, &size, error);
    if (status != MILENA_OK) return status;

    size_t itemsize = milena_dtype_size(dtype);
    size_t nbytes = 0;
    if (!milena_size_mul(size, itemsize, &nbytes)) {
        array_error(error, MILENA_ERR_OVERFLOW, "El buffer del array desborda size_t");
        return MILENA_ERR_OVERFLOW;
    }

    MilenaArrayStorage *storage = (MilenaArrayStorage *)calloc(1, sizeof(*storage));
    if (!storage) {
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar el almacenamiento del array");
        return MILENA_ERR_MEMORY;
    }
    storage->references = 1;
    storage->nbytes = nbytes;
    if (nbytes > 0) {
        storage->data = (unsigned char *)calloc(1, nbytes);
        if (!storage->data) {
            free(storage);
            array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar el buffer del array");
            return MILENA_ERR_MEMORY;
        }
    }

    out->shape = ndim > 0 ? (size_t *)calloc(ndim, sizeof(size_t)) : NULL;
    out->strides = ndim > 0 ? (ptrdiff_t *)calloc(ndim, sizeof(ptrdiff_t)) : NULL;
    if ((ndim > 0) && (!out->shape || !out->strides)) {
        free(out->shape);
        free(out->strides);
        free(storage->data);
        free(storage);
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la metadata del array");
        return MILENA_ERR_MEMORY;
    }

    if (ndim > 0) memcpy(out->shape, shape, ndim * sizeof(size_t));
    status = make_contiguous_strides(ndim, itemsize, shape, out->strides, error);
    if (status != MILENA_OK) {
        free(out->shape);
        free(out->strides);
        free(storage->data);
        free(storage);
        return status;
    }

    out->storage = storage;
    out->dtype = dtype;
    out->ndim = ndim;
    out->itemsize = itemsize;
    out->size = size;
    out->byte_offset = 0;
    out->flags = MILENA_ARRAY_OWN_DATA;
    return MILENA_OK;
}

MilenaStatus milena_array_zeros(MilenaArray *out, MilenaDType dtype,
                                size_t ndim, const size_t *shape,
                                MilenaError *error) {
    return allocate_array(out, dtype, ndim, shape, error);
}

MilenaStatus milena_array_from_f64(MilenaArray *out, size_t ndim,
                                   const size_t *shape, const double *values,
                                   MilenaError *error) {
    MilenaStatus status = allocate_array(out, MILENA_DTYPE_FLOAT64, ndim,
                                         shape, error);
    if (status != MILENA_OK) return status;
    if (out->size > 0 && !values) {
        milena_array_release(out);
        array_error(error, MILENA_ERR_ARGUMENT, "Faltan valores para inicializar el array");
        return MILENA_ERR_ARGUMENT;
    }
    if (out->size > 0) {
        memcpy(out->storage->data + out->byte_offset, values,
               out->size * sizeof(double));
    }
    return MILENA_OK;
}

MilenaStatus milena_array_from_i64(MilenaArray *out, size_t ndim,
                                   const size_t *shape, const int64_t *values,
                                   MilenaError *error) {
    MilenaStatus status = allocate_array(out, MILENA_DTYPE_INT64, ndim,
                                         shape, error);
    if (status != MILENA_OK) return status;
    if (out->size > 0 && !values) {
        milena_array_release(out);
        array_error(error, MILENA_ERR_ARGUMENT, "Faltan valores int64 para inicializar el array");
        return MILENA_ERR_ARGUMENT;
    }
    if (out->size > 0) {
        memcpy(out->storage->data + out->byte_offset, values,
               out->size * sizeof(int64_t));
    }
    return MILENA_OK;
}

void milena_array_retain(MilenaArray *array) {
    if (array && array->storage) array->storage->references++;
}

void milena_array_release(MilenaArray *array) {
    if (!array) return;
    if (array->storage && array->storage->references > 0) {
        array->storage->references--;
        if (array->storage->references == 0) {
            free(array->storage->data);
            free(array->storage);
        }
    }
    free(array->shape);
    free(array->strides);
    memset(array, 0, sizeof(*array));
}

bool milena_array_is_contiguous(const MilenaArray *array) {
    if (!array) return false;
    if (array->ndim == 0) return true;
    ptrdiff_t expected = (ptrdiff_t)array->itemsize;
    for (size_t i = array->ndim; i > 0; i--) {
        size_t axis = i - 1;
        if (array->strides[axis] != expected) return false;
        if (array->shape[axis] > 0 &&
            (size_t)expected > SIZE_MAX / array->shape[axis]) return false;
        expected *= (ptrdiff_t)array->shape[axis];
    }
    return true;
}

void *milena_array_data(MilenaArray *array) {
    if (!array || !array->storage) return NULL;
    return array->storage->data + array->byte_offset;
}

const void *milena_array_const_data(const MilenaArray *array) {
    if (!array || !array->storage) return NULL;
    return array->storage->data + array->byte_offset;
}

MilenaStatus milena_array_slice_view(MilenaArray *out,
                                     const MilenaArray *source,
                                     size_t axis, size_t start, size_t stop,
                                     size_t step, MilenaError *error) {
    if (!out || !source || !source->storage || out == source) {
        array_error(error, MILENA_ERR_ARGUMENT, "Array inválido para crear una vista slice");
        return MILENA_ERR_ARGUMENT;
    }
    if (axis >= source->ndim) {
        array_error(error, MILENA_ERR_ARGUMENT, "Eje fuera de rango para slice");
        return MILENA_ERR_ARGUMENT;
    }
    if (step == 0) {
        array_error(error, MILENA_ERR_ARGUMENT, "El paso de slice no puede ser cero");
        return MILENA_ERR_ARGUMENT;
    }
    if (start > stop || stop > source->shape[axis]) {
        array_error(error, MILENA_ERR_ARGUMENT, "Los límites de slice están fuera de rango");
        return MILENA_ERR_ARGUMENT;
    }
    if (source->strides[axis] < 0) {
        array_error(error, MILENA_ERR_UNSUPPORTED, "Slice aún no admite strides negativos");
        return MILENA_ERR_UNSUPPORTED;
    }

    size_t *new_shape = (size_t *)calloc(source->ndim, sizeof(size_t));
    ptrdiff_t *new_strides = (ptrdiff_t *)calloc(source->ndim, sizeof(ptrdiff_t));
    if (!new_shape || !new_strides) {
        free(new_shape);
        free(new_strides);
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la metadata del slice");
        return MILENA_ERR_MEMORY;
    }
    memcpy(new_shape, source->shape, source->ndim * sizeof(size_t));
    memcpy(new_strides, source->strides, source->ndim * sizeof(ptrdiff_t));

    size_t selected = 0;
    if (stop > start) selected = 1 + (stop - 1 - start) / step;
    new_shape[axis] = selected;

    size_t byte_delta = 0;
    if (!milena_size_mul(start, (size_t)source->strides[axis], &byte_delta) ||
        byte_delta > SIZE_MAX - source->byte_offset) {
        free(new_shape);
        free(new_strides);
        array_error(error, MILENA_ERR_OVERFLOW, "El offset de slice desborda size_t");
        return MILENA_ERR_OVERFLOW;
    }
    size_t new_offset = source->byte_offset + byte_delta;
    if (new_offset > source->storage->nbytes ||
        (selected > 0 && source->storage->nbytes - new_offset < source->itemsize)) {
        free(new_shape);
        free(new_strides);
        array_error(error, MILENA_ERR_ARGUMENT, "El slice apunta fuera del buffer");
        return MILENA_ERR_ARGUMENT;
    }

    size_t new_stride = 0;
    if (!milena_size_mul((size_t)new_strides[axis], step, &new_stride) ||
        new_stride > (size_t)PTRDIFF_MAX) {
        free(new_shape);
        free(new_strides);
        array_error(error, MILENA_ERR_OVERFLOW, "El stride de slice desborda ptrdiff_t");
        return MILENA_ERR_OVERFLOW;
    }
    new_strides[axis] = (ptrdiff_t)new_stride;

    *out = *source;
    out->shape = new_shape;
    out->strides = new_strides;
    out->size = 1;
    for (size_t i = 0; i < out->ndim; i++) {
        if (!milena_size_mul(out->size, out->shape[i], &out->size)) {
            free(out->shape);
            free(out->strides);
            memset(out, 0, sizeof(*out));
            array_error(error, MILENA_ERR_OVERFLOW, "El tamaño del slice desborda size_t");
            return MILENA_ERR_OVERFLOW;
        }
    }
    out->byte_offset = new_offset;
    out->flags = source->flags & ~MILENA_ARRAY_OWN_DATA;
    milena_array_retain(out);
    return MILENA_OK;
}

MilenaStatus milena_array_transpose_view(MilenaArray *out,
                                         const MilenaArray *source,
                                         const size_t *axes, MilenaError *error) {
    if (!out || !source || !source->storage || out == source) {
        array_error(error, MILENA_ERR_ARGUMENT, "Array inválido para transpose");
        return MILENA_ERR_ARGUMENT;
    }
    size_t *new_shape = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    ptrdiff_t *new_strides = source->ndim > 0 ?
        (ptrdiff_t *)calloc(source->ndim, sizeof(ptrdiff_t)) : NULL;
    bool *used = source->ndim > 0 ?
        (bool *)calloc(source->ndim, sizeof(bool)) : NULL;
    if (source->ndim > 0 && (!new_shape || !new_strides || !used)) {
        free(new_shape);
        free(new_strides);
        free(used);
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar metadata de transpose");
        return MILENA_ERR_MEMORY;
    }

    for (size_t axis = 0; axis < source->ndim; axis++) {
        size_t source_axis = axes ? axes[axis] : source->ndim - 1 - axis;
        if (source_axis >= source->ndim || used[source_axis]) {
            free(new_shape);
            free(new_strides);
            free(used);
            array_error(error, MILENA_ERR_ARGUMENT, "Los ejes de transpose no forman una permutación");
            return MILENA_ERR_ARGUMENT;
        }
        used[source_axis] = true;
        new_shape[axis] = source->shape[source_axis];
        new_strides[axis] = source->strides[source_axis];
    }
    free(used);

    *out = *source;
    out->shape = new_shape;
    out->strides = new_strides;
    out->flags = source->flags & ~MILENA_ARRAY_OWN_DATA;
    milena_array_retain(out);
    return MILENA_OK;
}

static bool same_size(size_t left, size_t right) {
    return left == right;
}

MilenaStatus milena_array_reshape_view(MilenaArray *out,
                                        const MilenaArray *source,
                                        size_t ndim, const size_t *shape,
                                        MilenaError *error) {
    if (!out || !source || !source->storage || !milena_array_is_contiguous(source)) {
        array_error(error, MILENA_ERR_UNSUPPORTED, "Solo se pueden reordenar arrays contiguos en esta etapa");
        return MILENA_ERR_UNSUPPORTED;
    }

    size_t requested_size = 0;
    MilenaStatus status = array_size(ndim, shape, &requested_size, error);
    if (status != MILENA_OK) return status;
    if (!same_size(requested_size, source->size)) {
        array_error(error, MILENA_ERR_ARGUMENT, "reshape no conserva el número de elementos");
        return MILENA_ERR_ARGUMENT;
    }

    size_t *new_shape = ndim > 0 ? (size_t *)calloc(ndim, sizeof(size_t)) : NULL;
    ptrdiff_t *new_strides = ndim > 0 ? (ptrdiff_t *)calloc(ndim, sizeof(ptrdiff_t)) : NULL;
    if ((ndim > 0) && (!new_shape || !new_strides)) {
        free(new_shape);
        free(new_strides);
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la forma de la vista");
        return MILENA_ERR_MEMORY;
    }
    if (ndim > 0) memcpy(new_shape, shape, ndim * sizeof(size_t));
    status = make_contiguous_strides(ndim, source->itemsize, shape, new_strides, error);
    if (status != MILENA_OK) {
        free(new_shape);
        free(new_strides);
        return status;
    }

    *out = *source;
    out->shape = new_shape;
    out->strides = new_strides;
    out->ndim = ndim;
    out->flags = source->flags & ~MILENA_ARRAY_OWN_DATA;
    milena_array_retain(out);
    return MILENA_OK;
}

static size_t shape_dimension(const MilenaArray *array, size_t output_axis,
                              size_t output_ndim) {
    if (output_axis < output_ndim - array->ndim) return 1;
    return array->shape[output_axis - (output_ndim - array->ndim)];
}

static ptrdiff_t element_offset(const MilenaArray *array,
                                const size_t *coordinates,
                                size_t output_ndim) {
    size_t missing = output_ndim - array->ndim;
    ptrdiff_t offset = (ptrdiff_t)array->byte_offset;
    for (size_t axis = 0; axis < array->ndim; axis++) {
        size_t output_axis = axis + missing;
        size_t coordinate = array->shape[axis] == 1 ? 0 : coordinates[output_axis];
        offset += (ptrdiff_t)coordinate * array->strides[axis];
    }
    return offset;
}

static void linear_coordinates(const MilenaArray *array, size_t index,
                               size_t *coordinates) {
    size_t remaining = index;
    for (size_t axis = array->ndim; axis > 0; axis--) {
        size_t current = axis - 1;
        coordinates[current] = array->shape[current] == 0 ? 0 :
            remaining % array->shape[current];
        if (array->shape[current] > 0) remaining /= array->shape[current];
    }
}

static bool same_shape(const MilenaArray *left, const MilenaArray *right) {
    if (!left || !right || left->ndim != right->ndim) return false;
    for (size_t axis = 0; axis < left->ndim; axis++) {
        if (left->shape[axis] != right->shape[axis]) return false;
    }
    return true;
}

static ptrdiff_t linear_offset(const MilenaArray *array, size_t index,
                              size_t *coordinates) {
    linear_coordinates(array, index, coordinates);
    return element_offset(array, coordinates, array->ndim);
}

MilenaStatus milena_array_reshape_copy(MilenaArray *out,
                                       const MilenaArray *source,
                                       size_t ndim, const size_t *shape,
                                       MilenaError *error) {
    if (!out || !source || !source->storage || out == source) {
        array_error(error, MILENA_ERR_ARGUMENT, "Array inválido para reshape copy");
        return MILENA_ERR_ARGUMENT;
    }
    size_t requested_size = 0;
    MilenaStatus status = array_size(ndim, shape, &requested_size, error);
    if (status != MILENA_OK) return status;
    if (requested_size != source->size) {
        array_error(error, MILENA_ERR_ARGUMENT, "reshape copy no conserva el número de elementos");
        return MILENA_ERR_ARGUMENT;
    }
    status = allocate_array(out, source->dtype, ndim, shape, error);
    if (status != MILENA_OK) return status;

    size_t *coordinates = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    if (source->ndim > 0 && !coordinates) {
        milena_array_release(out);
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de reshape copy");
        return MILENA_ERR_MEMORY;
    }
    for (size_t i = 0; i < source->size; i++) {
        ptrdiff_t offset = linear_offset(source, i, coordinates);
        memcpy(out->storage->data + i * out->itemsize,
               source->storage->data + offset, out->itemsize);
    }
    free(coordinates);
    return MILENA_OK;
}

MilenaStatus milena_array_greater_f64(MilenaArray *out,
                                      const MilenaArray *source,
                                      double threshold, MilenaError *error) {
    if (!out || !source || !source->storage || out == source ||
        source->dtype != MILENA_DTYPE_FLOAT64) {
        array_error(error, MILENA_ERR_TYPE, "greater_f64 requiere un array float64");
        return MILENA_ERR_TYPE;
    }
    MilenaStatus status = allocate_array(out, MILENA_DTYPE_BOOL,
                                         source->ndim, source->shape, error);
    if (status != MILENA_OK) return status;
    size_t *coordinates = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    if (source->ndim > 0 && !coordinates) {
        milena_array_release(out);
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de comparación");
        return MILENA_ERR_MEMORY;
    }
    bool *output_data = (bool *)out->storage->data;
    for (size_t i = 0; i < source->size; i++) {
        ptrdiff_t offset = linear_offset(source, i, coordinates);
        output_data[i] = *(const double *)(source->storage->data + offset) > threshold;
    }
    free(coordinates);
    return MILENA_OK;
}

MilenaStatus milena_array_boolean_mask(MilenaArray *out,
                                       const MilenaArray *source,
                                       const MilenaArray *mask,
                                       MilenaError *error) {
    if (!out || !source || !mask || !source->storage || !mask->storage ||
        out == source || out == mask || mask->dtype != MILENA_DTYPE_BOOL ||
        !same_shape(source, mask)) {
        array_error(error, MILENA_ERR_ARGUMENT, "La máscara debe ser bool y tener la forma del array");
        return MILENA_ERR_ARGUMENT;
    }
    size_t *coordinates = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    if (source->ndim > 0 && !coordinates) {
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de máscara");
        return MILENA_ERR_MEMORY;
    }
    size_t selected = 0;
    for (size_t i = 0; i < mask->size; i++) {
        ptrdiff_t offset = linear_offset(mask, i, coordinates);
        if (*(const bool *)(mask->storage->data + offset)) selected++;
    }
    size_t output_shape[] = {selected};
    MilenaStatus status = allocate_array(out, source->dtype, 1, output_shape, error);
    if (status != MILENA_OK) {
        free(coordinates);
        return status;
    }
    unsigned char *output_data = out->storage->data;
    size_t output_index = 0;
    for (size_t i = 0; i < source->size; i++) {
        ptrdiff_t mask_offset = linear_offset(mask, i, coordinates);
        if (!*(const bool *)(mask->storage->data + mask_offset)) continue;
        ptrdiff_t source_offset = linear_offset(source, i, coordinates);
        memcpy(output_data + output_index * out->itemsize,
               source->storage->data + source_offset, out->itemsize);
        output_index++;
    }
    free(coordinates);
    return MILENA_OK;
}

MilenaStatus milena_array_nonzero(MilenaArray *out, const MilenaArray *mask,
                                  MilenaError *error) {
    if (!out || !mask || !mask->storage || out == mask ||
        mask->dtype != MILENA_DTYPE_BOOL) {
        array_error(error, MILENA_ERR_TYPE, "nonzero requiere una máscara bool");
        return MILENA_ERR_TYPE;
    }
    size_t *coordinates = mask->ndim > 0 ?
        (size_t *)calloc(mask->ndim, sizeof(size_t)) : NULL;
    if (mask->ndim > 0 && !coordinates) {
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de nonzero");
        return MILENA_ERR_MEMORY;
    }
    size_t selected = 0;
    for (size_t i = 0; i < mask->size; i++) {
        ptrdiff_t offset = linear_offset(mask, i, coordinates);
        if (*(const bool *)(mask->storage->data + offset)) selected++;
    }
    size_t output_shape[] = {selected};
    MilenaStatus status = allocate_array(out, MILENA_DTYPE_INT64, 1,
                                         output_shape, error);
    if (status != MILENA_OK) {
        free(coordinates);
        return status;
    }
    int64_t *output_data = (int64_t *)out->storage->data;
    size_t output_index = 0;
    for (size_t i = 0; i < mask->size; i++) {
        ptrdiff_t offset = linear_offset(mask, i, coordinates);
        if (*(const bool *)(mask->storage->data + offset)) {
            if (i > (size_t)INT64_MAX) {
                free(coordinates);
                milena_array_release(out);
                array_error(error, MILENA_ERR_OVERFLOW, "El índice no cabe en int64");
                return MILENA_ERR_OVERFLOW;
            }
            output_data[output_index++] = (int64_t)i;
        }
    }
    free(coordinates);
    return MILENA_OK;
}

MilenaStatus milena_array_where(MilenaArray *out,
                                const MilenaArray *condition,
                                const MilenaArray *when_true,
                                const MilenaArray *when_false,
                                MilenaError *error) {
    if (!out || !condition || !when_true || !when_false ||
        !condition->storage || !when_true->storage || !when_false->storage ||
        out == condition || out == when_true || out == when_false ||
        condition->dtype != MILENA_DTYPE_BOOL ||
        !same_shape(condition, when_true) || !same_shape(when_true, when_false)) {
        array_error(error, MILENA_ERR_ARGUMENT, "where requiere tres arrays de la misma forma");
        return MILENA_ERR_ARGUMENT;
    }
    MilenaStatus status = allocate_array(out, when_true->dtype,
                                         when_true->ndim, when_true->shape, error);
    if (status != MILENA_OK) return status;
    size_t *coordinates = when_true->ndim > 0 ?
        (size_t *)calloc(when_true->ndim, sizeof(size_t)) : NULL;
    if (when_true->ndim > 0 && !coordinates) {
        milena_array_release(out);
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de where");
        return MILENA_ERR_MEMORY;
    }
    for (size_t i = 0; i < when_true->size; i++) {
        ptrdiff_t condition_offset = linear_offset(condition, i, coordinates);
        ptrdiff_t true_offset = linear_offset(when_true, i, coordinates);
        ptrdiff_t false_offset = linear_offset(when_false, i, coordinates);
        const unsigned char *selected = *(const bool *)(condition->storage->data + condition_offset) ?
            when_true->storage->data + true_offset :
            when_false->storage->data + false_offset;
        memcpy(out->storage->data + i * out->itemsize, selected, out->itemsize);
    }
    free(coordinates);
    return MILENA_OK;
}

static bool array_read_real(const MilenaArray *source, size_t index,
                            long double *value) {
    size_t *coordinates = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    if (source->ndim > 0 && !coordinates) return false;
    size_t remaining = index;
    for (size_t axis = source->ndim; axis > 0; axis--) {
        size_t current = axis - 1;
        coordinates[current] = source->shape[current] == 0 ? 0 :
            remaining % source->shape[current];
        if (source->shape[current] > 0) remaining /= source->shape[current];
    }
    ptrdiff_t offset = element_offset(source, coordinates, source->ndim);
    const unsigned char *data = source->storage->data + offset;
    switch (source->dtype) {
        case MILENA_DTYPE_BOOL: *value = *(const bool *)data ? 1.0L : 0.0L; break;
        case MILENA_DTYPE_INT8: *value = (long double)*(const int8_t *)data; break;
        case MILENA_DTYPE_INT16: *value = (long double)*(const int16_t *)data; break;
        case MILENA_DTYPE_INT32: *value = (long double)*(const int32_t *)data; break;
        case MILENA_DTYPE_INT64: *value = (long double)*(const int64_t *)data; break;
        case MILENA_DTYPE_UINT8: *value = (long double)*(const uint8_t *)data; break;
        case MILENA_DTYPE_UINT16: *value = (long double)*(const uint16_t *)data; break;
        case MILENA_DTYPE_UINT32: *value = (long double)*(const uint32_t *)data; break;
        case MILENA_DTYPE_UINT64: *value = (long double)*(const uint64_t *)data; break;
        case MILENA_DTYPE_FLOAT32: *value = (long double)*(const float *)data; break;
        case MILENA_DTYPE_FLOAT64: *value = (long double)*(const double *)data; break;
        default:
            free(coordinates);
            return false;
    }
    free(coordinates);
    return true;
}

static MilenaStatus cast_real_value(void *destination, MilenaDType dtype,
                                    long double value, MilenaError *error) {
    if (dtype != MILENA_DTYPE_BOOL && !isfinite(value)) {
        array_error(error, MILENA_ERR_TYPE, "No se puede convertir NaN o infinito a un tipo entero");
        return MILENA_ERR_TYPE;
    }
    switch (dtype) {
        case MILENA_DTYPE_BOOL: *(bool *)destination = value != 0.0L; break;
        case MILENA_DTYPE_INT8:
            if (value < (long double)INT8_MIN || value > (long double)INT8_MAX) goto range_error;
            *(int8_t *)destination = (int8_t)value; break;
        case MILENA_DTYPE_INT16:
            if (value < (long double)INT16_MIN || value > (long double)INT16_MAX) goto range_error;
            *(int16_t *)destination = (int16_t)value; break;
        case MILENA_DTYPE_INT32:
            if (value < (long double)INT32_MIN || value > (long double)INT32_MAX) goto range_error;
            *(int32_t *)destination = (int32_t)value; break;
        case MILENA_DTYPE_INT64:
            if (value < (long double)INT64_MIN || value > (long double)INT64_MAX) goto range_error;
            *(int64_t *)destination = (int64_t)value; break;
        case MILENA_DTYPE_UINT8:
            if (value < 0.0L || value > (long double)UINT8_MAX) goto range_error;
            *(uint8_t *)destination = (uint8_t)value; break;
        case MILENA_DTYPE_UINT16:
            if (value < 0.0L || value > (long double)UINT16_MAX) goto range_error;
            *(uint16_t *)destination = (uint16_t)value; break;
        case MILENA_DTYPE_UINT32:
            if (value < 0.0L || value > (long double)UINT32_MAX) goto range_error;
            *(uint32_t *)destination = (uint32_t)value; break;
        case MILENA_DTYPE_UINT64:
            if (value < 0.0L || value > (long double)UINT64_MAX) goto range_error;
            *(uint64_t *)destination = (uint64_t)value; break;
        case MILENA_DTYPE_FLOAT32: *(float *)destination = (float)value; break;
        case MILENA_DTYPE_FLOAT64: *(double *)destination = (double)value; break;
        default:
            array_error(error, MILENA_ERR_UNSUPPORTED, "Conversión de complex aún no implementada");
            return MILENA_ERR_UNSUPPORTED;
    }
    return MILENA_OK;

range_error:
    array_error(error, MILENA_ERR_OVERFLOW, "El valor no cabe en el dtype solicitado");
    return MILENA_ERR_OVERFLOW;
}

MilenaStatus milena_array_cast(MilenaArray *out, const MilenaArray *source,
                               MilenaDType dtype, MilenaError *error) {
    if (!out || !source || !source->storage || !valid_dtype(dtype)) {
        array_error(error, MILENA_ERR_ARGUMENT, "Array inválido para conversión de dtype");
        return MILENA_ERR_ARGUMENT;
    }
    if (source->dtype == MILENA_DTYPE_COMPLEX64 ||
        source->dtype == MILENA_DTYPE_COMPLEX128 ||
        dtype == MILENA_DTYPE_COMPLEX64 || dtype == MILENA_DTYPE_COMPLEX128) {
        array_error(error, MILENA_ERR_UNSUPPORTED, "Conversión de complex aún no implementada");
        return MILENA_ERR_UNSUPPORTED;
    }
    MilenaStatus status = allocate_array(out, dtype, source->ndim,
                                         source->shape, error);
    if (status != MILENA_OK) return status;
    for (size_t i = 0; i < source->size; i++) {
        long double value = 0.0L;
        if (!array_read_real(source, i, &value)) {
            milena_array_release(out);
            array_error(error, MILENA_ERR_MEMORY, "No se pudo leer el valor del array");
            return MILENA_ERR_MEMORY;
        }
        status = cast_real_value((unsigned char *)out->storage->data +
                                     i * out->itemsize,
                                 dtype, value, error);
        if (status != MILENA_OK) {
            milena_array_release(out);
            return status;
        }
    }
    return MILENA_OK;
}

static MilenaStatus broadcast_shape(size_t *output_ndim, size_t **output_shape,
                                    const MilenaArray *left,
                                    const MilenaArray *right,
                                    MilenaError *error) {
    size_t ndim = left->ndim > right->ndim ? left->ndim : right->ndim;
    size_t *shape = ndim > 0 ? (size_t *)calloc(ndim, sizeof(size_t)) : NULL;
    if (ndim > 0 && !shape) {
        array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la forma broadcast");
        return MILENA_ERR_MEMORY;
    }
    for (size_t axis = 0; axis < ndim; axis++) {
        size_t left_dim = shape_dimension(left, axis, ndim);
        size_t right_dim = shape_dimension(right, axis, ndim);
        if (left_dim != right_dim && left_dim != 1 && right_dim != 1) {
            free(shape);
            array_error(error, MILENA_ERR_ARGUMENT, "Las formas no son compatibles para broadcasting");
            return MILENA_ERR_ARGUMENT;
        }
        shape[axis] = left_dim > right_dim ? left_dim : right_dim;
    }
    *output_ndim = ndim;
    *output_shape = shape;
    return MILENA_OK;
}

static void increment_coordinates(size_t *coordinates, size_t ndim,
                                  const size_t *shape) {
    for (size_t axis = ndim; axis > 0; axis--) {
        size_t index = axis - 1;
        coordinates[index]++;
        if (coordinates[index] < shape[index]) return;
        coordinates[index] = 0;
    }
}

MilenaStatus milena_array_add(MilenaArray *out, const MilenaArray *left,
                              const MilenaArray *right, MilenaError *error) {
    if (!out || !left || !right || !left->storage || !right->storage ||
        left->dtype != right->dtype) {
        array_error(error, MILENA_ERR_TYPE, "La suma requiere arrays válidos del mismo dtype");
        return MILENA_ERR_TYPE;
    }
    if (left->dtype != MILENA_DTYPE_FLOAT64 && left->dtype != MILENA_DTYPE_INT64) {
        array_error(error, MILENA_ERR_UNSUPPORTED, "La suma aún no está implementada para este dtype");
        return MILENA_ERR_UNSUPPORTED;
    }

    size_t ndim = 0;
    size_t *shape = NULL;
    MilenaStatus status = broadcast_shape(&ndim, &shape, left, right, error);
    if (status != MILENA_OK) return status;
    status = allocate_array(out, left->dtype, ndim, shape, error);
    if (status != MILENA_OK) {
        free(shape);
        return status;
    }

    size_t *coordinates = ndim > 0 ? (size_t *)calloc(ndim, sizeof(size_t)) : NULL;
    if (ndim > 0 && !coordinates) {
        free(shape);
        milena_array_release(out);
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas broadcast");
        return MILENA_ERR_MEMORY;
    }

    for (size_t index = 0; index < out->size; index++) {
        ptrdiff_t left_offset = element_offset(left, coordinates, ndim);
        ptrdiff_t right_offset = element_offset(right, coordinates, ndim);
        ptrdiff_t output_offset = (ptrdiff_t)out->byte_offset +
                                  (ptrdiff_t)(index * out->itemsize);
        if (left->dtype == MILENA_DTYPE_FLOAT64) {
            const double *left_data = (const double *)(left->storage->data + left_offset);
            const double *right_data = (const double *)(right->storage->data + right_offset);
            double *output_data = (double *)(out->storage->data + output_offset);
            *output_data = *left_data + *right_data;
        } else {
            const int64_t *left_data = (const int64_t *)(left->storage->data + left_offset);
            const int64_t *right_data = (const int64_t *)(right->storage->data + right_offset);
            int64_t *output_data = (int64_t *)(out->storage->data + output_offset);
            if ((*right_data > 0 && *left_data > INT64_MAX - *right_data) ||
                (*right_data < 0 && *left_data < INT64_MIN - *right_data)) {
                free(coordinates);
                free(shape);
                milena_array_release(out);
                array_error(error, MILENA_ERR_OVERFLOW, "Suma int64 fuera de rango");
                return MILENA_ERR_OVERFLOW;
            }
            *output_data = *left_data + *right_data;
        }
        if (ndim > 0) increment_coordinates(coordinates, ndim, shape);
    }
    free(coordinates);
    free(shape);
    return MILENA_OK;
}

MilenaStatus milena_array_sum(MilenaArray *out, const MilenaArray *source,
                              int axis, bool keepdims, MilenaError *error) {
    if (!out || !source || !source->storage) {
        array_error(error, MILENA_ERR_ARGUMENT, "Array inválido para sum");
        return MILENA_ERR_ARGUMENT;
    }
    if (source->dtype != MILENA_DTYPE_FLOAT64 && source->dtype != MILENA_DTYPE_INT64) {
        array_error(error, MILENA_ERR_UNSUPPORTED, "sum aún no está implementado para este dtype");
        return MILENA_ERR_UNSUPPORTED;
    }
    if (axis < -1 || (axis >= 0 && (size_t)axis >= source->ndim)) {
        array_error(error, MILENA_ERR_ARGUMENT, "Eje fuera de rango para sum");
        return MILENA_ERR_ARGUMENT;
    }

    size_t output_ndim = 0;
    size_t *output_shape = NULL;
    if (axis < 0) {
        output_ndim = keepdims && source->ndim > 0 ? source->ndim : 0;
        if (output_ndim > 0) {
            output_shape = (size_t *)calloc(output_ndim, sizeof(size_t));
            if (!output_shape) {
                array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la forma de sum");
                return MILENA_ERR_MEMORY;
            }
            for (size_t i = 0; i < output_ndim; i++) output_shape[i] = 1;
        }
    } else {
        output_ndim = keepdims ? source->ndim : source->ndim - 1;
        if (output_ndim > 0) {
            output_shape = (size_t *)calloc(output_ndim, sizeof(size_t));
            if (!output_shape) {
                array_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la forma de sum");
                return MILENA_ERR_MEMORY;
            }
            size_t output_axis = 0;
            for (size_t input_axis = 0; input_axis < source->ndim; input_axis++) {
                if (input_axis == (size_t)axis && keepdims) {
                    output_shape[output_axis++] = 1;
                } else if (input_axis != (size_t)axis) {
                    output_shape[output_axis++] = source->shape[input_axis];
                }
            }
        }
    }

    MilenaStatus status = allocate_array(out, source->dtype, output_ndim,
                                         output_shape, error);
    if (status != MILENA_OK) {
        free(output_shape);
        return status;
    }

    size_t *coordinates = source->ndim > 0 ?
        (size_t *)calloc(source->ndim, sizeof(size_t)) : NULL;
    if (source->ndim > 0 && !coordinates) {
        free(output_shape);
        milena_array_release(out);
        array_error(error, MILENA_ERR_MEMORY, "No se pudieron reservar coordenadas de sum");
        return MILENA_ERR_MEMORY;
    }

    if (axis < 0) {
        if (source->dtype == MILENA_DTYPE_FLOAT64) {
            double result = 0.0;
            const double *data = (const double *)milena_array_const_data(source);
            for (size_t i = 0; i < source->size; i++) result += data[i];
            ((double *)milena_array_data(out))[0] = result;
        } else {
            int64_t result = 0;
            const int64_t *data = (const int64_t *)milena_array_const_data(source);
            for (size_t i = 0; i < source->size; i++) {
                if ((data[i] > 0 && result > INT64_MAX - data[i]) ||
                    (data[i] < 0 && result < INT64_MIN - data[i])) {
                    free(coordinates); free(output_shape); milena_array_release(out);
                    array_error(error, MILENA_ERR_OVERFLOW, "sum int64 fuera de rango");
                    return MILENA_ERR_OVERFLOW;
                }
                result += data[i];
            }
            ((int64_t *)milena_array_data(out))[0] = result;
        }
        free(coordinates);
        free(output_shape);
        return MILENA_OK;
    }

    size_t reduced = source->shape[axis];
    for (size_t output_index = 0; output_index < out->size; output_index++) {
        size_t remaining = output_index;
        for (size_t input_axis = source->ndim; input_axis > 0; input_axis--) {
            size_t current = input_axis - 1;
            if (current == (size_t)axis) {
                coordinates[current] = 0;
            } else {
                size_t output_axis = current;
                if (!keepdims && current > (size_t)axis) output_axis--;
                coordinates[current] = output_shape[output_axis] == 0 ?
                    0 : remaining % output_shape[output_axis];
                if (output_shape[output_axis] > 0) remaining /= output_shape[output_axis];
            }
        }

        if (source->dtype == MILENA_DTYPE_FLOAT64) {
            double result = 0.0;
            for (size_t reduced_index = 0; reduced_index < reduced; reduced_index++) {
                coordinates[axis] = reduced_index;
                ptrdiff_t offset = (ptrdiff_t)source->byte_offset;
                for (size_t input_axis = 0; input_axis < source->ndim; input_axis++) {
                    offset += (ptrdiff_t)coordinates[input_axis] * source->strides[input_axis];
                }
                result += *(const double *)(source->storage->data + offset);
            }
            ((double *)milena_array_data(out))[output_index] = result;
        } else {
            int64_t result = 0;
            for (size_t reduced_index = 0; reduced_index < reduced; reduced_index++) {
                coordinates[axis] = reduced_index;
                ptrdiff_t offset = (ptrdiff_t)source->byte_offset;
                for (size_t input_axis = 0; input_axis < source->ndim; input_axis++) {
                    offset += (ptrdiff_t)coordinates[input_axis] * source->strides[input_axis];
                }
                int64_t value = *(const int64_t *)(source->storage->data + offset);
                if ((value > 0 && result > INT64_MAX - value) ||
                    (value < 0 && result < INT64_MIN - value)) {
                    free(coordinates); free(output_shape); milena_array_release(out);
                    array_error(error, MILENA_ERR_OVERFLOW, "sum int64 fuera de rango");
                    return MILENA_ERR_OVERFLOW;
                }
                result += value;
            }
            ((int64_t *)milena_array_data(out))[output_index] = result;
        }
    }
    free(coordinates);
    free(output_shape);
    return MILENA_OK;
}
