#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Display a system notification toast on PS5 screen */
bool notify_send(const char* message);

/* Formatted notification */
bool notify_send_fmt(const char* format, ...);

#ifdef __cplusplus
}
#endif
