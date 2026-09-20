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

static void escape_json_str(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dst_size - 2; i++) {
        if (src[i] == '"') {
            dst[j++] = '\\';
            dst[j++] = '"';
        } else if (src[i] == '\\') {
            dst[j++] = '\\';
            dst[j++] = '\\';
        } else if (src[i] == '\n' || src[i] == '\r') {
            dst[j++] = ' ';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

bool notify_send_hud(const char* line1, const char* line2) {
    if (!line1 && !line2) {
        return false;
    }

#if defined(__PS5__) || defined(PS5)
    char payload[1536];
    char esc1[256] = {0};
    char esc2[256] = {0};

    if (line1) escape_json_str(line1, esc1, sizeof(esc1));
    if (line2) escape_json_str(line2, esc2, sizeof(esc2));

    if (esc2[0] != '\0') {
        snprintf(payload, sizeof(payload),
            "{\n"
            "  \"rawData\": {\n"
            "    \"viewTemplateType\": \"InteractiveToastTemplateB\",\n"
            "    \"channelType\": \"Downloads\",\n"
            "    \"useCaseId\": \"IDC\",\n"
            "    \"toastOverwriteType\": \"OverwriteLatest\",\n"
            "    \"isImmediate\": true,\n"
            "    \"priority\": 100,\n"
            "    \"viewData\": {\n"
            "      \"message\": {\n"
            "        \"body\": \"%s\"\n"
            "      },\n"
            "      \"subMessage\": {\n"
            "        \"body\": \"%s\"\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "}",
            esc1, esc2
        );
    } else {
        snprintf(payload, sizeof(payload),
            "{\n"
            "  \"rawData\": {\n"
            "    \"viewTemplateType\": \"InteractiveToastTemplateB\",\n"
            "    \"channelType\": \"Downloads\",\n"
            "    \"useCaseId\": \"IDC\",\n"
            "    \"toastOverwriteType\": \"OverwriteLatest\",\n"
            "    \"isImmediate\": true,\n"
            "    \"priority\": 100,\n"
            "    \"viewData\": {\n"
            "      \"message\": {\n"
            "        \"body\": \"%s\"\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "}",
            esc1
        );
    }

    int ret = sceNotificationSend(SCE_NOTIFICATION_LOCAL_USER_ID_SYSTEM, true, payload);
    return (ret == 0);
#else
    if (line2 && line2[0] != '\0') {
        printf("[NOTIFY] %s | %s\n", line1 ? line1 : "", line2);
    } else {
        printf("[NOTIFY] %s\n", line1 ? line1 : "");
    }
    return true;
#endif
}

bool notify_send(const char* message) {
    if (!message || message[0] == '\0') {
        return false;
    }

    /* Check if message contains a newline to split into dual-line HUD */
    const char* nl = strchr(message, '\n');
    if (nl) {
        char line1[256] = {0};
        char line2[256] = {0};
        size_t l1_len = (size_t)(nl - message);
        if (l1_len >= sizeof(line1)) l1_len = sizeof(line1) - 1;
        strncpy(line1, message, l1_len);
        line1[l1_len] = '\0';

        const char* p2 = nl + 1;
        while (*p2 == '\n' || *p2 == '\r') p2++;
        strncpy(line2, p2, sizeof(line2) - 1);
        line2[sizeof(line2) - 1] = '\0';

        return notify_send_hud(line1, line2);
    }

    return notify_send_hud(message, nullptr);
}

bool notify_send_fmt(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return notify_send(buffer);
}
