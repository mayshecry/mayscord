#ifndef MAYSCORD_INTERNAL_H
#define MAYSCORD_INTERNAL_H

#include "mayscord.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>

#define DC_LIMIT (8u * 1024u * 1024u)
#define DC_API "https://discord.com/api/v10"

struct dc_bot { char *token; time_t wait_until; int stop; };
struct buffer { char *data; size_t size; int failure; };

void dc_internal_wipe(void *p, size_t n);
int dc_internal_snowflake(const char *s);
int dc_internal_safe_token(const char *s);
size_t dc_internal_receive(char *ptr, size_t size, size_t count, void *userdata);
void dc_internal_pause_ms(unsigned long ms);
dc_result dc_internal_request(dc_bot *bot, const char *method, const char *path, const char *json, const char *file, dc_response *out);

#endif
