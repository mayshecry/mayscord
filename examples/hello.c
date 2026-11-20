#include "mayscord.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *token = getenv("DISCORD_TOKEN");
    const char *channel = getenv("DISCORD_CHANNEL_ID");
    dc_bot *bot;
    dc_response response;
    dc_result result;
    int exit_code = 0;
    if (!token || !channel) {
        fprintf(stderr, "Set DISCORD_TOKEN and DISCORD_CHANNEL_ID.\n");
        return 1;
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    bot = dc_new(token);
    if (!bot) {
        fprintf(stderr, "Invalid token or out of memory.\n");
        curl_global_cleanup();
        return 1;
    }
    result = dc_msg(bot, channel, "Hello from C!", &response);
    if (result != DC_OK) {
        fprintf(stderr, "Request failed: %s\n", dc_error(result));
        exit_code = 1;
    } else if (response.status < 200 || response.status >= 300) {
        fprintf(stderr, "Discord returned HTTP %ld. Retry after: %.3f seconds.\n", response.status, response.retry_after);
        exit_code = 1;
    } else {
        printf("Message sent (HTTP %ld).\n", response.status);
    }
    dc_response_free(&response);
    dc_free(bot);
    curl_global_cleanup();
    return exit_code;
}
