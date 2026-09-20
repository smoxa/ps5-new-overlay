#include "notify.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined(__PS5__) || defined(PS5)
extern "C" {
int sceSysUtilSendSystemNotificationWithText(int type, const char* text);
}
#endif

bool notify_send(const char* message) {
    if (!message || message[0] == '\0') {
        return false;
    }

#if defined(__PS5__) || defined(PS5)
    /* 0x222 is the standard notification toast without extra icons */
    int ret = sceSysUtilSendSystemNotificationWithText(0x222, message);
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
