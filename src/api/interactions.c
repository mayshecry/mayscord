#include "internal.h"

dc_result dc_interact(dc_bot *b, const char *id, const char *token, int type, const char *data, dc_response *o) {
    char *path, *json; size_t n; dc_result r;
    if (!o) return DC_INVALID;
    memset(o, 0, sizeof(*o));
    if (!dc_internal_snowflake(id) || !dc_internal_safe_token(token) || type < 1 || type > 255) return DC_INVALID;
    n = strlen(id) + strlen(token) + 32; path = (char *)malloc(n);
    if (!path) return DC_MEMORY;
    snprintf(path, n, "/interactions/%s/%s/callback", id, token);
    n = (data ? strlen(data) : 0) + 64; json = (char *)malloc(n);
    if (!json) { free(path); return DC_MEMORY; }
    if (data) snprintf(json, n, "{\"type\":%d,\"data\":%s}", type, data);
    else snprintf(json, n, "{\"type\":%d}", type);
    r = dc_request(b, "POST", path, json, o);
    dc_internal_wipe(path, strlen(path)); free(path); free(json); return r;
}
dc_result dc_command(dc_bot *b, const char *id, const char *json, dc_response *o) {
    char path[96];
    if (!o) return DC_INVALID;
    memset(o, 0, sizeof(*o));
    if (!dc_internal_snowflake(id) || !json) return DC_INVALID;
    snprintf(path, sizeof(path), "/applications/%s/commands", id);
    return dc_request(b, "POST", path, json, o);
}

dc_result dc_interact_reply(dc_bot *bot, const char *id, const char *token, const char *content, int ephemeral, dc_response *out) {
    char *escaped, *data;
    size_t n;
    dc_result r;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(id) || !dc_internal_safe_token(token) || !content || !*content || (ephemeral != 0 && ephemeral != 1)) return DC_INVALID;
    escaped = dc_json_string(content);
    if (!escaped) return DC_MEMORY;
    n = strlen(escaped) + 100;
    data = (char *)malloc(n);
    if (!data) { free(escaped); return DC_MEMORY; }
    snprintf(data, n, "{\"content\":%s,\"flags\":%d,\"allowed_mentions\":{\"parse\":[]}}", escaped, ephemeral ? 64 : 0);
    r = dc_interact(bot, id, token, 4, data, out);
    free(escaped); free(data); return r;
}
