#include "array.h"

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
        case MILENA_DTYPE_INT64: return "int64";
        case MILENA_DTYPE_FLOAT64: return "float64";
        default: return "unknown";
    }
}

size_t milena_dtype_size(MilenaDType dtype) {
    switch (dtype) {
        case MILENA_DTYPE_BOOL: return sizeof(bool);
        case MILENA_DTYPE_INT64: return sizeof(int64_t);
        case MILENA_DTYPE_FLOAT64: return sizeof(double);
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
