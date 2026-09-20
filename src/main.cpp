#include "ps5_overlay.h"
#include "monitor.h"
#include "overlay_ui.h"
#include "notify.h"
#include "config.h"
#include "web_server.h"
#include "shellui_inject.h"
#include "embedded_shellui.h"

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

    /* Inject in-game overlay into SceShellUI */
    bool hud_injected = false;
    if (!test_mode) {
        unlink("/system_tmp/ps5_overlay_ready");
        unlink("/system_tmp/ps5_overlay.log");

        pid_t shellui_pid = shellui_find_pid();
        if (shellui_pid > 0) {
            printf("[STATUS] Found SceShellUI (PID: %d). Injecting in-game overlay...\n", shellui_pid);
            if (shellui_inject_elf(shellui_pid, g_overlay_shellui_elf, g_overlay_shellui_elf_size)) {
                printf("[STATUS] Successfully injected HUD into SceShellUI!\n");
                hud_injected = true;
            } else {
                fprintf(stderr, "[WARNING] Failed to inject HUD into SceShellUI.\n");
            }
        } else {
            fprintf(stderr, "[WARNING] SceShellUI process not found.\n");
        }
    }

    /* Start embedded Web HUD server */
    if (config.web_server_enabled && !test_mode) {
        if (web_server_start(config.web_port)) {
            printf("[STATUS] Web HUD running at http://0.0.0.0:%d/\n", config.web_port);
        } else {
            fprintf(stderr, "[WARNING] Failed to start Web HUD server on port %d\n", config.web_port);
        }
    }

    /* Send single startup notification toast */
    if (hud_injected) {
        notify_send_hud("PS5 Overlay Active", "HUD Injected! Open game to see OSD | Web: 8080");
    } else {
        notify_send_hud("PS5 Overlay Warning", "HUD inject failed. Check http://<ip>:8080/log");
    }

    printf("[STATUS] Overlay daemon running. Toast interval: %d s | Polling: %d ms\n",
           config.toast_interval_sec, config.update_interval_ms);

    HardwareMetrics metrics{};
    char hud_text[256]{};
    char hud_line1[128]{};
    char hud_line2[128]{};
    time_t last_toast_time = 0;

    /* Main monitor loop */
    while (s_running) {
        if (monitor_update(&metrics)) {
            monitor_format_hud_string(&metrics, &config, hud_text, sizeof(hud_text));
            monitor_format_hud_lines(&metrics, &config, hud_line1, sizeof(hud_line1), hud_line2, sizeof(hud_line2));

            overlay_ui_update(hud_text);

            if (config.web_server_enabled) {
                web_server_update_metrics(&metrics);
            }

            if (config.toast_notifications) {
                time_t now = time(nullptr);
                if (now - last_toast_time >= config.toast_interval_sec) {
                    notify_send_hud(hud_line1, hud_line2);
                    last_toast_time = now;
                }
            }
        }

        if (test_mode) {
            printf("[TEST] HUD Line 1: %s\n", hud_line1);
            printf("[TEST] HUD Line 2: %s\n", hud_line2);
            printf("[TEST] HUD Full:   %s\n", hud_text);
            break;
        }

        usleep(config.update_interval_ms * 1000);
    }

    printf("\n[STATUS] Shutting down PS5 Overlay daemon...\n");
    if (config.web_server_enabled) {
        web_server_stop();
    }
    overlay_ui_shutdown();
    monitor_cleanup();
    printf("[STATUS] Goodbye!\n");
    return 0;
}
