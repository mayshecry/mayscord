#include "internal.h"

char *dc_json_string(const char *text) {
    const unsigned char *p;
    char *out, *q;
    size_t n = 2;
    static const char hex[] = "0123456789abcdef";
    if (!text) return NULL;
    for (p = (const unsigned char *)text; *p; p++) {
        size_t add = (*p < 0x20) ? 6 : (*p == '"' || *p == '\\') ? 2 : 1;
        if (n > SIZE_MAX - add - 1) return NULL;
        n += add;
    }
    out = (char *)malloc(n + 1);
    if (!out) return NULL;
    q = out; *q++ = '"';
    for (p = (const unsigned char *)text; *p; p++) {
        if (*p < 0x20) {
            *q++ = '\\'; *q++ = 'u'; *q++ = '0'; *q++ = '0';
            *q++ = hex[*p >> 4]; *q++ = hex[*p & 15];
        } else if (*p == '"' || *p == '\\') { *q++ = '\\'; *q++ = (char)*p; }
        else *q++ = (char)*p;
    }
    *q++ = '"'; *q = 0;
    return out;
}

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    return p;
}

static const char *skip_value(const char *p, int depth) {
    char end;
    if (depth > 128) return NULL;
    p = skip_ws(p);
    if (*p == '"') {
        p++;
        while (*p && *p != '"') {
            if (*p == '\\') { p++; if (!*p) return NULL; }
            p++;
        }
        return *p == '"' ? p + 1 : NULL;
    }
    if (*p == '{' || *p == '[') {
        end = *p++ == '{' ? '}' : ']';
        for (;;) {
            p = skip_ws(p);
            if (*p == end) return p + 1;
            if (!*p) return NULL;
            if (*p == ',' || *p == ':') { p++; continue; }
            p = skip_value(p, depth + 1);
            if (!p) return NULL;
        }
    }
    if (!*p) return NULL;
    while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') p++;
    return p;
}

char *dc_field(const char *json, const char *key) {
    const char *p, *k, *key_end, *end, *value;
    size_t n, kn;
    char *out;
    if (!json || !key || strchr(key, '"') || strchr(key, '\\')) return NULL;
    p = skip_ws(json);
    if (*p++ != '{') return NULL;
    kn = strlen(key);
    for (;;) {
        p = skip_ws(p);
        if (*p == '}' || !*p) return NULL;
        if (*p != '"') return NULL;
        k = p + 1;
        end = skip_value(p, 0);
        if (!end) return NULL;
        key_end = end - 1;
        p = skip_ws(end);
        if (*p++ != ':') return NULL;
        value = skip_ws(p);
        end = skip_value(value, 0);
        if (!end) return NULL;
        if ((size_t)(key_end - k) == kn && !memcmp(k, key, kn)) {
            n = (size_t)(end - value);
            out = (char *)malloc(n + 1);
            if (!out) return NULL;
            memcpy(out, value, n); out[n] = 0;
            return out;
        }
        p = skip_ws(end);
        if (*p == ',') p++;
        else if (*p != '}') return NULL;
    }
}

