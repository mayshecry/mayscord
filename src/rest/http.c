#include "internal.h"

struct header_data { dc_response *response; double reset; int remaining_zero; };

static int header_match(const char *p, size_t n, const char *key) {
    size_t i, len = strlen(key);
    if (n < len) return 0;
    for (i = 0; i < len; i++) if (tolower((unsigned char)p[i]) != key[i]) return 0;
    return 1;
}

static double header_number(const char *p, size_t n, size_t skip) {
    char tmp[64];
    size_t len = n - skip;
    if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
    memcpy(tmp, p + skip, len); tmp[len] = 0;
    return strtod(tmp, NULL);
}

static size_t headers(char *ptr, size_t size, size_t count, void *userdata) {
    struct header_data *h = (struct header_data *)userdata;
    size_t n;
    if (size && count > SIZE_MAX / size) return 0;
    n = size * count;
    if (header_match(ptr, n, "retry-after:")) h->response->retry_after = header_number(ptr, n, 12);
    if (header_match(ptr, n, "x-ratelimit-reset-after:")) h->reset = header_number(ptr, n, 24);
    if (header_match(ptr, n, "x-ratelimit-remaining:")) h->remaining_zero = header_number(ptr, n, 22) == 0;

    return n;
}

static int valid_path(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    if (!s || s[0] != '/' || s[1] == '/' || strlen(s) > 8192) return 0;
    for (; *p; p++) if (*p <= 32 || *p >= 127 || *p == '#' || *p == '\\') return 0;
    return 1;
}

dc_result dc_internal_request(dc_bot *bot, const char *method, const char *path, const char *json, const char *file, dc_response *out) {
    CURL *curl = NULL;
    CURLcode code;
    struct curl_slist *list = NULL, *next;
    struct buffer b = { NULL, 0, 0 };
    struct header_data h = { out, 0, 0 };
    curl_mime *mime = NULL;
    unsigned int attempt;
    char *url = NULL, *auth = NULL;
    size_t len;
    dc_result result = DC_OK;
    if (!out) return DC_INVALID;
    memset(out, 0, sizeof(*out));
    if (!bot || !method || !valid_path(path)) return DC_INVALID;
    if (strcmp(method, "GET") && strcmp(method, "POST") && strcmp(method, "PUT") && strcmp(method, "PATCH") && strcmp(method, "DELETE")) return DC_INVALID;
    len = strlen(DC_API) + strlen(path) + 1;
    url = (char *)malloc(len);
    auth = (char *)malloc(strlen(bot->token) + sizeof("Authorization: Bot "));
    if (!url || !auth) { result = DC_MEMORY; goto done; }
    snprintf(url, len, "%s%s", DC_API, path);
    sprintf(auth, "Authorization: Bot %s", bot->token);
    list = curl_slist_append(NULL, auth);
    if (!list) { result = DC_MEMORY; goto done; }
    if (!file) {
        next = curl_slist_append(list, "Content-Type: application/json");
        if (!next) { result = DC_MEMORY; goto done; }
        list = next;
    }
    curl = curl_easy_init();
    if (!curl) { result = DC_MEMORY; goto done; }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DiscordBot (https://github.com/mayshecry, 0.1.0) mayscord/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dc_internal_receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headers);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &h);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (file) {
        curl_mimepart *part;
        mime = curl_mime_init(curl);
        if (!mime) { result = DC_MEMORY; goto done; }
        part = curl_mime_addpart(mime);
        if (!part || curl_mime_name(part, "payload_json") != CURLE_OK || curl_mime_data(part, json, CURL_ZERO_TERMINATED) != CURLE_OK || curl_mime_type(part, "application/json") != CURLE_OK) { result = DC_MEMORY; goto done; }
        part = curl_mime_addpart(mime);
        if (!part || curl_mime_name(part, "files[0]") != CURLE_OK || curl_mime_filedata(part, file) != CURLE_OK) { result = DC_INVALID; goto done; }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    } else if (json) { curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json); curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strlen(json)); }
    for (attempt = 0; attempt < 4; attempt++) {
        while (time(NULL) < bot->wait_until) dc_internal_pause_ms(250);
        free(b.data); b.data = NULL; b.size = 0; b.failure = 0;
        out->retry_after = 0; h.reset = 0; h.remaining_zero = 0;
        code = curl_easy_perform(curl);
        if (code != CURLE_OK) { result = b.failure ? (dc_result)b.failure : DC_NETWORK; goto done; }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->status);
        if (h.remaining_zero && h.reset > 0 && h.reset <= 3600) bot->wait_until = time(NULL) + (time_t)h.reset + 1;
        if (out->status == 429 && out->retry_after > 0 && out->retry_after <= 3600) bot->wait_until = time(NULL) + (time_t)out->retry_after + 1;
        if (out->status != 429 || out->retry_after <= 0 || out->retry_after > 60 || attempt == 3) break;
    }
    if (!b.data) { b.data = (char *)calloc(1, 1); if (!b.data) { result = DC_MEMORY; goto done; } }
    out->body = b.data; out->length = b.size; b.data = NULL;
done:
    free(b.data);
    if (mime) curl_mime_free(mime);
    if (curl) curl_easy_cleanup(curl);
    if (list) {
        if (list->data) dc_internal_wipe(list->data, strlen(list->data));
        curl_slist_free_all(list);
    }
    if (auth) { dc_internal_wipe(auth, strlen(auth)); free(auth); }
    free(url);
    return result;
}

dc_result dc_request(dc_bot *bot, const char *method, const char *path, const char *json, dc_response *out) {
    return dc_internal_request(bot, method, path, json, NULL, out);
}

