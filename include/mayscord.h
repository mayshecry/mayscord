#ifndef MAYSCORD_H
#define MAYSCORD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dc_bot dc_bot;

typedef struct {
    long status;
    char *body;
    size_t length;
    double retry_after;
} dc_response;

typedef enum {
    DC_OK = 0,
    DC_INVALID = -1,
    DC_MEMORY = -2,
    DC_NETWORK = -3,
    DC_TOO_LARGE = -4
} dc_result;

dc_bot *dc_new(const char *token);
void dc_free(dc_bot *bot);
void dc_response_free(dc_response *response);
const char *dc_error(dc_result result);

dc_result dc_request(dc_bot *bot, const char *method, const char *path, const char *json, dc_response *out);
dc_result dc_msg(dc_bot *bot, const char *channel_id, const char *content, dc_response *out);
dc_result dc_edit(dc_bot *bot, const char *channel_id, const char *message_id, const char *content, dc_response *out);
dc_result dc_delete(dc_bot *bot, const char *channel_id, const char *message_id, dc_response *out);
dc_result dc_reply(dc_bot *bot, const char *channel_id, const char *message_id, const char *content, dc_response *out);
dc_result dc_interact(dc_bot *bot, const char *interaction_id, const char *interaction_token, int type, const char *data_json, dc_response *out);
dc_result dc_command(dc_bot *bot, const char *application_id, const char *command_json, dc_response *out);
dc_result dc_upload(dc_bot *bot, const char *channel_id, const char *file_path, const char *content, dc_response *out);
dc_result dc_roles(dc_bot *bot, const char *guild_id, dc_response *out);
dc_result dc_member(dc_bot *bot, const char *guild_id, const char *user_id, dc_response *out);
dc_result dc_user(dc_bot *bot, const char *user_id, dc_response *out);
dc_result dc_guild(dc_bot *bot, const char *guild_id, dc_response *out);
dc_result dc_channel(dc_bot *bot, const char *channel_id, dc_response *out);
dc_result dc_messages(dc_bot *bot, const char *channel_id, unsigned limit, const char *before_id, dc_response *out);
dc_result dc_typing(dc_bot *bot, const char *channel_id, dc_response *out);
dc_result dc_add_role(dc_bot *bot, const char *guild_id, const char *user_id, const char *role_id, dc_response *out);
dc_result dc_remove_role(dc_bot *bot, const char *guild_id, const char *user_id, const char *role_id, dc_response *out);
dc_result dc_react(dc_bot *bot, const char *channel_id, const char *message_id, const char *emoji, dc_response *out);
dc_result dc_unreact(dc_bot *bot, const char *channel_id, const char *message_id, const char *emoji, dc_response *out);
dc_result dc_dm_channel(dc_bot *bot, const char *user_id, dc_response *out);
dc_result dc_embed(dc_bot *bot, const char *channel_id, const char *title, const char *description, unsigned long color, dc_response *out);
dc_result dc_interact_reply(dc_bot *bot, const char *interaction_id, const char *interaction_token, const char *content, int ephemeral, dc_response *out);
typedef void (*dc_event_fn)(dc_bot *bot, const char *event, const char *json, void *userdata);
dc_result dc_gateway(dc_bot *bot, unsigned long intents, dc_event_fn callback, void *userdata);
void dc_stop(dc_bot *bot);
char *dc_field(const char *object_json, const char *key);
char *dc_json_string(const char *text);

#ifdef __cplusplus
}
#endif
#endif
