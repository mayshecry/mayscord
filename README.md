# mayscord

A small C99 Discord library for Linux and Windows, created by [@mayshecry](https://github.com/mayshecry). It offers simple message helpers, a raw JSON REST API, file uploads and an experimental Gateway listener.

## Status

This is an early library, not a complete Discord framework. Gateway events, heartbeat/identify, messages, replies, roles, reactions, embeds, JSON REST calls, uploads and basic rate-limit handling are implemented. Gateway reconnect/resume, sharding, voice, interaction HTTP signature verification and production-grade rate-limit coordination are not implemented. It has not been security-audited or tested with a real bot token, and the Windows build has not been verified.

## Requirements

- C99 compiler
- libcurl 7.86 or newer, built with HTTPS and WebSocket (`wss`) support
- Make and pkg-config, or CMake 3.15+

Some libcurl packages are built without WebSocket support even when the version is new enough; use a WebSocket-enabled build for `dc_gateway`.

## Linux

Install your distribution's libcurl development package, pkg-config and a C compiler:

```sh
make
make test
make integration
```

The optional integration test needs Python 3 and starts a local mock HTTP server. It compiles with a temporary test-only internal header pointing to localhost; normal builds always use Discord HTTPS. It does not contact Discord.

Or:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Windows

Install Visual Studio C++ tools, CMake and a WebSocket-enabled libcurl. For example, using vcpkg:

```powershell
vcpkg install curl[websockets]:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Availability of the vcpkg feature depends on your vcpkg version. Ensure libcurl runtime DLLs are available beside the executable or on PATH. Windows instructions have not been tested.

## Reply to users

Create a Discord bot, invite it to a server, and give it permission to view the channel, read message history and send messages. Enable the **Message Content Intent** in the Developer Portal for this example. Set `DISCORD_TOKEN`, then run:

```sh
export DISCORD_TOKEN='your-bot-token'
./ping
```

When a user sends `!ping`, the bot replies `Pong!`. The example subscribes to guilds, guild messages, direct messages and message content. Your bot must have the appropriate access and Discord must supply the relevant event content. The Gateway listener stops on connection errors; it does not automatically reconnect.

```c
static void on_event(dc_bot *bot, const char *event, const char *json, void *data) {
    if (strcmp(event, "MESSAGE_CREATE") == 0) {
        puts(json);
    }
}

dc_gateway(bot, 1UL | 512UL | 4096UL | 32768UL, on_event, NULL);
```

See `examples/ping.c` for the complete example, including extraction of the channel and message IDs, ignoring bot authors and checking errors. The callback receives a temporary event name and raw JSON `d` object. Copy data you need to keep. Call `dc_stop(bot)` inside a callback to exit normally. Do not call it from another thread: bots are not thread-safe. Callbacks run synchronously on the Gateway loop; keep them short, since blocking work, including a slow `dc_reply`, can delay heartbeats. A production bot should queue events and perform work separately with appropriate rate-limit coordination.

## Send, reply, upload and inspect roles

C has no object methods, so `dc_msg(bot, channel, content, &response)` is the equivalent of `s.msg("content")`. All IDs are decimal strings, not numbers.

```c
dc_response r;
dc_result result = dc_reply(bot, channel_id, message_id, "Hello!", &r);
if (result == DC_OK && r.status >= 200 && r.status < 300) {
    puts("Replied!");
}
dc_response_free(&r);
```

Other helpers:

```c
dc_msg(bot, channel_id, "Hello!", &r);
dc_edit(bot, channel_id, message_id, "Updated", &r);
dc_delete(bot, channel_id, message_id, &r);
dc_upload(bot, channel_id, "/path/to/image.png", "An image", &r);
dc_roles(bot, guild_id, &r);
dc_member(bot, guild_id, user_id, &r);
```

Call only one function at a time and free its response before the next call. More helpers:

| Function | Purpose |
| --- | --- |
| `dc_user(bot, user_id, &r)` | Get a user |
| `dc_guild(bot, guild_id, &r)` | Get a guild |
| `dc_channel(bot, channel_id, &r)` | Get a channel |
| `dc_messages(bot, channel_id, limit, before_id, &r)` | List 1–100 messages; pass `NULL` for no `before` cursor |
| `dc_typing(bot, channel_id, &r)` | Send a typing indicator |
| `dc_add_role(bot, guild_id, user_id, role_id, &r)` | Give a member a role |
| `dc_remove_role(bot, guild_id, user_id, role_id, &r)` | Remove a member's role |
| `dc_react(bot, channel_id, message_id, emoji, &r)` | Add the bot's reaction |
| `dc_unreact(bot, channel_id, message_id, emoji, &r)` | Remove the bot's reaction |
| `dc_dm_channel(bot, user_id, &r)` | Create or fetch a DM channel |
| `dc_embed(bot, channel_id, title, description, color, &r)` | Send a simple embed |
| `dc_interact_reply(bot, interaction_id, token, content, ephemeral, &r)` | Send a normal or ephemeral interaction response |

Reactions accept a UTF-8 Unicode emoji or a custom emoji as `name:id`; the library URL-encodes it. For DMs, take the `id` from the successful `dc_dm_channel` response, free that response, then use `dc_msg` with the channel ID. DM delivery may be blocked by user settings. Embed color is an integer from `0` to `0xffffff`; Discord validates embed sizes. Interaction replies require a valid received interaction and a timely response. Set `ephemeral` to 1 or 0. Role changes require Manage Roles and a role below the bot's highest role.

Free each response before reusing `r`. `dc_roles` returns the guild's role objects as JSON; `dc_member` returns a member object whose `roles` field contains role IDs. Discord permissions and access apply. Upload accepts one local file per call, uses multipart form data and streams from disk through libcurl. Discord enforces its own file size and content limits. Never pass an untrusted file path without validating it yourself.

Every request initializes its response, including on failure. `DC_OK` means the HTTP exchange completed, **not** that Discord accepted it: check `r.status` for a 2xx status. `r.body` is a null-terminated response body, `r.length` is its byte count, and `r.retry_after` is the Retry-After header in seconds, if present. Free with `dc_response_free`. `dc_error` describes non-HTTP errors.

## Other endpoints

```c
dc_request(bot, "GET", "/users/@me", NULL, &r);
dc_command(bot, application_id, "{\"name\":\"hello\",\"description\":\"Say hello\"}", &r);
dc_interact(bot, interaction_id, interaction_token, 4, "{\"content\":\"Hello!\",\"allowed_mentions\":{\"parse\":[]}}", &r);
```

`dc_request` accepts `GET`, `POST`, `PUT`, `PATCH`, or `DELETE`, a path relative to `https://discord.com/api/v10` and an optional raw JSON body. It cannot set custom headers or upload arbitrary multipart bodies. `dc_command` registers a global application command. `dc_interact` sends a callback; your application must receive and authenticate interactions and meet Discord's deadline. Raw JSON is not validated or escaped by the library. Use `dc_json_string` for a null-terminated UTF-8 string; free the result. Embedded NUL and UTF-8 validation are not supported.

`dc_field(object_json, key)` returns a newly allocated **raw JSON value**, or NULL if absent, invalid or out of memory. String values retain quotes and escapes, numbers and objects are returned as JSON text. Free it with `free`. It is a small field extractor, not a validating JSON parser; use a full JSON library for general processing, decoded Unicode, and untrusted JSON.

## Rate limits

REST calls are synchronous. When Discord reports zero remaining requests and a reset interval, the library conservatively waits before the next request on that bot, even for unrelated routes. On a 429 with a positive Retry-After header of at most 60 seconds it waits and retries up to three times; otherwise it returns the 429. A 429 with a wait up to one hour also delays subsequent requests on that bot. Longer waits, absent headers, invalid-request limits, proactive global request limits, route buckets and coordination across processes or bot objects are the application's responsibility. Never blindly retry failed transport operations: the server may already have acted on them. The library does not retry network failures.

## Security and lifecycle

Call `curl_global_init` once before starting threads or using the library, and `curl_global_cleanup` only after all library work is finished. `dc_new` copies the token; `dc_free` releases it. Do not use a bot concurrently or free it during an operation. Callbacks and synchronous REST calls can block; requests have a 10-second connection timeout and 30-second total timeout, not counting rate-limit waits and retries.

TLS certificate and hostname verification are enabled, redirects are disabled, response bodies are capped at 8 MiB, and message helpers disable automatic mentions. Gateway event buffers are also capped at 8 MiB. Token storage is wiped on free, but copies may exist in libcurl, the environment or OS memory; wiping is not a complete secret-protection guarantee. Keep bot and interaction tokens out of source control, logs and client-side code. Apply least-privilege permissions. Raw JSON payloads must explicitly control `allowed_mentions`.

Consult [Discord's API documentation](https://discord.com/developers/docs/intro) for current payloads, intents, permissions, rate limits and API behavior.

## Project layout

```text
include/             Public API
src/core/            Bot lifecycle and shared utilities
src/api/             Message, guild, role and interaction helpers
src/rest/            HTTP transport and rate-limit handling
src/gateway/         Gateway event loop
src/util/            JSON helpers
src/internal/        Private shared definitions
examples/            Example programs
tests/               Unit and mock-server integration tests
```

Applications include `mayscord.h`; files in `src/internal/` are private implementation details.
