#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void config_set_defaults(OverlayConfig* config) {
    if (!config) return;
    config->enabled = true;
    config->show_cpu_temp = true;
    config->show_cpu_load = true;
    config->show_all_cores = false;
    config->show_gpu_temp = true;
    config->show_gpu_load = true;
    config->show_ram = true;
    config->show_fan = true;
    config->background_panel = true;
    config->position = 0;              /* 0 = Top, 1 = Bottom */
    config->font_size = 18;
    config->toast_notifications = true;
    config->toast_interval_sec = 4;
    config->web_server_enabled = true;
    config->web_port = 8080;
}

static char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static bool parse_bool(const char* val, bool default_val) {
    if (!val) return default_val;
    if (strcasecmp(val, "true") == 0 || strcasecmp(val, "1") == 0 || strcasecmp(val, "yes") == 0 || strcasecmp(val, "on") == 0) {
        return true;
    }
    if (strcasecmp(val, "false") == 0 || strcasecmp(val, "0") == 0 || strcasecmp(val, "no") == 0 || strcasecmp(val, "off") == 0) {
        return false;
    }
    return default_val;
}

bool config_load(OverlayConfig* config, const char* filepath) {
    config_set_defaults(config);
    if (!filepath) return false;

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim_whitespace(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';' || trimmed[0] == '[') {
            continue;
        }

        char* eq = strchr(trimmed, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = trim_whitespace(trimmed);
        char* val = trim_whitespace(eq + 1);

        if (strcasecmp(key, "enabled") == 0) {
            config->enabled = parse_bool(val, config->enabled);
        } else if (strcasecmp(key, "show_cpu_temp") == 0) {
            config->show_cpu_temp = parse_bool(val, config->show_cpu_temp);
        } else if (strcasecmp(key, "show_cpu_load") == 0) {
            config->show_cpu_load = parse_bool(val, config->show_cpu_load);
        } else if (strcasecmp(key, "show_all_cores") == 0) {
            config->show_all_cores = parse_bool(val, config->show_all_cores);
        } else if (strcasecmp(key, "show_gpu_temp") == 0 || strcasecmp(key, "show_soc_temp") == 0) {
            config->show_gpu_temp = parse_bool(val, config->show_gpu_temp);
        } else if (strcasecmp(key, "show_gpu_load") == 0 || strcasecmp(key, "show_vram") == 0) {
            config->show_gpu_load = parse_bool(val, config->show_gpu_load);
        } else if (strcasecmp(key, "show_ram") == 0) {
            config->show_ram = parse_bool(val, config->show_ram);
        } else if (strcasecmp(key, "show_fan") == 0) {
            config->show_fan = parse_bool(val, config->show_fan);
        } else if (strcasecmp(key, "background") == 0 || strcasecmp(key, "background_panel") == 0) {
            config->background_panel = parse_bool(val, config->background_panel);
        } else if (strcasecmp(key, "position") == 0) {
            if (strcasecmp(val, "bottom") == 0) {
                config->position = 1;
            } else {
                config->position = 0;
            }
        } else if (strcasecmp(key, "font_size") == 0) {
            int fs = atoi(val);
            if (fs >= 10 && fs <= 48) config->font_size = fs;
        } else if (strcasecmp(key, "interval_ms") == 0) {
            int interval = atoi(val);
            if (interval >= 100 && interval <= 10000) config->update_interval_ms = interval;
        } else if (strcasecmp(key, "toast_notifications") == 0) {
            config->toast_notifications = parse_bool(val, config->toast_notifications);
        } else if (strcasecmp(key, "toast_interval_sec") == 0) {
            int t = atoi(val);
            if (t >= 1 && t <= 3600) config->toast_interval_sec = t;
        } else if (strcasecmp(key, "web_server") == 0 || strcasecmp(key, "web_server_enabled") == 0) {
            config->web_server_enabled = parse_bool(val, config->web_server_enabled);
        } else if (strcasecmp(key, "web_port") == 0) {
            int p = atoi(val);
            if (p >= 1 && p <= 65535) config->web_port = p;
        }
    }

    fclose(fp);
    return true;
}

bool config_save(const OverlayConfig* config, const char* filepath) {
    if (!config || !filepath) return false;

    FILE* fp = fopen(filepath, "w");
    if (!fp) return false;

    fprintf(fp, "[ps5_overlay]\n");
    fprintf(fp, "enabled=%s\n", config->enabled ? "true" : "false");
    fprintf(fp, "position=%s\n", config->position == 1 ? "bottom" : "top");
    fprintf(fp, "show_cpu_temp=%s\n", config->show_cpu_temp ? "true" : "false");
    fprintf(fp, "show_cpu_load=%s\n", config->show_cpu_load ? "true" : "false");
    fprintf(fp, "show_all_cores=%s\n", config->show_all_cores ? "true" : "false");
    fprintf(fp, "show_gpu_temp=%s\n", config->show_gpu_temp ? "true" : "false");
    fprintf(fp, "show_gpu_load=%s\n", config->show_gpu_load ? "true" : "false");
    fprintf(fp, "show_ram=%s\n", config->show_ram ? "true" : "false");
    fprintf(fp, "show_fan=%s\n", config->show_fan ? "true" : "false");
    fprintf(fp, "background=%s\n", config->background_panel ? "true" : "false");
    fprintf(fp, "font_size=%d\n", config->font_size);
    fprintf(fp, "interval_ms=%d\n", config->update_interval_ms);
    fprintf(fp, "toast_notifications=%s\n", config->toast_notifications ? "true" : "false");
    fprintf(fp, "toast_interval_sec=%d\n", config->toast_interval_sec);
    fprintf(fp, "web_server=%s\n", config->web_server_enabled ? "true" : "false");
    fprintf(fp, "web_port=%d\n", config->web_port);

    fclose(fp);
    return true;
}
