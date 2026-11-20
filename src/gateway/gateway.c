#include "internal.h"

static dc_result ws_send(CURL *ws, const char *text) {
    size_t n = strlen(text), sent = 0;
    CURLcode code;
    unsigned tries = 0;
    do {
        code = curl_ws_send(ws, text, n, &sent, 0, CURLWS_TEXT);
        if (code == CURLE_AGAIN) { dc_internal_pause_ms(10); tries++; }
    } while (code == CURLE_AGAIN && tries < 500);
    return code == CURLE_OK && sent == n ? DC_OK : DC_NETWORK;
}

void dc_stop(dc_bot *bot) { if (bot) bot->stop = 1; }

dc_result dc_gateway(dc_bot *bot, unsigned long intents, dc_event_fn callback, void *userdata) {
    CURL *ws;
    CURLcode code;
    struct buffer message_buffer = { NULL, 0, 0 };
    dc_result result = DC_NETWORK;
    char chunk[8192], *op = NULL, *data = NULL, *seq = NULL, *event = NULL, *interval = NULL, *payload = NULL;
    char sequence[32] = "null";
    time_t next_beat = 0;
    long heartbeat = 0;
    int awaiting_ack = 0, identified = 0;
    if (!bot || !callback) return DC_INVALID;
    bot->stop = 0;
    ws = curl_easy_init();
    if (!ws) return DC_MEMORY;
    curl_easy_setopt(ws, CURLOPT_URL, "wss://gateway.discord.gg/?v=10&encoding=json");
    curl_easy_setopt(ws, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(ws, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(ws, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(ws, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(ws, CURLOPT_NOSIGNAL, 1L);
    code = curl_easy_perform(ws);
    if (code != CURLE_OK) goto done;
    while (!bot->stop) {
        size_t n = 0;
        const struct curl_ws_frame *meta = NULL;
        if (heartbeat && time(NULL) >= next_beat) {
            char beat[96];
            if (awaiting_ack) goto done;
            snprintf(beat, sizeof(beat), "{\"op\":1,\"d\":%s}", sequence);
            if (ws_send(ws, beat) != DC_OK) goto done;
            awaiting_ack = 1; next_beat = time(NULL) + heartbeat;
        }
        code = curl_ws_recv(ws, chunk, sizeof(chunk), &n, &meta);
        if (code == CURLE_AGAIN) { dc_internal_pause_ms(25); continue; }
        if (code != CURLE_OK || !meta || (meta->flags & CURLWS_CLOSE)) goto done;
        if (meta->flags & (CURLWS_PING | CURLWS_PONG)) continue;
        if (meta->flags & CURLWS_BINARY) goto done;
        if (n && dc_internal_receive(chunk, 1, n, &message_buffer) != n) { result = (dc_result)message_buffer.failure; goto done; }
        if (meta->bytesleft || (meta->flags & CURLWS_CONT)) continue;
        if (!message_buffer.data) continue;
        op = dc_field(message_buffer.data, "op");
        data = dc_field(message_buffer.data, "d");
        seq = dc_field(message_buffer.data, "s");
        event = dc_field(message_buffer.data, "t");
        if (!op || !data) goto done;
        if (seq && strcmp(seq, "null") && strlen(seq) < sizeof(sequence)) {
            const char *p = seq;
            while (*p >= '0' && *p <= '9') p++;
            if (!*p && p != seq) strcpy(sequence, seq);
        }
        if (!strcmp(op, "10")) {
            size_t needed;
            interval = dc_field(data, "heartbeat_interval");
            if (!interval || identified) goto done;
            heartbeat = strtol(interval, NULL, 10) / 1000;
            if (heartbeat < 1 || heartbeat > 3600) goto done;
            next_beat = time(NULL) + heartbeat / 2;
            needed = strlen(bot->token) + 256;
            payload = (char *)malloc(needed);
            if (!payload) { result = DC_MEMORY; goto done; }
            snprintf(payload, needed, "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%lu,\"properties\":{\"os\":\"mayscord\",\"browser\":\"mayscord\",\"device\":\"mayscord\"}}}", bot->token, intents);
            if (ws_send(ws, payload) != DC_OK) goto done;
            dc_internal_wipe(payload, strlen(payload)); free(payload); payload = NULL;
            identified = 1;
        } else if (!strcmp(op, "11")) awaiting_ack = 0;
        else if (!strcmp(op, "1")) {
            char beat[96];
            snprintf(beat, sizeof(beat), "{\"op\":1,\"d\":%s}", sequence);
            if (ws_send(ws, beat) != DC_OK) goto done;
            awaiting_ack = 1;
        } else if (!strcmp(op, "7") || !strcmp(op, "9")) goto done;
        else if (!strcmp(op, "0") && event && event[0] == '"') {
            size_t len = strlen(event);
            if (len >= 2 && event[len-1] == '"') { event[len-1] = 0; callback(bot, event + 1, data, userdata); }
        }
        free(op); op = NULL; free(data); data = NULL; free(seq); seq = NULL; free(event); event = NULL; free(interval); interval = NULL;
        free(message_buffer.data); message_buffer.data = NULL; message_buffer.size = 0;
    }
    result = DC_OK;
done:
    if (payload) { dc_internal_wipe(payload, strlen(payload)); free(payload); }
    free(op); free(data); free(seq); free(event); free(interval); free(message_buffer.data);
    curl_easy_cleanup(ws);
    return result;
}

