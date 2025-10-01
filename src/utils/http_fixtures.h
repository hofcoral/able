#ifndef HTTP_FIXTURES_H
#define HTTP_FIXTURES_H

#include <stdbool.h>

#include "utils/http_client.h"

bool http_fixtures_try_get(const char *method, const char *url, HttpResponse *response);

#endif
