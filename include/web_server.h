#pragma once

#include "ps5_overlay.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start the embedded Web HUD server on the specified port */
bool web_server_start(int port);

/* Update latest metrics snapshot served by the web server */
void web_server_update_metrics(const HardwareMetrics* metrics);

/* Stop the embedded Web HUD server */
void web_server_stop(void);

#ifdef __cplusplus
}
#endif
