#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/number.h"
#include "utils/utils.h"

static mpd_context_t NUMBER_CONTEXT;
static int NUMBER_CONTEXT_READY = 0;

static void ensure_number_context(void)
{
    if (NUMBER_CONTEXT_READY)
        return;
    mpd_defaultcontext(&NUMBER_CONTEXT);
    NUMBER_CONTEXT.prec = 20;
    NUMBER_CONTEXT.round = MPD_ROUND_HALF_UP;
    NUMBER_CONTEXT.traps = 0;
    NUMBER_CONTEXT.status = 0;
    NUMBER_CONTEXT.clamp = MPD_CLAMP_DEFAULT;
    NUMBER_CONTEXT_READY = 1;
}

mpd_context_t *number_context(void)
{
    ensure_number_context();
    return &NUMBER_CONTEXT;
}

static void number_raise(uint32_t status, int line, int column, const char *context)
{
    if (status == 0)
        return;

    if (status & MPD_Conversion_syntax)
        log_script_error(line, column, "Invalid number literal");
    else if (status & MPD_Division_by_zero)
        log_script_error(line, column, "Division by zero");
    else if (status & MPD_Invalid_operation)
        log_script_error(line, column, "Invalid number operation");
    else if (status & MPD_Overflow)
        log_script_error(line, column, "Number overflow");
    else if (status & MPD_Underflow)
        log_script_error(line, column, "Number underflow");
    else if (status & MPD_Malloc_error)
        log_script_error(line, column, "Number allocation failed");
    else if (status & MPD_Not_implemented)
        log_script_error(line, column, "Number operation not implemented");
    else
        log_script_error(line, column, "Number error in %s", context);

    exit(1);
}

static void number_check_status(uint32_t status, int line, int column, const char *context)
{
    uint32_t fatal = status & (MPD_IEEE_Invalid_operation | MPD_Division_by_zero |
                               MPD_Overflow | MPD_Underflow | MPD_Malloc_error |
                               MPD_Not_implemented);
    if (fatal)
        number_raise(fatal, line, column, context);
}

static Number *number_alloc(void)
{
    Number *value = malloc(sizeof(Number));
    if (!value)
        return NULL;
    value->dec = mpd_new(number_context());
    if (!value->dec)
    {
        free(value);
        return NULL;
    }
    return value;
}

Number *number_from_string(const char *text, int line, int column)
{
    if (!text)
        text = "0";
    Number *value = number_alloc();
    if (!value)
    {
        if (line == 0 && column == 0)
            return NULL;
        number_raise(MPD_Malloc_error, line, column, "number_from_string");
    }
    uint32_t status = 0;
    mpd_qset_string(value->dec, text, number_context(), &status);
    if (status)
    {
        number_free(value);
        if (line == 0 && column == 0)
            return NULL;
        number_check_status(status, line, column, "number_from_string");
    }
    return value;
}

Number *number_from_int(long long value)
{
    Number *num = number_alloc();
    if (!num)
        return NULL;
    uint32_t status = 0;
    mpd_qset_i64(num->dec, (int64_t)value, number_context(), &status);
    if (status)
        number_raise(status, 0, 0, "number_from_int");
    return num;
}

Number *number_from_bool(bool value)
{
    return number_from_int(value ? 1 : 0);
}

Number *number_from_value(Value value, int line, int column)
{
    switch (value.type)
    {
    case VAL_NUMBER:
        return number_clone(value.number);
    case VAL_BOOL:
        return number_from_bool(value.boolean);
    case VAL_STRING:
        return number_from_string(value.str, line, column);
    default:
        log_script_error(line, column, "Value is not a number");
        exit(1);
    }
}

Number *number_clone(const Number *value)
{
    if (!value || !value->dec)
        return NULL;
    Number *copy = number_alloc();
    if (!copy)
        return NULL;
    uint32_t status = 0;
    mpd_qcopy(copy->dec, value->dec, &status);
    if (status)
    {
        number_free(copy);
        return NULL;
    }
    return copy;
}

void number_free(Number *value)
{
    if (!value)
        return;
    if (value->dec)
        mpd_del(value->dec);
    free(value);
}

char *number_to_string(const Number *value)
{
    if (!value || !value->dec)
        return NULL;
    char *text = mpd_to_sci(value->dec, 0);
    if (!text)
        return NULL;
    char *out = strdup(text);
    mpd_free(text);
    return out;
}

bool number_is_zero(const Number *value)
{
    if (!value || !value->dec)
        return true;
    return mpd_iszero(value->dec);
}

bool number_is_int(const Number *value)
{
    if (!value || !value->dec)
        return false;
    return mpd_isinteger(value->dec);
}

bool number_to_long(const Number *value, long long *out, int line, int column)
{
    if (!value || !value->dec)
        return false;
    if (!mpd_isinteger(value->dec))
    {
        log_script_error(line, column, "Expected an integer");
        exit(1);
    }
    uint32_t status = 0;
    int64_t result = mpd_qget_i64(value->dec, &status);
    if (status)
        number_raise(status, line, column, "number_to_long");
    if (out)
        *out = (long long)result;
    return true;
}

double number_to_double(const Number *value, int line, int column)
{
    if (!value || !value->dec)
        number_raise(MPD_Invalid_operation, line, column, "number_to_double");
    if (mpd_isinteger(value->dec))
    {
        long long out = 0;
        number_to_long(value, &out, line, column);
        return (double)out;
    }
    char *text = number_to_string(value);
    if (!text)
        number_raise(MPD_Invalid_operation, line, column, "number_to_double");
    char *end = NULL;
    double result = strtod(text, &end);
    free(text);
    if (!end || *end != '\0')
        number_raise(MPD_Invalid_operation, line, column, "number_to_double");
    return result;
}

int number_compare(const Number *left, const Number *right, int line, int column)
{
    uint32_t status = 0;
    int cmp = mpd_qcmp(left->dec, right->dec, &status);
    number_check_status(status, line, column, "number_compare");
    return cmp;
}

static Number *number_binary(const Number *left, const Number *right,
                             void (*op)(mpd_t *, const mpd_t *, const mpd_t *, const mpd_context_t *, uint32_t *),
                             int line, int column, const char *context)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, context);
    uint32_t status = 0;
    op(result->dec, left->dec, right->dec, number_context(), &status);
    number_check_status(status, line, column, context);
    return result;
}

Number *number_add(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qadd, line, column, "number_add");
}

Number *number_sub(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qsub, line, column, "number_sub");
}

Number *number_mul(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qmul, line, column, "number_mul");
}

Number *number_div(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qdiv, line, column, "number_div");
}

Number *number_mod(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qrem, line, column, "number_mod");
}

Number *number_pow(const Number *left, const Number *right, int line, int column)
{
    return number_binary(left, right, mpd_qpow, line, column, "number_pow");
}

Number *number_abs(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_abs");
    uint32_t status = 0;
    mpd_qabs(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_abs");
    return result;
}

Number *number_neg(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_neg");
    uint32_t status = 0;
    mpd_qminus(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_neg");
    return result;
}

Number *number_floor(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_floor");
    uint32_t status = 0;
    mpd_qfloor(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_floor");
    return result;
}

Number *number_ceil(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_ceil");
    uint32_t status = 0;
    mpd_qceil(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_ceil");
    return result;
}

Number *number_round(const Number *value, int scale, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_round");
    uint32_t status = 0;
    mpd_qrescale(result->dec, value->dec, -scale, number_context(), &status);
    number_check_status(status, line, column, "number_round");
    return result;
}

Number *number_trunc(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_trunc");
    uint32_t status = 0;
    mpd_qtrunc(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_trunc");
    return result;
}

Number *number_sqrt(const Number *value, int line, int column)
{
    Number *result = number_alloc();
    if (!result)
        number_raise(MPD_Malloc_error, line, column, "number_sqrt");
    uint32_t status = 0;
    mpd_qsqrt(result->dec, value->dec, number_context(), &status);
    number_check_status(status, line, column, "number_sqrt");
    return result;
}

char *number_to_fixed(const Number *value, int scale, int line, int column)
{
    if (scale < 0)
    {
        log_script_error(line, column, "to_fixed() expects a non-negative scale");
        exit(1);
    }
    Number *rounded = number_round(value, scale, line, column);
    char fmt[32];
    snprintf(fmt, sizeof(fmt), ".%df", scale);
    uint32_t status = 0;
    char *formatted = mpd_qformat(rounded->dec, fmt, number_context(), &status);
    number_free(rounded);
    number_check_status(status, line, column, "number_to_fixed");
    if (!formatted)
        return NULL;
    char *out = strdup(formatted);
    mpd_free(formatted);
    return out;
}
