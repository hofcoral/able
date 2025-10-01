#ifndef INTERPRETER_SERVER_H
#define INTERPRETER_SERVER_H

#include "types/value.h"

Value interpreter_server_listen(const Value *args, int arg_count, int line, int column);

#endif
