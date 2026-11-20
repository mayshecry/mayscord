#include "internal.h"

dc_result dc_upload(dc_bot *bot, const char *channel, const char *file, const char *content, dc_response *out) {
    char path[128], *escaped, *json;
    size_t n;
    dc_result r;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(channel) || !file || !*file) return DC_INVALID;
    escaped = dc_json_string(content ? content : "");
    if (!escaped) return DC_MEMORY;
    n = strlen(escaped) + 80;
    json = (char *)malloc(n);
    if (!json) { free(escaped); return DC_MEMORY; }
    snprintf(json, n, "{\"content\":%s,\"allowed_mentions\":{\"parse\":[]}}", escaped);
    snprintf(path, sizeof(path), "/channels/%s/messages", channel);
    r = dc_internal_request(bot, "POST", path, json, file, out);
    free(escaped); free(json); return r;
}

static dc_result message(dc_bot *bot, const char *channel, const char *id, const char *content, int reply, dc_response *out) {
    char path[128]; char *escaped, *json; size_t n; dc_result r;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(channel) || (id && !dc_internal_snowflake(id)) || !content || !*content) return DC_INVALID;
    escaped = dc_json_string(content);
    if (!escaped) return DC_MEMORY;
    n = strlen(escaped) + (id ? strlen(id) : 0) + 128;
    json = (char *)malloc(n);
    if (!json) { free(escaped); return DC_MEMORY; }
    if (reply) snprintf(json, n, "{\"content\":%s,\"allowed_mentions\":{\"parse\":[],\"replied_user\":false},\"message_reference\":{\"message_id\":\"%s\"}}", escaped, id);
    else snprintf(json, n, "{\"content\":%s,\"allowed_mentions\":{\"parse\":[]}}", escaped);
    if (id && !reply) snprintf(path, sizeof(path), "/channels/%s/messages/%s", channel, id);
    else snprintf(path, sizeof(path), "/channels/%s/messages", channel);
    r = dc_request(bot, id && !reply ? "PATCH" : "POST", path, json, out);
    free(json); free(escaped); return r;
}

dc_result dc_msg(dc_bot *b, const char *c, const char *s, dc_response *o) { return message(b, c, NULL, s, 0, o); }
dc_result dc_edit(dc_bot *b, const char *c, const char *id, const char *s, dc_response *o) { if (!id) { if (o) memset(o, 0, sizeof(*o)); return DC_INVALID; } return message(b, c, id, s, 0, o); }
dc_result dc_reply(dc_bot *b, const char *c, const char *id, const char *s, dc_response *o) { if (!id) { if (o) memset(o, 0, sizeof(*o)); return DC_INVALID; } return message(b, c, id, s, 1, o); }
dc_result dc_delete(dc_bot *b, const char *c, const char *id, dc_response *o) {
    char path[128];
    if (!o) return DC_INVALID;
    memset(o, 0, sizeof(*o));
    if (!dc_internal_snowflake(c) || !dc_internal_snowflake(id)) return DC_INVALID;
    snprintf(path, sizeof(path), "/channels/%s/messages/%s", c, id);
    return dc_request(b, "DELETE", path, NULL, o);
}
dc_result dc_messages(dc_bot *bot, const char *channel, unsigned limit, const char *before, dc_response *out) {
    char path[160];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(channel) || limit < 1 || limit > 100 || (before && !dc_internal_snowflake(before))) return DC_INVALID;
    if (before) snprintf(path, sizeof(path), "/channels/%s/messages?limit=%u&before=%s", channel, limit, before);
    else snprintf(path, sizeof(path), "/channels/%s/messages?limit=%u", channel, limit);
    return dc_request(bot, "GET", path, NULL, out);
}

dc_result dc_typing(dc_bot *bot, const char *channel, dc_response *out) {
    char path[96];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(channel)) return DC_INVALID;
    snprintf(path, sizeof(path), "/channels/%s/typing", channel);
    return dc_request(bot, "POST", path, NULL, out);
}

static dc_result reaction(dc_bot *bot, const char *channel, const char *message_id, const char *emoji, const char *method, dc_response *out) {
    char *escaped, *path;
    size_t n;
    dc_result r;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!bot || !dc_internal_snowflake(channel) || !dc_internal_snowflake(message_id) || !emoji || !*emoji || strlen(emoji) > 256) return DC_INVALID;
    escaped = curl_easy_escape(NULL, emoji, 0);
    if (!escaped) return DC_MEMORY;
    n = strlen(escaped) + 128;
    path = (char *)malloc(n);
    if (!path) { curl_free(escaped); return DC_MEMORY; }
    snprintf(path, n, "/channels/%s/messages/%s/reactions/%s/@me", channel, message_id, escaped);
    r = dc_request(bot, method, path, NULL, out);
    free(path); curl_free(escaped); return r;
}

dc_result dc_react(dc_bot *bot, const char *channel, const char *message_id, const char *emoji, dc_response *out) { return reaction(bot, channel, message_id, emoji, "PUT", out); }
dc_result dc_unreact(dc_bot *bot, const char *channel, const char *message_id, const char *emoji, dc_response *out) { return reaction(bot, channel, message_id, emoji, "DELETE", out); }

dc_result dc_dm_channel(dc_bot *bot, const char *user, dc_response *out) {
    char json[64];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(user)) return DC_INVALID;
    snprintf(json, sizeof(json), "{\"recipient_id\":\"%s\"}", user);
    return dc_request(bot, "POST", "/users/@me/channels", json, out);
}

dc_result dc_embed(dc_bot *bot, const char *channel, const char *title, const char *description, unsigned long color, dc_response *out) {
    char *a, *b, *json;
    char path[96];
    size_t n;
    dc_result r;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(channel) || !title || !description || color > 0xffffffUL || (!*title && !*description)) return DC_INVALID;
    a = dc_json_string(title); b = dc_json_string(description);
    if (!a || !b) { free(a); free(b); return DC_MEMORY; }
    if (strlen(a) > SIZE_MAX - strlen(b) - 128) { free(a); free(b); return DC_MEMORY; }
    n = strlen(a) + strlen(b) + 128;
    json = (char *)malloc(n);
    if (!json) { free(a); free(b); return DC_MEMORY; }
    snprintf(json, n, "{\"embeds\":[{\"title\":%s,\"description\":%s,\"color\":%lu}],\"allowed_mentions\":{\"parse\":[]}}", a, b, color);
    snprintf(path, sizeof(path), "/channels/%s/messages", channel);
    r = dc_request(bot, "POST", path, json, out);
    free(a); free(b); free(json); return r;
}

