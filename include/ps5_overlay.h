#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PS5_OVERLAY_VERSION "1.0.10"
#define PS5_OVERLAY_DEFAULT_CONFIG_PATH "/data/ps5_overlay/config.ini"

#ifdef __cplusplus
extern "C" {
#endif

/* Hardware metrics snapshot */
typedef struct {
    int cpu_temp;             /* Celsius */
    int soc_temp;             /* Celsius (APU/GPU) */
    float cpu_usage;          /* Total CPU load % */
    float cpu_core_usage[8];  /* Per-core CPU load % */
    int ram_used_mb;          /* Used system RAM in MB */
    int ram_total_mb;         /* Total system RAM in MB */
    float ram_percentage;     /* RAM used % */
    int vram_used_mb;         /* Used VRAM in MB */
    int vram_total_mb;        /* Total VRAM in MB */
    float vram_percentage;    /* VRAM used % */
    int fan_duty_percent;     /* Fan speed percentage 0-100% */
} HardwareMetrics;

/* Overlay configuration */
typedef struct {
    bool enabled;
    bool show_cpu_temp;
    bool show_cpu_load;
    bool show_all_cores;
    bool show_gpu_temp;
    bool show_gpu_load;
    bool show_ram;
    bool show_fan;
    bool background_panel;
    int position;             /* 0 = Top, 1 = Bottom */
    int font_size;            /* Font size pt (default: 18) */
    int update_interval_ms;   /* Polling interval in ms (default: 1000) */
    bool toast_notifications; /* Periodic native OSD toast overlay */
    int toast_interval_sec;   /* Toast interval in seconds (default: 4) */
    bool web_server_enabled;  /* Embedded real-time Web HUD (default: true) */
    int web_port;             /* Web HUD HTTP port (default: 8080) */
} OverlayConfig;

#ifdef __cplusplus
}
#endif
