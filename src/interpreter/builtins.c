#include <string.h>
#include "interpreter/builtins.h"
#include "interpreter/module.h"
#include "types/object.h"
#include "types/promise.h"
#include "types/value.h"
#include "utils/utils.h"

void builtins_register(Env *global_env, const char *file_path)
{
    const char *funcs[] = {"pr", "input", "type", "len", "bool", "int", "float",
                            "str", "list", "dict", "range", "register_modifier", "register_decorator",
                            "server_listen", "json_stringify", "json_parse", "read_text_file"};
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
    Value promise_ns = promise_namespace_value();
    set_variable(global_env, "Promise", promise_ns);

    /* Load Able-defined built-ins from lib/builtins.abl */
    Value mod = import_module_value("builtins", 0, 0);
    Object *obj = mod.obj;
    for (int i = 0; i < obj->count; ++i)
    {
        set_variable(global_env, obj->pairs[i].key, obj->pairs[i].value);
    }
}
