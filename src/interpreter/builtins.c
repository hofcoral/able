#include <string.h>
#include "interpreter/builtins.h"
#include "types/value.h"
#include "utils/utils.h"

void builtins_register(Env *global_env, const char *file_path)
{
    const char *funcs[] = {"pr", "input", "type", "len", "bool", "int", "float",
                            "str", "list", "dict", "range"};
    Value undef = {.type = VAL_UNDEFINED};
    for (size_t i = 0; i < sizeof(funcs) / sizeof(funcs[0]); ++i)
        set_variable(global_env, funcs[i], undef);

    const char *errors[] = {"TypeError", "ImportError", "StopIteration"};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
        set_variable(global_env, errors[i], undef);

    Value ver = {.type = VAL_STRING, .str = strdup("0.1.0")};
    set_variable(global_env, "__version__", ver);
    Value filev = {.type = VAL_STRING, .str = strdup(file_path)};
    set_variable(global_env, "__file__", filev);
}
