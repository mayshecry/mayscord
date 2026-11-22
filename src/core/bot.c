#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

void dc_internal_wipe(void *p, size_t n) {
    volatile unsigned char *v = (volatile unsigned char *)p;
    while (n--) *v++ = 0;
}

int dc_internal_snowflake(const char *s) {
    size_t n = 0;
    if (!s || !*s) return 0;
    while (*s) { if (*s < '0' || *s > '9' || ++n > 20) return 0; s++; }
    return 1;
}

int dc_internal_safe_token(const char *s) {
    size_t n = 0;
    if (!s || !*s) return 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (!(isalnum(c) || c == '.' || c == '_' || c == '-')) return 0;
        if (++n > 4096) return 0;
    }
    return 1;
}

dc_bot *dc_new(const char *token) {
    dc_bot *bot;
    size_t n;
    if (!dc_internal_safe_token(token)) return NULL;
    bot = (dc_bot *)calloc(1, sizeof(*bot));
    if (!bot) return NULL;
    n = strlen(token) + 1;
    bot->token = (char *)malloc(n);
    if (!bot->token) { free(bot); return NULL; }
    memcpy(bot->token, token, n);
    return bot;
}

void dc_free(dc_bot *bot) {
    if (!bot) return;
    if (bot->token) { dc_internal_wipe(bot->token, strlen(bot->token)); free(bot->token); }
    free(bot);
}

void dc_response_free(dc_response *r) {
    if (!r) return;
    free(r->body);
    memset(r, 0, sizeof(*r));
}

const char *dc_error(dc_result r) {
    switch (r) {
        case DC_OK: return "success";
        case DC_INVALID: return "invalid argument";
        case DC_MEMORY: return "out of memory";
        case DC_NETWORK: return "network or TLS error";
        case DC_TOO_LARGE: return "response exceeds size limit";
        default: return "unknown error";
    }
}

size_t dc_internal_receive(char *ptr, size_t size, size_t count, void *userdata) {
    struct buffer *b = (struct buffer *)userdata;
    size_t n;
    char *next;
    if (size && count > SIZE_MAX / size) { b->failure = DC_TOO_LARGE; return 0; }
    n = size * count;
    if (n > DC_LIMIT - b->size) { b->failure = DC_TOO_LARGE; return 0; }
    next = (char *)realloc(b->data, b->size + n + 1);
    if (!next) { b->failure = DC_MEMORY; return 0; }
    b->data = next;
    memcpy(b->data + b->size, ptr, n);
    b->size += n;
    b->data[b->size] = 0;
    return n;
}

void dc_internal_pause_ms(unsigned long ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timeval tv;
    tv.tv_sec = (long)(ms / 1000);
    tv.tv_usec = (long)((ms % 1000) * 1000);
    select(0, NULL, NULL, NULL, &tv);
#endif
}


