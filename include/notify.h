#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Display a system notification toast on PS5 screen */
bool notify_send(const char* message);

/* Display a dual-line HUD notification toast */
bool notify_send_hud(const char* line1, const char* line2);

/* Formatted notification */
bool notify_send_fmt(const char* format, ...);

#ifdef __cplusplus
}
#endif
