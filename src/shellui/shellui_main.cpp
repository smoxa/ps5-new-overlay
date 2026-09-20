/*
 * ps5_overlay_shellui: Native on-screen in-game HUD overlay for PS5 SceShellUI.
 * Injected into SceShellUI via ptrace and rendered over the "Game" container scene
 * using Sce.PlayStation.PUI.UI2 widgets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#if defined(__PS5__) || defined(PS5)
#include <ps5/kernel.h>
#include <sys/syscall.h>

static void log_shellui(const char* fmt, ...) {
    FILE* fp = fopen("/system_tmp/ps5_overlay.log", "a");
    if (fp) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(fp, fmt, ap);
        va_end(ap);
        fclose(fp);
    }
}

typedef void* MonoDomain;
typedef void* MonoAssembly;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoObject;
typedef void* MonoMethod;
typedef void* MonoString;
typedef void* MonoProperty;

/* Function pointer definitions for Mono runtime (loaded from libmonosgen-2.0.sprx) */
static MonoDomain* (*mono_get_root_domain)(void) = nullptr;
static void* (*mono_thread_attach)(MonoDomain* domain) = nullptr;
static MonoDomain* (*mono_domain_get)(void) = nullptr;
static MonoAssembly* (*mono_domain_assembly_open)(MonoDomain* domain, const char* name) = nullptr;
static MonoImage* (*mono_assembly_get_image)(MonoAssembly* assembly) = nullptr;
static MonoClass* (*mono_class_from_name)(MonoImage* image, const char* name_space, const char* name) = nullptr;
static MonoMethod* (*mono_class_get_method_from_name)(MonoClass* klass, const char* name, int param_count) = nullptr;
static MonoProperty* (*mono_class_get_property_from_name)(MonoClass* klass, const char* name) = nullptr;
static MonoMethod* (*mono_property_get_get_method)(MonoProperty* prop) = nullptr;
static MonoMethod* (*mono_property_get_set_method)(MonoProperty* prop) = nullptr;
static MonoObject* (*mono_runtime_invoke)(MonoMethod* method, void* obj, void** params, MonoObject** exc) = nullptr;
static MonoString* (*mono_string_new)(MonoDomain* domain, const char* text) = nullptr;
static MonoObject* (*mono_object_new)(MonoDomain* domain, MonoClass* klass) = nullptr;
static void* (*mono_object_unbox)(MonoObject* obj) = nullptr;
static void (*mono_runtime_object_init)(MonoObject* obj) = nullptr;
static uint64_t (*mono_compile_method)(MonoMethod* method) = nullptr;

/* Hardware monitor functions */
static int (*sys_sceKernelGetCpuTemperature)(int* cputemp) = nullptr;
static int (*sys_sceKernelGetSocSensorTemperature)(int sensorId, int* soctime) = nullptr;
static int (*sys_sceKernelGetCurrentFanDuty)(uint16_t* duty, uint64_t* chassis) = nullptr;

#define KERNEL_DLSYM(handle, sym) \
    (*(void**)&sym = (void*)kernel_dynlib_dlsym(-1, handle, #sym))

static int get_module_handle_internal(const char* name) {
    uint32_t handle = 0;
    if (kernel_dynlib_handle(-1, name, &handle) == 0 && handle != 0) {
        return (int)handle;
    }
    return 0;
}

static bool resolve_mono_symbols(void) {
    int libmono = get_module_handle_internal("libmonosgen-2.0.sprx");
    if (!libmono) {
        log_shellui("[SHELLUI] libmonosgen-2.0.sprx handle not found!\n");
        return false;
    }
    log_shellui("[SHELLUI] Found libmonosgen-2.0.sprx handle: 0x%x\n", libmono);

    KERNEL_DLSYM(libmono, mono_get_root_domain);
    KERNEL_DLSYM(libmono, mono_thread_attach);
    KERNEL_DLSYM(libmono, mono_domain_get);
    KERNEL_DLSYM(libmono, mono_domain_assembly_open);
    KERNEL_DLSYM(libmono, mono_assembly_get_image);
    KERNEL_DLSYM(libmono, mono_class_from_name);
    KERNEL_DLSYM(libmono, mono_class_get_method_from_name);
    KERNEL_DLSYM(libmono, mono_class_get_property_from_name);
    KERNEL_DLSYM(libmono, mono_property_get_get_method);
    KERNEL_DLSYM(libmono, mono_property_get_set_method);
    KERNEL_DLSYM(libmono, mono_runtime_invoke);
    KERNEL_DLSYM(libmono, mono_string_new);
    KERNEL_DLSYM(libmono, mono_object_new);
    KERNEL_DLSYM(libmono, mono_object_unbox);
    KERNEL_DLSYM(libmono, mono_runtime_object_init);
    KERNEL_DLSYM(libmono, mono_compile_method);

    int libkernel = get_module_handle_internal("libkernel_sys.sprx");
    if (!libkernel) libkernel = 0x2001;

    sys_sceKernelGetCpuTemperature = (int(*)(int*))kernel_dynlib_dlsym(-1, libkernel, "sceKernelGetCpuTemperature");
    sys_sceKernelGetSocSensorTemperature = (int(*)(int, int*))kernel_dynlib_dlsym(-1, libkernel, "sceKernelGetSocSensorTemperature");
    sys_sceKernelGetCurrentFanDuty = (int(*)(uint16_t*, uint64_t*))kernel_dynlib_dlsym(-1, libkernel, "sceKernelGetCurrentFanDuty");

    bool ok = (mono_get_root_domain && mono_thread_attach && mono_class_from_name);
    log_shellui("[SHELLUI] resolve_mono_symbols result: %s\n", ok ? "SUCCESS" : "FAILED");
    return ok;
}

static MonoImage* load_system_dll(MonoDomain* domain, const char* dll_name) {
    char path[256];
    snprintf(path, sizeof(path), "/system_ex/common_ex/lib/%s", dll_name);
    MonoAssembly* assm = mono_domain_assembly_open(domain, path);
    if (!assm) {
        assm = mono_domain_assembly_open(domain, dll_name);
    }
    if (!assm) return nullptr;
    return mono_assembly_get_image(assm);
}

static MonoObject* invoke_method(MonoMethod* method, void* obj, void** params) {
    if (!method || !mono_runtime_invoke) return nullptr;
    MonoObject* exc = nullptr;
    return mono_runtime_invoke(method, obj, params, &exc);
}

static void set_property_string(MonoDomain* domain, MonoClass* cls, MonoObject* obj, const char* prop_name, const char* val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    MonoString* str = mono_string_new(domain, val);
    void* args[1] = { str };
    invoke_method(setter, obj, args);
}

static void set_property_float(MonoClass* cls, MonoObject* obj, const char* prop_name, float val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    void* args[1] = { &val };
    invoke_method(setter, obj, args);
}

static void set_property_bool(MonoClass* cls, MonoObject* obj, const char* prop_name, bool val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    uint32_t bval = val ? 1 : 0;
    void* args[1] = { &bval };
    invoke_method(setter, obj, args);
}

static void set_property_int(MonoClass* cls, MonoObject* obj, const char* prop_name, int val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    void* args[1] = { &val };
    invoke_method(setter, obj, args);
}

static void set_property_object(MonoClass* cls, MonoObject* obj, const char* prop_name, MonoObject* val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    void* args[1] = { val };
    invoke_method(setter, obj, args);
}

static MonoObject* create_ui_color(MonoImage* pui_img, MonoDomain* domain, float r, float g, float b, float a) {
    MonoClass* col_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI", "UIColor");
    if (!col_class) return nullptr;
    MonoObject* inst = mono_object_new(domain, col_class);
    if (!inst) return nullptr;
    MonoObject* unboxed = (MonoObject*)mono_object_unbox(inst);
    MonoMethod* ctor = mono_class_get_method_from_name(col_class, ".ctor", 4);
    if (ctor) {
        void* args[4] = { &r, &g, &b, &a };
        invoke_method(ctor, unboxed, args);
    }
    return unboxed ? unboxed : inst;
}

static MonoObject* create_ui_font(MonoImage* pui_img, MonoDomain* domain, int size, int style, int weight) {
    MonoClass* font_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "UIFont");
    if (!font_class) return nullptr;
    MonoObject* inst = mono_object_new(domain, font_class);
    if (!inst) return nullptr;
    MonoObject* unboxed = (MonoObject*)mono_object_unbox(inst);
    MonoMethod* ctor = mono_class_get_method_from_name(font_class, ".ctor", 3);
    if (ctor) {
        void* args[3] = { &size, &style, &weight };
        invoke_method(ctor, unboxed, args);
    }
    return unboxed ? unboxed : inst;
}

static MonoObject* create_hud_label(MonoDomain* domain, MonoImage* pui_img, MonoClass* label_class,
                                    const char* name, float x, const char* text,
                                    MonoObject* font, float r, float g, float b) {
    MonoObject* label = mono_object_new(domain, label_class);
    if (!label) return nullptr;
    mono_runtime_object_init(label);

    set_property_string(domain, label_class, label, "Name", name);
    set_property_int(label_class, label, "PositionType", 1);
    set_property_float(label_class, label, "MarginLeft", x);
    set_property_float(label_class, label, "MarginTop", 5.0f);
    set_property_string(domain, label_class, label, "Text", text);
    if (font) {
        set_property_object(label_class, label, "Font", font);
    }
    set_property_int(label_class, label, "HorizontalAlignment", 0);
    set_property_int(label_class, label, "VerticalAlignment", 0);
    set_property_bool(label_class, label, "FitWidthToText", true);
    set_property_bool(label_class, label, "FitHeightToText", true);
    set_property_int(label_class, label, "NumberOfLines", 1);
    set_property_bool(label_class, label, "EnableThemedTextShadow", true);

    MonoObject* text_color = create_ui_color(pui_img, domain, r, g, b, 1.0f);
    if (text_color) {
        set_property_object(label_class, label, "TextColor", text_color);
    }

    return label;
}

static void append_widget(MonoClass* widget_class, MonoObject* parent, MonoObject* child) {
    if (!widget_class || !parent || !child) return;
    MonoMethod* append_child = mono_class_get_method_from_name(widget_class, "AppendChild", 1);
    if (append_child) {
        void* args[1] = { child };
        invoke_method(append_child, parent, args);
    }
}
#endif

int main(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;

#if defined(__PS5__) || defined(PS5)
    log_shellui("[SHELLUI] Overlay thread started in PID %d\n", getpid());

    /* 1. Resolve Mono runtime functions */
    while (!resolve_mono_symbols()) {
        log_shellui("[SHELLUI] Waiting for Mono symbols...\n");
        sleep(1);
    }

    MonoDomain* domain = mono_get_root_domain();
    if (!domain) {
        log_shellui("[SHELLUI] mono_get_root_domain returned null!\n");
        return -1;
    }
    mono_thread_attach(domain);
    log_shellui("[SHELLUI] Attached to Mono root domain: %p\n", domain);

    /* 2. Load assemblies */
    MonoImage* pui_img = nullptr;
    MonoImage* app_system_img = nullptr;
    while (!pui_img || !app_system_img) {
        pui_img = load_system_dll(domain, "Sce.PlayStation.PUI.dll");
        app_system_img = load_system_dll(domain, "Sce.Vsh.ShellUI.AppSystem.dll");
        if (!pui_img || !app_system_img) {
            log_shellui("[SHELLUI] Waiting for PUI / AppSystem assemblies...\n");
            sleep(1);
        }
    }
    log_shellui("[SHELLUI] Loaded PUI (%p) and AppSystem (%p)\n", pui_img, app_system_img);

    /* 3. Get classes and methods */
    MonoClass* layer_mgr_class = mono_class_from_name(app_system_img, "Sce.Vsh.ShellUI.AppSystem", "LayerManager");
    MonoClass* scene_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "Scene");
    MonoClass* widget_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "Widget");
    MonoClass* panel_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "Panel");
    MonoClass* label_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "Label");

    if (!layer_mgr_class || !scene_class || !widget_class || !panel_class || !label_class) {
        log_shellui("[SHELLUI] Required PUI classes not found!\n");
        return -1;
    }

    MonoMethod* find_scene = mono_class_get_method_from_name(layer_mgr_class, "FindContainerSceneByPath", 1);
    MonoProperty* root_prop = mono_class_get_property_from_name(scene_class, "RootWidget");
    MonoMethod* get_root = root_prop ? mono_property_get_get_method(root_prop) : nullptr;

    if (!find_scene || !get_root) {
        log_shellui("[SHELLUI] FindContainerSceneByPath or get_RootWidget method not found!\n");
        return -1;
    }

    /* Signal initial readiness */
    FILE* fp = fopen("/system_tmp/ps5_overlay_ready", "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }

    log_shellui("[SHELLUI] Overlay ready! Entering game monitoring loop...\n");

    MonoObject* hud_panel = nullptr;
    MonoObject* cpu_val = nullptr;
    MonoObject* gpu_val = nullptr;
    MonoObject* ram_val = nullptr;
    MonoObject* fan_val = nullptr;
    bool attached_to_game = false;
    MonoObject* last_attached_scene = nullptr;

    /* Continuous monitoring loop: attaches to Game scene whenever active */
    while (true) {
        MonoString* game_str = mono_string_new(domain, "Game");
        void* scene_args[1] = { game_str };
        MonoObject* game_scene = invoke_method(find_scene, nullptr, scene_args);

        if (game_scene && game_scene != last_attached_scene) {
            log_shellui("[SHELLUI] Found active Game container scene: %p\n", game_scene);
            MonoObject* root_widget = invoke_method(get_root, game_scene, nullptr);

            if (root_widget) {
                log_shellui("[SHELLUI] Game RootWidget: %p. Creating HUD widgets...\n", root_widget);

                /* Create HUD panel */
                hud_panel = mono_object_new(domain, panel_class);
                mono_runtime_object_init(hud_panel);

                set_property_string(domain, panel_class, hud_panel, "Name", "ps5_in_game_hud_panel");
                set_property_float(panel_class, hud_panel, "X", 0.0f);
                set_property_float(panel_class, hud_panel, "Y", 0.0f);
                set_property_float(panel_class, hud_panel, "Width", 1920.0f);
                set_property_float(panel_class, hud_panel, "Height", 34.0f);
                set_property_bool(panel_class, hud_panel, "BackgroundVisibility", true);
                set_property_float(panel_class, hud_panel, "BackgroundOpacity", 0.72f);
                set_property_int(panel_class, hud_panel, "BackgroundStyle", 1);

                MonoObject* bg_color = create_ui_color(pui_img, domain, 0.0f, 0.0f, 0.0f, 0.72f);
                if (bg_color) {
                    set_property_object(panel_class, hud_panel, "BackgroundColor", bg_color);
                }

                MonoObject* hud_font = create_ui_font(pui_img, domain, 18, 1, 900);

                // CPU
                MonoObject* cpu_lbl = create_hud_label(domain, pui_img, label_class, "id_cpu_tag", 28.0f, "CPU", hud_font, 0.40f, 1.0f, 0.40f);
                cpu_val = create_hud_label(domain, pui_img, label_class, "id_cpu_val", 72.0f, "--°C", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep1    = create_hud_label(domain, pui_img, label_class, "id_sep1", 132.0f, "|", hud_font, 0.60f, 0.60f, 0.60f);

                // GPU
                MonoObject* gpu_lbl = create_hud_label(domain, pui_img, label_class, "id_gpu_tag", 152.0f, "GPU", hud_font, 0.70f, 0.40f, 1.0f);
                gpu_val = create_hud_label(domain, pui_img, label_class, "id_gpu_val", 196.0f, "--°C", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep2    = create_hud_label(domain, pui_img, label_class, "id_sep2", 256.0f, "|", hud_font, 0.60f, 0.60f, 0.60f);

                // RAM
                MonoObject* ram_lbl = create_hud_label(domain, pui_img, label_class, "id_ram_tag", 276.0f, "RAM", hud_font, 1.0f, 0.70f, 0.30f);
                ram_val = create_hud_label(domain, pui_img, label_class, "id_ram_val", 324.0f, "N/A", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep3    = create_hud_label(domain, pui_img, label_class, "id_sep3", 420.0f, "|", hud_font, 0.60f, 0.60f, 0.60f);

                // FAN
                MonoObject* fan_lbl = create_hud_label(domain, pui_img, label_class, "id_fan_tag", 440.0f, "FAN", hud_font, 0.20f, 0.88f, 1.0f);
                fan_val = create_hud_label(domain, pui_img, label_class, "id_fan_val", 486.0f, "--%", hud_font, 1.0f, 1.0f, 1.0f);

                append_widget(widget_class, hud_panel, cpu_lbl);
                append_widget(widget_class, hud_panel, cpu_val);
                append_widget(widget_class, hud_panel, sep1);

                append_widget(widget_class, hud_panel, gpu_lbl);
                append_widget(widget_class, hud_panel, gpu_val);
                append_widget(widget_class, hud_panel, sep2);

                append_widget(widget_class, hud_panel, ram_lbl);
                append_widget(widget_class, hud_panel, ram_val);
                append_widget(widget_class, hud_panel, sep3);

                append_widget(widget_class, hud_panel, fan_lbl);
                append_widget(widget_class, hud_panel, fan_val);

                /* Attach to RootWidget */
                append_widget(widget_class, root_widget, hud_panel);

                last_attached_scene = game_scene;
                attached_to_game = true;
                log_shellui("[SHELLUI] HUD attached to Game Scene RootWidget!\n");
            }
        } else if (!game_scene && attached_to_game) {
            /* Game closed */
            log_shellui("[SHELLUI] Game closed, waiting for next game...\n");
            attached_to_game = false;
            last_attached_scene = nullptr;
        }

        /* Update metrics if attached */
        if (attached_to_game && hud_panel) {
            int cpu_temp = 0;
            if (sys_sceKernelGetCpuTemperature) {
                sys_sceKernelGetCpuTemperature(&cpu_temp);
            }

            int gpu_temp = 0;
            if (sys_sceKernelGetSocSensorTemperature) {
                sys_sceKernelGetSocSensorTemperature(0, &gpu_temp);
            }

            uint16_t fan_duty = 0;
            uint64_t chassis = 0;
            double fan_pct = 0.0;
            if (sys_sceKernelGetCurrentFanDuty && sys_sceKernelGetCurrentFanDuty(&fan_duty, &chassis) == 0) {
                fan_pct = ((double)fan_duty * 100.0) / 1024.0;
            }

            char buf[32];
            if (cpu_val) {
                snprintf(buf, sizeof(buf), "%d°C", cpu_temp);
                set_property_string(domain, label_class, cpu_val, "Text", buf);
            }

            if (gpu_val) {
                snprintf(buf, sizeof(buf), "%d°C", gpu_temp);
                set_property_string(domain, label_class, gpu_val, "Text", buf);
            }

            if (fan_val) {
                snprintf(buf, sizeof(buf), "%.0f%%", fan_pct);
                set_property_string(domain, label_class, fan_val, "Text", buf);
            }
        }

        sleep(1);
    }
#else
    printf("[SHELLUI_OVERLAY] Host test stub.\n");
#endif

    return 0;
}
