#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "types/env.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "interpreter/module.h"
#include "interpreter/builtins.h"
#include "ast/ast.h"
#include "utils/utils.h"


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        log_info("Usage: %s <file.abl>", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *code = read_file(filename);

    Lexer lexer;
    lexer_init(&lexer, code);

    int stmt_count;

    ASTNode **prog = parse_program(&lexer, &stmt_count);

    Env *global_env = env_create(NULL);
    interpreter_init();
    module_system_init(global_env, argv[0]);
    builtins_register(global_env, filename);
    interpreter_set_env(global_env);
    run_ast(prog, stmt_count);
    module_system_cleanup();
    interpreter_cleanup();

    // Run "Garbage Collection"
    free_ast(prog, stmt_count);
    env_release(global_env);
    free(code);

    return 0;
}
