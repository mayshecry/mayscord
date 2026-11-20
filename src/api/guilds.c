#include "internal.h"

dc_result dc_roles(dc_bot *bot, const char *guild, dc_response *out) {
    char path[80];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(guild)) return DC_INVALID;
    snprintf(path, sizeof(path), "/guilds/%s/roles", guild);
    return dc_request(bot, "GET", path, NULL, out);
}

dc_result dc_member(dc_bot *bot, const char *guild, const char *user, dc_response *out) {
    char path[100];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(guild) || !dc_internal_snowflake(user)) return DC_INVALID;
    snprintf(path, sizeof(path), "/guilds/%s/members/%s", guild, user);
    return dc_request(bot, "GET", path, NULL, out);
}

static dc_result get_one(dc_bot *bot, const char *prefix, const char *id, dc_response *out) {
    char path[128];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(id)) return DC_INVALID;
    snprintf(path, sizeof(path), "/%s/%s", prefix, id);
    return dc_request(bot, "GET", path, NULL, out);
}

dc_result dc_user(dc_bot *bot, const char *user, dc_response *out) { return get_one(bot, "users", user, out); }
dc_result dc_guild(dc_bot *bot, const char *guild, dc_response *out) { return get_one(bot, "guilds", guild, out); }
dc_result dc_channel(dc_bot *bot, const char *channel, dc_response *out) { return get_one(bot, "channels", channel, out); }

static dc_result role_change(dc_bot *bot, const char *guild, const char *user, const char *role, const char *method, dc_response *out) {
    char path[128];
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!dc_internal_snowflake(guild) || !dc_internal_snowflake(user) || !dc_internal_snowflake(role)) return DC_INVALID;
    snprintf(path, sizeof(path), "/guilds/%s/members/%s/roles/%s", guild, user, role);
    return dc_request(bot, method, path, NULL, out);
}

dc_result dc_add_role(dc_bot *bot, const char *guild, const char *user, const char *role, dc_response *out) { return role_change(bot, guild, user, role, "PUT", out); }
dc_result dc_remove_role(dc_bot *bot, const char *guild, const char *user, const char *role, dc_response *out) { return role_change(bot, guild, user, role, "DELETE", out); }

