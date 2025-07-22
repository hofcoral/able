#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "types/variable.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "ast/ast.h"

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file)
    {
        fprintf(stderr, "Error:Could not open file%s\n", filename);
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
        fprintf(stderr, "Error: Memory allocation failed\n");
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
        printf("Usage: %s <file.abl>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *code = read_file(filename);

    Lexer lexer = {code, 0, strlen(code)};

    int stmt_count;

    ASTNode **prog = parse_program(&lexer, &stmt_count);

    run_ast(prog, stmt_count);

    // Run "Garbage Collection"
    free_ast(prog, stmt_count);
    free_vars();
    free(code);

    return 0;
}
