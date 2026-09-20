#pragma once

#include "ps5_overlay.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize overlay UI renderer (connects/injects into ShellUI or prepares HUD) */
bool overlay_ui_init(const OverlayConfig* config);

/* Update the displayed HUD text on screen */
bool overlay_ui_update(const char* hud_text);

/* Hide or show the overlay */
void overlay_ui_set_visible(bool visible);

/* Cleanup overlay UI resources */
void overlay_ui_shutdown(void);

#ifdef __cplusplus
}
#endif
