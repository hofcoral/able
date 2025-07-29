#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#include "parser/parser.h"
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "types/object.h"
#include "types/env.h"
#include "utils/utils.h"
#include "interpreter/interpreter.h"
#include "interpreter/module.h"

#include "uthash.h"

typedef struct ModuleEntry {
    char *name;
    Value obj;
    Env *env;
    UT_hash_handle hh;
} ModuleEntry;

static ModuleEntry *modules = NULL;
static Env *global_env_ref = NULL;
static char exec_dir[PATH_MAX];

static char *find_module_file(const char *name)
{
    const char *ablepath = getenv("ABLEPATH");
    const char *paths[64];
    int path_count = 0;
    if (exec_dir[0]) {
        static char libdir[PATH_MAX];
        snprintf(libdir, sizeof(libdir), "%s/lib", exec_dir);
        paths[path_count++] = libdir;
    }
    paths[path_count++] = ".";
    if (ablepath) {
        char *dup = strdup(ablepath);
        char *tok = strtok(dup, ":");
        while (tok && path_count < 64) {
            paths[path_count++] = tok;
            tok = strtok(NULL, ":");
        }
    }
    for (int i = 0; i < path_count; ++i) {
        static char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%s/%s.abl", paths[i], name);
        if (access(buf, R_OK) == 0)
            return strdup(buf);
    }
    return NULL;
}

static ModuleEntry *load_module(const char *name, int line, int column)
{
    ModuleEntry *m = NULL;
    HASH_FIND_STR(modules, name, m);
    if (m)
        return m;

    char *file = find_module_file(name);
    if (!file) {
        log_script_error(line, column, "ImportError: module '%s' not found", name);
        exit(1);
    }
    char *src = read_file(file);
    Lexer lx; lexer_init(&lx, src);
    int count; ASTNode **prog = parse_program(&lx, &count);
    Env *env = env_create(global_env_ref);
    interpreter_set_env(env);
    run_ast(prog, count);
    interpreter_pop_env();

    Object *obj = malloc(sizeof(Object));
    obj->count = 0; obj->capacity = 0; obj->pairs = NULL;
    Variable *var, *tmp;
    HASH_ITER(hh, env->vars, var, tmp) {
        object_set(obj, var->name, var->value);
    }
    Value val = { .type = VAL_OBJECT, .obj = obj };

    m = malloc(sizeof(ModuleEntry));
    m->name = strdup(name);
    m->obj = val;
    m->env = env;
    HASH_ADD_KEYPTR(hh, modules, m->name, strlen(m->name), m);

    free_ast(prog, count);
    free(src);
    free(file);
    return m;
}

void module_system_init(Env *global_env, const char *exec_path)
{
    global_env_ref = global_env;
    modules = NULL;
    if (exec_path) {
        char real[PATH_MAX];
        if (realpath(exec_path, real)) {
            char *dir = dirname(real);
            char *parent = dirname(dir);
            strncpy(exec_dir, parent, sizeof(exec_dir));
        } else {
            exec_dir[0] = '\0';
        }
    } else {
        exec_dir[0] = '\0';
    }
}

void module_system_cleanup()
{
    ModuleEntry *cur, *tmp;
    HASH_ITER(hh, modules, cur, tmp) {
        HASH_DEL(modules, cur);
        free(cur->name);
        free_object(cur->obj.obj);
        env_release(cur->env);
        free(cur);
    }
}

Value import_module_value(const char *name, int line, int column)
{
    ModuleEntry *m = load_module(name, line, column);
    return m->obj;
}

Value import_module_attr(const char *mod, const char *attr, int line, int column)
{
    ModuleEntry *m = load_module(mod, line, column);
    Value v = object_get(m->obj.obj, attr);
    if (v.type == VAL_NULL || v.type == VAL_UNDEFINED) {
        log_script_error(line, column, "ImportError: module '%s' has no attribute '%s'", mod, attr);
        exit(1);
    }
    return clone_value(&v);
}
