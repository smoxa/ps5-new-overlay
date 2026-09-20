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
#include <cmath>

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

    bool ok = (mono_get_root_domain && mono_thread_attach && mono_class_from_name && mono_compile_method);
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

/* Patch Sony's UI thread check so our background thread can freely manipulate PUI widgets */
static void patch_main_thread_check(MonoDomain* domain) {
    MonoImage* core_img = load_system_dll(domain, "Sce.PlayStation.Core.dll");
    if (!core_img) {
        log_shellui("[SHELLUI] Sce.PlayStation.Core.dll not found\n");
        return;
    }
    MonoClass* diag_class = mono_class_from_name(core_img, "Sce.PlayStation.Core.Runtime", "Diagnostics");
    if (!diag_class) {
        log_shellui("[SHELLUI] Diagnostics class not found\n");
        return;
    }
    MonoMethod* check_method = mono_class_get_method_from_name(diag_class, "CheckRunningOnMainThread", 0);
    if (!check_method) {
        log_shellui("[SHELLUI] CheckRunningOnMainThread method not found\n");
        return;
    }
    uint64_t real_addr = (uint64_t)mono_compile_method(check_method);
    if (!real_addr) {
        log_shellui("[SHELLUI] Failed to compile CheckRunningOnMainThread\n");
        return;
    }

    uint64_t page_addr = real_addr & ~0x3FFFULL;
    if (kernel_mprotect(-1, page_addr, 0x4000, PROT_READ | PROT_WRITE | PROT_EXEC) == 0) {
        *(volatile uint8_t*)real_addr = 0xC3; // x86 'ret'
        kernel_mprotect(-1, page_addr, 0x4000, PROT_READ | PROT_EXEC);
        log_shellui("[SHELLUI] CheckRunningOnMainThread successfully patched with RET!\n");
    } else {
        log_shellui("[SHELLUI] kernel_mprotect failed for CheckRunningOnMainThread patch\n");
    }
}

/* Thunk-compiled direct property setter (as used in onionHEN) */
template <typename Param>
static void Set_Property(MonoClass* Klass, MonoObject* Instance, const char* Property_Name, Param Value)
{
    if (!Klass || !Instance) return;
    MonoProperty* Prop = mono_class_get_property_from_name(Klass, Property_Name);
    if (!Prop) return;
    MonoMethod* Set_Method = mono_property_get_set_method(Prop);
    if (!Set_Method) return;
    uint64_t Thunk = (uint64_t)mono_compile_method(Set_Method);
    if (!Thunk) return;
    void(*Method)(MonoObject*, Param) = (void(*)(MonoObject*, Param))Thunk;
    Method(Instance, Value);
}

/* Property setter via runtime invoke for object references */
template <typename Param>
static void Set_Property_Invoke(MonoClass* Klass, MonoObject* Instance, const char* Property_Name, Param Value)
{
    if (!Klass || !Instance) return;
    MonoProperty* Prop = mono_class_get_property_from_name(Klass, Property_Name);
    if (!Prop) return;
    MonoMethod* Set_Method = mono_property_get_set_method(Prop);
    if (!Set_Method) return;
    void* args[1] = { (void*)Value };
    mono_runtime_invoke(Set_Method, Instance, args, nullptr);
}

/* Thunk-compiled constructor caller for unboxed value types */
template <typename... Args>
static void Invoke_Ctor(MonoClass* klass, MonoObject* instance, Args... args)
{
    int count = sizeof...(args);
    MonoMethod* method = mono_class_get_method_from_name(klass, ".ctor", count);
    if (!method) return;
    uint64_t thunk = (uint64_t)mono_compile_method(method);
    if (!thunk) return;
    void(*fn)(MonoObject*, Args...) = (void(*)(MonoObject*, Args...))thunk;
    fn(instance, args...);
}

static MonoObject* create_ui_color(MonoImage* pui_img, MonoDomain* domain, float r, float g, float b, float a) {
    MonoClass* col_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI", "UIColor");
    if (!col_class) return nullptr;
    MonoObject* inst = mono_object_new(domain, col_class);
    if (!inst) return nullptr;
    MonoObject* unboxed = (MonoObject*)mono_object_unbox(inst);
    Invoke_Ctor(col_class, unboxed, r, g, b, a);
    return unboxed;
}

static MonoObject* create_ui_font(MonoImage* pui_img, MonoDomain* domain, int size, int style, int weight) {
    MonoClass* font_class = mono_class_from_name(pui_img, "Sce.PlayStation.PUI.UI2", "UIFont");
    if (!font_class) return nullptr;
    MonoObject* inst = mono_object_new(domain, font_class);
    if (!inst) return nullptr;
    MonoObject* unboxed = (MonoObject*)mono_object_unbox(inst);
    Invoke_Ctor(font_class, unboxed, size, style, weight);
    return unboxed;
}

static MonoObject* create_hud_label(MonoDomain* domain, MonoImage* pui_img, MonoClass* label_class,
                                    const char* name, float x, float y, const char* text,
                                    MonoObject* font, float r, float g, float b, float a = 1.0f) {
    MonoObject* label = mono_object_new(domain, label_class);
    if (!label) return nullptr;
    mono_runtime_object_init(label);

    Set_Property(label_class, label, "Name", mono_string_new(domain, name));
    Set_Property(label_class, label, "PositionType", 1);
    Set_Property(label_class, label, "MarginLeft", x);
    Set_Property(label_class, label, "MarginTop", y);
    Set_Property(label_class, label, "Text", mono_string_new(domain, text));
    if (font) {
        Set_Property_Invoke(label_class, label, "Font", font);
    }
    Set_Property(label_class, label, "HorizontalAlignment", 0);
    Set_Property(label_class, label, "VerticalAlignment", 0);
    Set_Property(label_class, label, "FitWidthToText", false);
    Set_Property(label_class, label, "FitHeightToText", true);
    Set_Property(label_class, label, "NumberOfLines", 1);
    Set_Property(label_class, label, "EnableThemedTextShadow", true);

    MonoObject* text_color = create_ui_color(pui_img, domain, r, g, b, a);
    if (text_color) {
        Set_Property_Invoke(label_class, label, "TextColor", text_color);
    }

    return label;
}

static void widget_append_child(MonoClass* widget_class, MonoObject* parent, MonoObject* child) {
    if (!widget_class || !parent || !child) return;
    MonoMethod* append_child = mono_class_get_method_from_name(widget_class, "AppendChild", 1);
    if (append_child) {
        void* args[1] = { child };
        mono_runtime_invoke(append_child, parent, args, nullptr);
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

    /* 2. Patch Sony's MainThread check so background UI modifications succeed */
    patch_main_thread_check(domain);

    /* 3. Load assemblies */
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

    /* 4. Get classes and methods */
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

    MonoObject* bg_panel = nullptr;
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
        MonoObject* exc = nullptr;
        MonoObject* game_scene = mono_runtime_invoke(find_scene, nullptr, scene_args, &exc);

        if (game_scene && game_scene != last_attached_scene) {
            log_shellui("[SHELLUI] Found active Game container scene: %p\n", game_scene);
            MonoObject* root_widget = mono_runtime_invoke(get_root, game_scene, nullptr, &exc);

            if (root_widget) {
                log_shellui("[SHELLUI] Game RootWidget: %p. Creating HUD widgets...\n", root_widget);

                /* Create HUD background panel */
                bg_panel = mono_object_new(domain, panel_class);
                mono_runtime_object_init(bg_panel);

                Set_Property(panel_class, bg_panel, "Name", mono_string_new(domain, "id_onion_overlay_bg"));
                Set_Property(panel_class, bg_panel, "X", 0.0f);
                Set_Property(panel_class, bg_panel, "Y", 0.0f);
                Set_Property(panel_class, bg_panel, "Width", 1920.0f);
                Set_Property(panel_class, bg_panel, "Height", 34.0f);

                MonoObject* bg_color = create_ui_color(pui_img, domain, 0.0f, 0.0f, 0.0f, 0.70f);
                if (bg_color) {
                    Set_Property_Invoke(panel_class, bg_panel, "BackgroundColor", bg_color);
                }

                Set_Property(panel_class, bg_panel, "BackgroundVisibility", true);
                Set_Property(panel_class, bg_panel, "BackgroundOpacity", 1.0f);
                Set_Property(panel_class, bg_panel, "BackgroundStyle", 1);

                log_shellui("[SHELLUI] Background panel created. Appending to RootWidget...\n");
                widget_append_child(widget_class, root_widget, bg_panel);

                /* Font: 18pt, bold=1, weight=900 */
                MonoObject* hud_font = create_ui_font(pui_img, domain, 18, 1, 900);
                log_shellui("[SHELLUI] Font created: %p\n", hud_font);

                float y = 5.0f;

                // CPU: #66FF66
                MonoObject* cpu_lbl = create_hud_label(domain, pui_img, label_class, "id_cpu_lbl", 24.0f, y, "CPU", hud_font, 102.0f/255.0f, 1.0f, 102.0f/255.0f);
                cpu_val = create_hud_label(domain, pui_img, label_class, "id_cpu_val", 70.0f, y, "--°C", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep1    = create_hud_label(domain, pui_img, label_class, "id_sep1", 136.0f, y, "|", hud_font, 0.75f, 0.75f, 0.75f);

                // GPU: #B366FF
                MonoObject* gpu_lbl = create_hud_label(domain, pui_img, label_class, "id_gpu_lbl", 156.0f, y, "GPU", hud_font, 179.0f/255.0f, 102.0f/255.0f, 1.0f);
                gpu_val = create_hud_label(domain, pui_img, label_class, "id_gpu_val", 204.0f, y, "--°C", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep2    = create_hud_label(domain, pui_img, label_class, "id_sep2", 270.0f, y, "|", hud_font, 0.75f, 0.75f, 0.75f);

                // RAM: #FFB34D
                MonoObject* ram_lbl = create_hud_label(domain, pui_img, label_class, "id_ram_lbl", 290.0f, y, "RAM", hud_font, 1.0f, 179.0f/255.0f, 77.0f/255.0f);
                ram_val = create_hud_label(domain, pui_img, label_class, "id_ram_val", 340.0f, y, "N/A", hud_font, 1.0f, 1.0f, 1.0f);
                MonoObject* sep3    = create_hud_label(domain, pui_img, label_class, "id_sep3", 440.0f, y, "|", hud_font, 0.75f, 0.75f, 0.75f);

                // FAN: #33E0FF
                MonoObject* fan_lbl = create_hud_label(domain, pui_img, label_class, "id_fan_lbl", 460.0f, y, "FAN", hud_font, 51.0f/255.0f, 224.0f/255.0f, 1.0f);
                fan_val = create_hud_label(domain, pui_img, label_class, "id_fan_val", 506.0f, y, "--%", hud_font, 1.0f, 1.0f, 1.0f);

                widget_append_child(widget_class, root_widget, cpu_lbl);
                widget_append_child(widget_class, root_widget, cpu_val);
                widget_append_child(widget_class, root_widget, sep1);

                widget_append_child(widget_class, root_widget, gpu_lbl);
                widget_append_child(widget_class, root_widget, gpu_val);
                widget_append_child(widget_class, root_widget, sep2);

                widget_append_child(widget_class, root_widget, ram_lbl);
                widget_append_child(widget_class, root_widget, ram_val);
                widget_append_child(widget_class, root_widget, sep3);

                widget_append_child(widget_class, root_widget, fan_lbl);
                widget_append_child(widget_class, root_widget, fan_val);

                /* Center test label requested by user */
                MonoObject* center_font = create_ui_font(pui_img, domain, 36, 1, 900);
                MonoObject* center_lbl = create_hud_label(domain, pui_img, label_class, "id_center_test",
                                                          640.0f, 480.0f, "PS5 OVERLAY ACTIVE",
                                                          center_font, 1.0f, 0.90f, 0.0f);
                widget_append_child(widget_class, root_widget, center_lbl);
                log_shellui("[SHELLUI] Center test banner added!\n");

                last_attached_scene = game_scene;
                attached_to_game = true;
                log_shellui("[SHELLUI] HUD attached to Game Scene RootWidget successfully!\n");
            }
        } else if (!game_scene && attached_to_game) {
            /* Game closed */
            log_shellui("[SHELLUI] Game closed, waiting for next game...\n");
            attached_to_game = false;
            last_attached_scene = nullptr;
        }

        /* Update metrics if attached */
        if (attached_to_game) {
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
                Set_Property(label_class, cpu_val, "Text", mono_string_new(domain, buf));
            }

            if (gpu_val) {
                snprintf(buf, sizeof(buf), "%d°C", gpu_temp);
                Set_Property(label_class, gpu_val, "Text", mono_string_new(domain, buf));
            }

            if (fan_val) {
                snprintf(buf, sizeof(buf), "%.0f%%", fan_pct);
                Set_Property(label_class, fan_val, "Text", mono_string_new(domain, buf));
            }
        }

        sleep(1);
    }
#else
    printf("[SHELLUI_OVERLAY] Host test stub.\n");
#endif

    return 0;
}
