#pragma once

#include "ps5_overlay.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the hardware monitor subsystem */
bool monitor_init(void);

/* Collect the latest hardware metrics */
bool monitor_update(HardwareMetrics* metrics);

/* Format metrics into a human-readable HUD string */
void monitor_format_hud_string(const HardwareMetrics* metrics, const OverlayConfig* config, char* buffer, size_t max_len);

/* Format metrics into dual-line HUD strings (Line 1: CPU/GPU, Line 2: RAM/Fan) */
void monitor_format_hud_lines(const HardwareMetrics* metrics, const OverlayConfig* config,
                              char* line1, size_t line1_len, char* line2, size_t line2_len);

/* Free/cleanup monitor subsystem */
void monitor_cleanup(void);

#ifdef __cplusplus
}
#endif
