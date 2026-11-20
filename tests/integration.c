#include "mayscord.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

static void check(dc_result result, dc_response *r, long status) {
    if (result != DC_OK || r->status != status) {
        fprintf(stderr, "request failure: result=%d HTTP=%ld\n", result, r->status);
        exit(1);
    }
    dc_response_free(r);
}

int main(int argc, char **argv) {
    dc_bot *bot;
    dc_response r;
    dc_result e;
    if (argc != 2 || curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    bot = dc_new("test.token");
    if (!bot) return 1;
    e = dc_msg(bot, "123", "Hello \"world\"", &r); check(e, &r, 200);
    e = dc_upload(bot, "123", argv[1], "file", &r); check(e, &r, 200);
    e = dc_typing(bot, "123", &r); check(e, &r, 204);
    e = dc_react(bot, "123", "456", "hello:789", &r); check(e, &r, 204);
    e = dc_unreact(bot, "123", "456", "hello:789", &r); check(e, &r, 204);
    e = dc_add_role(bot, "1", "2", "3", &r); check(e, &r, 204);
    e = dc_remove_role(bot, "1", "2", "3", &r); check(e, &r, 204);
    e = dc_messages(bot, "123", 10, "456", &r); check(e, &r, 200);
    e = dc_dm_channel(bot, "2", &r); check(e, &r, 200);
    e = dc_embed(bot, "123", "title", "description", 0xff00ff, &r); check(e, &r, 200);
    e = dc_interact_reply(bot, "1", "token", "hello", 1, &r); check(e, &r, 200);
    dc_free(bot);
    curl_global_cleanup();
    return 0;
}
