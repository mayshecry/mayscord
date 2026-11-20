#include "mayscord.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *id_value(char *s) {
    size_t n;
    if (!s) return NULL;
    n = strlen(s);
    if (n < 3 || s[0] != '"' || s[n - 1] != '"') return NULL;
    s[n - 1] = 0;
    return s + 1;
}

static void on_event(dc_bot *bot, const char *name, const char *json, void *userdata) {
    char *content, *channel, *message, *author, *is_bot;
    char *channel_id, *message_id;
    dc_response r;
    dc_result status;
    (void)userdata;
    if (strcmp(name, "MESSAGE_CREATE")) return;
    content = dc_field(json, "content");
    channel = dc_field(json, "channel_id");
    message = dc_field(json, "id");
    author = dc_field(json, "author");
    is_bot = author ? dc_field(author, "bot") : NULL;
    channel_id = id_value(channel);
    message_id = id_value(message);
    if (content && !strcmp(content, "\"!ping\"") && channel_id && message_id && (!is_bot || strcmp(is_bot, "true"))) {
        status = dc_reply(bot, channel_id, message_id, "Pong!", &r);
        if (status != DC_OK) fprintf(stderr, "Reply: %s\n", dc_error(status));
        else if (r.status < 200 || r.status >= 300) fprintf(stderr, "Reply: HTTP %ld\n", r.status);
        dc_response_free(&r);
    }
    free(content); free(channel); free(message); free(author); free(is_bot);
}

int main(void) {
    dc_bot *bot;
    dc_result result;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    bot = dc_new(getenv("DISCORD_TOKEN"));
    if (!bot) { fprintf(stderr, "Set a valid DISCORD_TOKEN.\n"); curl_global_cleanup(); return 1; }
    result = dc_gateway(bot, 1UL | 512UL | 4096UL | 32768UL, on_event, NULL);
    if (result != DC_OK) fprintf(stderr, "Gateway stopped: %s\n", dc_error(result));
    dc_free(bot);
    curl_global_cleanup();
    return result == DC_OK ? 0 : 1;
}
