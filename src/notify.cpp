#include "notify.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#if defined(__PS5__) || defined(PS5)
#define SCE_NOTIFICATION_LOCAL_USER_ID_SYSTEM 0xFE

extern "C" {
int sceNotificationSend(int userId, bool isLogged, const char* payload);
}
#endif

bool notify_send(const char* message) {
    if (!message || message[0] == '\0') {
        return false;
    }

#if defined(__PS5__) || defined(PS5)
    char payload[1024];
    /* Escape quotes and newlines for JSON payload */
    char escaped[512] = {0};
    int j = 0;
    for (int i = 0; message[i] != '\0' && j < (int)sizeof(escaped) - 2; i++) {
        if (message[i] == '"') {
            escaped[j++] = '\\';
            escaped[j++] = '"';
        } else if (message[i] == '\n') {
            escaped[j++] = ' ';
        } else {
            escaped[j++] = message[i];
        }
    }
    escaped[j] = '\0';

    snprintf(payload, sizeof(payload),
        "{\n"
        "  \"rawData\": {\n"
        "    \"viewTemplateType\": \"InteractiveToastTemplateB\",\n"
        "    \"channelType\": \"Downloads\",\n"
        "    \"useCaseId\": \"IDC\",\n"
        "    \"toastOverwriteType\": \"No\",\n"
        "    \"isImmediate\": true,\n"
        "    \"priority\": 100,\n"
        "    \"viewData\": {\n"
        "      \"message\": {\n"
        "        \"body\": \"%s\"\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}",
        escaped
    );

    int ret = sceNotificationSend(SCE_NOTIFICATION_LOCAL_USER_ID_SYSTEM, true, payload);
    return (ret == 0);
#else
    printf("[NOTIFY] %s\n", message);
    return true;
#endif
}

bool notify_send_fmt(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return notify_send(buffer);
}
