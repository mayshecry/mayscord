#include "mayscord.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    dc_bot *bot = dc_new("abc.DEF_123-xyz");
    dc_response r;
    char *s;
    assert(bot);
    assert(!dc_new("bad\r\nHeader: x"));
    assert(!dc_new(""));
    s = dc_json_string("a\"\\\n\t");
    assert(s && strcmp(s, "\"a\\\"\\\\\\u000a\\u0009\"") == 0);
    free(s);
    s = dc_json_string("");
    assert(s && strcmp(s, "\"\"") == 0);
    free(s);
    assert(dc_msg(bot, "1/evil", "hello", &r) == DC_INVALID);
    assert(!r.body);
    assert(dc_msg(bot, "123", "", &r) == DC_INVALID);
    assert(dc_delete(bot, "123", NULL, &r) == DC_INVALID);
    assert(dc_request(bot, "GET", "//example.com/", NULL, &r) == DC_INVALID);
    assert(dc_request(bot, "TRACE", "/users/@me", NULL, &r) == DC_INVALID);
    assert(dc_interact(bot, "1", "bad/token", 4, "{}", &r) == DC_INVALID);
    assert(dc_roles(bot, "1/2", &r) == DC_INVALID);
    assert(dc_member(bot, "1", "a", &r) == DC_INVALID);
    assert(dc_upload(bot, "1", NULL, "hi", &r) == DC_INVALID);
    assert(dc_user(bot, "x", &r) == DC_INVALID);
    assert(dc_guild(bot, "x", &r) == DC_INVALID);
    assert(dc_channel(bot, "x", &r) == DC_INVALID);
    assert(dc_messages(bot, "1", 0, NULL, &r) == DC_INVALID);
    assert(dc_messages(bot, "1", 101, NULL, &r) == DC_INVALID);
    assert(dc_messages(bot, "1", 10, "x", &r) == DC_INVALID);
    assert(dc_typing(bot, "x", &r) == DC_INVALID);
    assert(dc_add_role(bot, "1", "2", "x", &r) == DC_INVALID);
    assert(dc_remove_role(bot, "1", "x", "3", &r) == DC_INVALID);
    assert(dc_react(bot, "1", "2", "", &r) == DC_INVALID);
    assert(dc_unreact(bot, "1", "x", "ok", &r) == DC_INVALID);
    assert(dc_dm_channel(bot, "x", &r) == DC_INVALID);
    assert(dc_embed(bot, "1", "", "", 0, &r) == DC_INVALID);
    assert(dc_embed(bot, "1", "title", "description", 0x1000000, &r) == DC_INVALID);
    assert(dc_interact_reply(bot, "1", "token", "hello", 2, &r) == DC_INVALID);
    s = dc_field("{\"t\":\"MESSAGE_CREATE\",\"d\":{\"content\":\"hi, {wow}\",\"id\":\"123\"}}", "d");
    assert(s && strcmp(s, "{\"content\":\"hi, {wow}\",\"id\":\"123\"}") == 0);
    free(s);
    s = dc_field("{\"a\": 3, \"b\": [1, {\"x\": true}]}", "b");
    assert(s && strcmp(s, "[1, {\"x\": true}]") == 0);
    free(s);
    assert(!dc_field("{\"a\":1}", "missing"));
    dc_response_free(&r);
    dc_free(bot);
    puts("ok");
    return 0;
}
