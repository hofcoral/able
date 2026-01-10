#ifndef NUMBER_H
#define NUMBER_H

#include <stdbool.h>
#include <stdint.h>

#include "types/value.h"
#include "mpdecimal.h"

typedef struct Number
{
    mpd_t *dec;
} Number;

mpd_context_t *number_context(void);

Number *number_from_string(const char *text, int line, int column);
Number *number_from_int(long long value);
Number *number_from_bool(bool value);
Number *number_from_value(Value value, int line, int column);

Number *number_clone(const Number *value);
void number_free(Number *value);

char *number_to_string(const Number *value);
bool number_is_zero(const Number *value);
bool number_is_int(const Number *value);
bool number_to_long(const Number *value, long long *out, int line, int column);
double number_to_double(const Number *value, int line, int column);

int number_compare(const Number *left, const Number *right, int line, int column);

Number *number_add(const Number *left, const Number *right, int line, int column);
Number *number_sub(const Number *left, const Number *right, int line, int column);
Number *number_mul(const Number *left, const Number *right, int line, int column);
Number *number_div(const Number *left, const Number *right, int line, int column);
Number *number_mod(const Number *left, const Number *right, int line, int column);
Number *number_pow(const Number *left, const Number *right, int line, int column);

Number *number_abs(const Number *value, int line, int column);
Number *number_neg(const Number *value, int line, int column);
Number *number_floor(const Number *value, int line, int column);
Number *number_ceil(const Number *value, int line, int column);
Number *number_round(const Number *value, int scale, int line, int column);
Number *number_trunc(const Number *value, int line, int column);
Number *number_sqrt(const Number *value, int line, int column);

char *number_to_fixed(const Number *value, int scale, int line, int column);

#endif
