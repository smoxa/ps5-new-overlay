#include "monitor.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__PS5__) || defined(PS5)

struct OrbisKernelTimespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

struct ProcStats {
    int32_t lo_data;
    uint32_t td_tid;
    OrbisKernelTimespec user_cpu_usage_time;
    OrbisKernelTimespec system_cpu_usage_time;
};

extern "C" {
    int sceKernelGetCpuTemperature(int* temp);
    int sceKernelGetSocSensorTemperature(int sensorId, int* temp);
    int sceKernelGetCurrentFanDuty(uint16_t* duty, uint64_t* chassis);
    int get_page_table_stats(int vm, int type, int* total, int* free);
    int sceKernelGetCpuUsage(struct ProcStats* out, int32_t* size);
    int sceKernelClockGettime(int clockId, struct OrbisKernelTimespec* tp);
}

#define VM_SYSTEM 1
#define PAGE_TABLE_RAM 1
#define PAGE_TABLE_VRAM 2
#define CLOCK_ID_REALTIME 4
#define MAX_PROC_THREADS 3072
#define NUM_CPU_CORES 8

struct ThreadSnapshot {
    OrbisKernelTimespec timestamp;
    int thread_count;
    ProcStats threads[MAX_PROC_THREADS];
};

static ThreadSnapshot s_snapshots[2];
static int s_current_bank = 0;
static bool s_has_prev_snapshot = false;

#endif

bool monitor_init(void) {
#if defined(__PS5__) || defined(PS5)
    s_current_bank = 0;
    s_has_prev_snapshot = false;
    memset(s_snapshots, 0, sizeof(s_snapshots));
#endif
    return true;
}

void monitor_cleanup(void) {
}

#if defined(__PS5__) || defined(PS5)

static void compute_cpu_usage(ThreadSnapshot* cur, ThreadSnapshot* prev, float* total_usage, float* core_usage) {
    *total_usage = 0.0f;
    for (int i = 0; i < NUM_CPU_CORES; i++) {
        core_usage[i] = 0.0f;
    }

    if (!cur || !prev || cur->thread_count <= 0 || prev->thread_count <= 0) {
        return;
    }

    double dt = (double)(cur->timestamp.tv_sec - prev->timestamp.tv_sec) +
                (double)(cur->timestamp.tv_nsec - prev->timestamp.tv_nsec) * 1e-9;

    if (dt <= 0.001) {
        return;
    }

    /* Sum active CPU time across all threads */
    double total_active_cpu_time = 0.0;

    for (int i = 0; i < cur->thread_count; i++) {
        uint32_t tid = cur->threads[i].td_tid;
        for (int j = 0; j < prev->thread_count; j++) {
            if (prev->threads[j].td_tid == tid) {
                double cur_time = (double)cur->threads[i].user_cpu_usage_time.tv_sec +
                                  (double)cur->threads[i].user_cpu_usage_time.tv_nsec * 1e-9 +
                                  (double)cur->threads[i].system_cpu_usage_time.tv_sec +
                                  (double)cur->threads[i].system_cpu_usage_time.tv_nsec * 1e-9;

                double prev_time = (double)prev->threads[j].user_cpu_usage_time.tv_sec +
                                   (double)prev->threads[j].user_cpu_usage_time.tv_nsec * 1e-9 +
                                   (double)prev->threads[j].system_cpu_usage_time.tv_sec +
                                   (double)prev->threads[j].system_cpu_usage_time.tv_nsec * 1e-9;

                double delta = cur_time - prev_time;
                if (delta > 0.0) {
                    total_active_cpu_time += delta;
                }
                break;
            }
        }
    }

    /* PS5 has 8 Zen2 cores (16 threads). System CPU load over 8 cores: */
    double usage_pct = (total_active_cpu_time / (dt * (double)NUM_CPU_CORES)) * 100.0;
    if (usage_pct > 100.0) usage_pct = 100.0;
    if (usage_pct < 0.0) usage_pct = 0.0;

    *total_usage = (float)usage_pct;

    /* Distribute estimation across cores if needed */
    for (int i = 0; i < NUM_CPU_CORES; i++) {
        core_usage[i] = *total_usage;
    }
}

#endif

bool monitor_update(HardwareMetrics* metrics) {
    if (!metrics) return false;

#if defined(__PS5__) || defined(PS5)
    /* 1. CPU Temperature */
    int cpu_t = 0;
    if (sceKernelGetCpuTemperature(&cpu_t) == 0) {
        metrics->cpu_temp = cpu_t;
    } else {
        metrics->cpu_temp = 0;
    }

    /* 2. SOC / APU / GPU Temperature */
    int soc_t = 0;
    if (sceKernelGetSocSensorTemperature(0, &soc_t) == 0) {
        metrics->soc_temp = soc_t;
    } else {
        metrics->soc_temp = 0;
    }

    /* 3. System RAM (Memory) */
    int ram_total = 0, ram_free = 0;
    if (get_page_table_stats(VM_SYSTEM, PAGE_TABLE_RAM, &ram_total, &ram_free) == 0) {
        metrics->ram_total_mb = ram_total;
        metrics->ram_used_mb = ram_total - ram_free;
        metrics->ram_percentage = (ram_total > 0) ? ((float)metrics->ram_used_mb / (float)ram_total) * 100.0f : 0.0f;
    }

    /* 4. VRAM (Video Memory) */
    int vram_total = 0, vram_free = 0;
    if (get_page_table_stats(VM_SYSTEM, PAGE_TABLE_VRAM, &vram_total, &vram_free) == 0) {
        metrics->vram_total_mb = vram_total;
        metrics->vram_used_mb = vram_total - vram_free;
        metrics->vram_percentage = (vram_total > 0) ? ((float)metrics->vram_used_mb / (float)vram_total) * 100.0f : 0.0f;
    }

    /* 5. Fan Duty (Speed) */
    uint16_t duty = 0;
    uint64_t chassis = 0;
    if (sceKernelGetCurrentFanDuty && sceKernelGetCurrentFanDuty(&duty, &chassis) == 0) {
        /* Duty is out of 1024 */
        metrics->fan_duty_percent = (int)(((double)duty * 100.0) / 1024.0);
    } else {
        metrics->fan_duty_percent = 0;
    }

    /* 6. CPU Usage */
    ThreadSnapshot* cur = &s_snapshots[s_current_bank];
    cur->thread_count = MAX_PROC_THREADS;
    if (sceKernelGetCpuUsage(cur->threads, &cur->thread_count) == 0) {
        sceKernelClockGettime(CLOCK_ID_REALTIME, &cur->timestamp);
        if (s_has_prev_snapshot) {
            ThreadSnapshot* prev = &s_snapshots[!s_current_bank];
            compute_cpu_usage(cur, prev, &metrics->cpu_usage, metrics->cpu_core_usage);
        } else {
            metrics->cpu_usage = 0.0f;
            s_has_prev_snapshot = true;
        }
        s_current_bank = !s_current_bank;
    }

#else
    /* Mock data for non-PS5 host development/testing */
    metrics->cpu_temp = 58;
    metrics->soc_temp = 62;
    metrics->cpu_usage = 23.5f;
    for (int i = 0; i < 8; i++) metrics->cpu_core_usage[i] = 20.0f + (float)i;
    metrics->ram_used_mb = 4320;
    metrics->ram_total_mb = 16384;
    metrics->ram_percentage = 26.3f;
    metrics->vram_used_mb = 3100;
    metrics->vram_total_mb = 8192;
    metrics->vram_percentage = 37.8f;
    metrics->fan_duty_percent = 35;
#endif

    return true;
}

void monitor_format_hud_string(const HardwareMetrics* m, const OverlayConfig* cfg, char* buffer, size_t max_len) {
    if (!m || !cfg || !buffer || max_len == 0) return;

    buffer[0] = '\0';
    char item[128];
    bool first = true;

    auto append_sep = [&]() {
        if (!first) {
            strncat(buffer, "  |  ", max_len - strlen(buffer) - 1);
        }
        first = false;
    };

    /* CPU temp & load */
    if (cfg->show_cpu_temp || cfg->show_cpu_load) {
        append_sep();
        strncat(buffer, "CPU: ", max_len - strlen(buffer) - 1);

        if (cfg->show_cpu_temp) {
            snprintf(item, sizeof(item), "%d°C", m->cpu_temp);
            strncat(buffer, item, max_len - strlen(buffer) - 1);
        }

        if (cfg->show_cpu_load) {
            if (cfg->show_cpu_temp) strncat(buffer, " ", max_len - strlen(buffer) - 1);
            if (cfg->show_all_cores) {
                snprintf(item, sizeof(item), "[%.0f%% %.0f%% %.0f%% %.0f%% %.0f%% %.0f%% %.0f%% %.0f%%]",
                         m->cpu_core_usage[0], m->cpu_core_usage[1], m->cpu_core_usage[2], m->cpu_core_usage[3],
                         m->cpu_core_usage[4], m->cpu_core_usage[5], m->cpu_core_usage[6], m->cpu_core_usage[7]);
            } else {
                snprintf(item, sizeof(item), "%.0f%%", m->cpu_usage);
            }
            strncat(buffer, item, max_len - strlen(buffer) - 1);
        }
    }

    /* APU / GPU temp & VRAM */
    if (cfg->show_gpu_temp || cfg->show_gpu_load) {
        append_sep();
        strncat(buffer, "GPU: ", max_len - strlen(buffer) - 1);

        if (cfg->show_gpu_temp) {
            snprintf(item, sizeof(item), "%d°C", m->soc_temp);
            strncat(buffer, item, max_len - strlen(buffer) - 1);
        }

        if (cfg->show_gpu_load) {
            if (cfg->show_gpu_temp) strncat(buffer, " ", max_len - strlen(buffer) - 1);
            snprintf(item, sizeof(item), "%.0f%%", m->vram_percentage);
            strncat(buffer, item, max_len - strlen(buffer) - 1);
        }
    }

    /* RAM usage */
    if (cfg->show_ram) {
        append_sep();
        snprintf(item, sizeof(item), "RAM: %d MB (%.0f%%)", m->ram_used_mb, m->ram_percentage);
        strncat(buffer, item, max_len - strlen(buffer) - 1);
    }

    /* Fan speed */
    if (cfg->show_fan) {
        append_sep();
        snprintf(item, sizeof(item), "FAN: %d%%", m->fan_duty_percent);
        strncat(buffer, item, max_len - strlen(buffer) - 1);
    }
}
