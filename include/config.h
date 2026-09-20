#pragma once

#include "ps5_overlay.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Set default configuration options */
void config_set_defaults(OverlayConfig* config);

/* Load configuration from file (e.g. /data/ps5_overlay/config.ini) */
bool config_load(OverlayConfig* config, const char* filepath);

/* Save current configuration to file */
bool config_save(const OverlayConfig* config, const char* filepath);

#ifdef __cplusplus
}
#endif
