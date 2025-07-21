#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "interpreter.h"

#define MAX_VARS 128

typedef struct
{
    char *name;
    Value value;
} Variable;

static Variable vars[MAX_VARS];
static int var_count = 0;

void set_variable(const char *name, Value val)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(vars[i].name, name) == 0)
        {
            // Free previous value if string
            if (vars[i].value.type == VAL_STRING)
            {
                free(vars[i].value.str);
            }

            // Assign new value
            if (val.type == VAL_STRING)
            {
                vars[i].value.type = VAL_STRING;
                vars[i].value.str = strdup(val.str);
            }
            else if (val.type == VAL_NUMBER)
            {
                vars[i].value.type = VAL_NUMBER;
                vars[i].value.num = val.num;
            }
            return;
        }
    }

    // New variable
    vars[var_count].name = strdup(name);
    if (val.type == VAL_STRING)
    {
        vars[var_count].value.type = VAL_STRING;
        vars[var_count].value.str = strdup(val.str);
    }
    else if (val.type == VAL_NUMBER)
    {
        vars[var_count].value.type = VAL_NUMBER;
        vars[var_count].value.num = val.num;
    }
    var_count++;
}

Value get_variable(const char *name)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(vars[i].name, name) == 0)
        {
            return vars[i].value; // safe if caller doesn't free
        }
    }
    fprintf(stderr, "Runtime error: variable '%s' is not defined.\n", name);
    exit(1);
}

void run_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];
        // Set stmt
        if (n->type == NODE_SET)
        {
            Value v;

            if (n->copy_from_var)
            {
                v = get_variable(n->copy_from_var);
                if (v.type == VAL_STRING)
                    v.str = strdup(v.str); // ensure safety
            }
            else
            {
                v = n->literal_value;
                if (v.type == VAL_STRING)
                    v.str = strdup(v.str); // duplicate before storing
            }

            set_variable(n->set_name, v);
        }
        // Function call
        else if (n->type == NODE_FUNC_CALL)
        {
            if (strcmp(n->func_name, "pr") == 0)
            {
                if (n->arg_count != 1)
                {
                    fprintf(stderr, "pr() takes 1 argument\n");
                    continue;
                }
                ASTNode *arg = n->args[0];
                if (arg->type != NODE_VAR)
                {
                    fprintf(stderr, "pr() only supports variables for now\n");
                    continue;
                }
                Value val = get_variable(arg->set_name);
                if (val.type == VAL_STRING)
                {
                    printf("%s\n", val.str);
                }
                else if (val.type == VAL_NUMBER)
                {
                    printf("%f\n", val.num);
                }
                else
                {
                    printf("(undefined: %s)\n", arg->set_name);
                }
            }
            else
            {
                fprintf(stderr, "Unknown function: %s\n", n->func_name);
            }
        }
    }
}

void free_vars()
{
    for (int i = 0; i < var_count; i++)
    {
        free(vars[i].name);
        if (vars[i].value.type == VAL_STRING)
        {
            free(vars[i].value.str);
        }
    }
    var_count = 0;
}
