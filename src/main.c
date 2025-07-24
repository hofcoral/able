#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "types/env.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "ast/ast.h"
#include "utils/utils.h"

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file)
    {
        log_error("Error:Could not open file%s", filename);
        exit(1);
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Allocate buffer for parsing text
    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
    {
        log_error("Error: Memory allocation failed");
        exit(1);
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0'; // Null-terminate EOF

    fclose(file);
    return buffer;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        log_info("Usage: %s <file.abl>", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *code = read_file(filename);

    Lexer lexer = {code, 0, strlen(code)};

    int stmt_count;

    ASTNode **prog = parse_program(&lexer, &stmt_count);

    Env *global_env = env_create(NULL);
    interpreter_set_env(global_env);
    run_ast(prog, stmt_count);

    // Run "Garbage Collection"
    free_ast(prog, stmt_count);
    env_release(global_env);
    free(code);

    return 0;
}
