#include "ps5_overlay.h"
#include "monitor.h"
#include "overlay_ui.h"
#include "notify.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

static volatile bool s_running = true;

static void signal_handler(int sig) {
    (void)sig;
    s_running = false;
}

int main(int argc, char** argv) {
    printf("=========================================\n");
    printf("   PS5 Hardware Overlay v%s\n", PS5_OVERLAY_VERSION);
    printf("=========================================\n");

    bool test_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("ps5_overlay version %s\n", PS5_OVERLAY_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--test") == 0 || strcmp(argv[i], "-t") == 0) {
            test_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: ps5_overlay [options]\n");
            printf("Options:\n");
            printf("  -v, --version  Show version\n");
            printf("  -t, --test     Run one sample test and exit\n");
            printf("  -h, --help     Show this help message\n");
            return 0;
        }
    }

    /* Register signal handlers for clean exit */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Load configuration */
    OverlayConfig config;
    if (config_load(&config, PS5_OVERLAY_DEFAULT_CONFIG_PATH)) {
        printf("[CONFIG] Loaded settings from %s\n", PS5_OVERLAY_DEFAULT_CONFIG_PATH);
    } else {
        printf("[CONFIG] Using default settings (config not found or unreadable)\n");
    }

    if (!config.enabled) {
        printf("[INFO] Overlay disabled in configuration. Exiting.\n");
        return 0;
    }

    /* Initialize subsystems */
    if (!monitor_init()) {
        fprintf(stderr, "[ERROR] Failed to initialize hardware monitor.\n");
        return 1;
    }

    if (!overlay_ui_init(&config)) {
        fprintf(stderr, "[WARNING] Overlay UI init returned false; proceeding with fallback.\n");
    }

    /* Send startup notification toast */
    notify_send_fmt("PS5 Overlay v%s Started!\nCPU/GPU & Memory HUD Active", PS5_OVERLAY_VERSION);

    printf("[STATUS] Overlay daemon running. Polling interval: %d ms\n", config.update_interval_ms);

    HardwareMetrics metrics{};
    char hud_text[256]{};
    time_t last_toast_time = 0;

    /* Main monitor loop */
    while (s_running) {
        if (monitor_update(&metrics)) {
            monitor_format_hud_string(&metrics, &config, hud_text, sizeof(hud_text));
            overlay_ui_update(hud_text);

            if (config.toast_notifications) {
                time_t now = time(nullptr);
                if (now - last_toast_time >= config.toast_interval_sec) {
                    notify_send(hud_text);
                    last_toast_time = now;
                }
            }
        }

        if (test_mode) {
            printf("[TEST] HUD Output: %s\n", hud_text);
            break;
        }

        usleep(config.update_interval_ms * 1000);
    }

    printf("\n[STATUS] Shutting down PS5 Overlay daemon...\n");
    overlay_ui_shutdown();
    monitor_cleanup();
    printf("[STATUS] Goodbye!\n");
    return 0;
}
