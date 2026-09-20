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
#include <sys/mman.h>
#include <ps5/kernel.h>
#include <sys/syscall.h>

#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_EXEC
#define PROT_EXEC 0x4
#endif

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
static int (*sys_sceKernelMprotect)(void* addr, size_t len, int prot) = nullptr;
static int (*sys_get_page_table_stats)(int vm, int type, int* total, int* free) = nullptr;

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
    sys_sceKernelMprotect = (int(*)(void*, size_t, int))kernel_dynlib_dlsym(-1, libkernel, "sceKernelMprotect");
    sys_get_page_table_stats = (int(*)(int, int, int*, int*))kernel_dynlib_dlsym(-1, libkernel, "get_page_table_stats");

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
    log_shellui("[SHELLUI] Attempting to patch CheckRunningOnMainThread...\n");
    MonoImage* core_img = nullptr;
    for (int retry = 0; retry < 10 && !core_img; retry++) {
        core_img = load_system_dll(domain, "Sce.PlayStation.Core.dll");
        if (!core_img) {
            log_shellui("[SHELLUI] Waiting for Sce.PlayStation.Core.dll (%d/10)...\n", retry + 1);
            sleep(1);
        }
    }
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
    log_shellui("[SHELLUI] CheckRunningOnMainThread address: 0x%lx\n", real_addr);

    uint64_t page_addr = real_addr & ~0x3FFFULL;
    int r1 = sys_sceKernelMprotect ? sys_sceKernelMprotect((void*)page_addr, 0x4000, PROT_READ | PROT_WRITE | PROT_EXEC) : -1;
    if (r1 == 0) {
        *(volatile uint8_t*)real_addr = 0xC3; // x86 'ret'
        sys_sceKernelMprotect((void*)page_addr, 0x4000, PROT_READ | PROT_EXEC);
        log_shellui("[SHELLUI] CheckRunningOnMainThread successfully patched via sceKernelMprotect!\n");
        return;
    }
    int r2 = kernel_mprotect(getpid(), page_addr, 0x4000, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (r2 == 0) {
        *(volatile uint8_t*)real_addr = 0xC3; // x86 'ret'
        kernel_mprotect(getpid(), page_addr, 0x4000, PROT_READ | PROT_EXEC);
        log_shellui("[SHELLUI] CheckRunningOnMainThread successfully patched via kernel_mprotect!\n");
        return;
    }
    log_shellui("[SHELLUI] Failed to patch CheckRunningOnMainThread! (r1=%d, r2=%d)\n", r1, r2);
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
    MonoObject* exc = nullptr;
    mono_runtime_invoke(Set_Method, Instance, args, &exc);
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

static MonoObject* create_ui_color(MonoImage* pui_img, MonoDomain* domain, float r, float g, float b, float a = 1.0f) {
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

static void widget_append_child(MonoClass* widget_class, MonoObject* parent, MonoObject* child) {
    if (!widget_class || !parent || !child) return;
    MonoMethod* append_child = mono_class_get_method_from_name(widget_class, "AppendChild", 1);
    if (append_child) {
        void* args[1] = { child };
        MonoObject* exc = nullptr;
        mono_runtime_invoke(append_child, parent, args, &exc);
    }
}

/*
 * Proven HUD architecture (v1.0.8 baseline):
 * - Exactly 1 background panel attached to RootWidget
 * - Exactly 11 container cells (Panel) attached to RootWidget
 * - Exactly 1 Label inside each container cell
 * - Exactly 1 UIFont instance (18pt, bold=1, weight=900)
 * - Zero nested panels, zero RemoveFromParent calls, zero off-screen coordinate hacks!
 */
struct HudItem {
    MonoObject* cell = nullptr;
    MonoObject* label = nullptr;
};

struct HudState {
    MonoObject* bg_panel = nullptr;
    HudItem cpu_lbl;
    HudItem cpu_val;
    HudItem sep1;
    HudItem gpu_lbl;
    HudItem gpu_val;
    HudItem sep2;
    HudItem ram_lbl;
    HudItem ram_val;
    HudItem sep3;
    HudItem fan_lbl;
    HudItem fan_val;
    bool initialized = false;
    MonoObject* root_widget = nullptr;
    char current_theme[32] = "";
    char current_pos[16] = "";
};

static HudState s_hud;
static MonoObject* s_font = nullptr;

static HudItem create_hud_item(MonoDomain* domain, MonoImage* pui_img,
                               MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                               MonoObject* root, const char* name, float x, float y, float w, float h,
                               const char* text, MonoObject* font,
                               float r, float g, float b, float a = 1.0f) {
    HudItem item;
    /* 1. Container cell (Panel) positioned at (X = x, Y = y) */
    item.cell = mono_object_new(domain, panel_class);
    if (!item.cell) return item;
    mono_runtime_object_init(item.cell);

    char cell_name[96];
    snprintf(cell_name, sizeof(cell_name), "%s_cell", name);
    Set_Property(panel_class, item.cell, "Name", mono_string_new(domain, cell_name));
    Set_Property(panel_class, item.cell, "X", x);
    Set_Property(panel_class, item.cell, "Y", y);
    Set_Property(panel_class, item.cell, "Width", w);
    Set_Property(panel_class, item.cell, "Height", h);
    Set_Property(panel_class, item.cell, "BackgroundVisibility", false);
    widget_append_child(widget_class, root, item.cell);

    /* 2. Label inside container cell */
    item.label = mono_object_new(domain, label_class);
    if (!item.label) return item;
    mono_runtime_object_init(item.label);

    Set_Property(label_class, item.label, "Name", mono_string_new(domain, name));
    Set_Property(label_class, item.label, "PositionType", 1);
    Set_Property(label_class, item.label, "MarginLeft", 0.0f);
    Set_Property(label_class, item.label, "MarginTop", 5.0f);
    Set_Property(label_class, item.label, "Width", w);
    Set_Property(label_class, item.label, "Height", h);
    Set_Property(label_class, item.label, "Text", mono_string_new(domain, text));
    if (font) {
        Set_Property_Invoke(label_class, item.label, "Font", font);
    }
    Set_Property(label_class, item.label, "HorizontalAlignment", 0);
    Set_Property(label_class, item.label, "VerticalAlignment", 0);
    Set_Property(label_class, item.label, "FitWidthToText", false);
    Set_Property(label_class, item.label, "FitHeightToText", true);
    Set_Property(label_class, item.label, "NumberOfLines", 1);
    Set_Property(label_class, item.label, "EnableThemedTextShadow", true);

    MonoObject* text_color = create_ui_color(pui_img, domain, r, g, b, a);
    if (text_color) {
        Set_Property_Invoke(label_class, item.label, "TextColor", text_color);
    }

    widget_append_child(widget_class, item.cell, item.label);
    return item;
}

static void update_hud_item(MonoClass* panel_class, MonoClass* label_class, MonoImage* pui_img, MonoDomain* domain,
                            HudItem& item, float x, float y, float w, float h,
                            const char* text, float r, float g, float b) {
    if (!item.cell || !item.label) return;

    Set_Property(panel_class, item.cell, "X", x);
    Set_Property(panel_class, item.cell, "Y", y);
    Set_Property(panel_class, item.cell, "Width", w);
    Set_Property(panel_class, item.cell, "Height", h);

    Set_Property(label_class, item.label, "Width", w);
    Set_Property(label_class, item.label, "Height", h);
    Set_Property(label_class, item.label, "Text", mono_string_new(domain, text));
    MonoObject* col = create_ui_color(pui_img, domain, r, g, b, 1.0f);
    if (col) {
        Set_Property_Invoke(label_class, item.label, "TextColor", col);
    }
}

struct ThemeStyle {
    float bg_r, bg_g, bg_b, bg_a;
    float cpu_r, cpu_g, cpu_b;
    float gpu_r, gpu_g, gpu_b;
    float ram_r, ram_g, ram_b;
    float fan_r, fan_g, fan_b;
    float val_r, val_g, val_b;
    float sep_r, sep_g, sep_b;
    const char* cpu_lbl;
    const char* gpu_lbl;
    const char* ram_lbl;
    const char* fan_lbl;
    const char* sep_char;
    bool is_ribbon;
    bool is_vertical;
};

static ThemeStyle get_theme_style(const char* name) {
    ThemeStyle s{};
    if (!name || name[0] == '\0') {
        name = "esports";
    }

    if (strcmp(name, "deck") == 0 || strcmp(name, "1") == 0) {
        s.bg_r = 0.05f; s.bg_g = 0.07f; s.bg_b = 0.11f; s.bg_a = 0.90f;
        s.cpu_r = 0.22f; s.cpu_g = 0.74f; s.cpu_b = 0.97f;
        s.gpu_r = 0.00f; s.gpu_g = 0.64f; s.gpu_b = 1.00f;
        s.ram_r = 0.98f; s.ram_g = 0.75f; s.ram_b = 0.14f;
        s.fan_r = 0.18f; s.fan_g = 0.83f; s.fan_b = 0.75f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.20f; s.sep_g = 0.25f; s.sep_b = 0.33f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "|";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "cyber") == 0 || strcmp(name, "2") == 0) {
        s.bg_r = 0.04f; s.bg_g = 0.04f; s.bg_b = 0.06f; s.bg_a = 0.94f;
        s.cpu_r = 0.99f; s.cpu_g = 0.93f; s.cpu_b = 0.04f;
        s.gpu_r = 0.00f; s.gpu_g = 0.94f; s.gpu_b = 1.00f;
        s.ram_r = 1.00f; s.ram_g = 0.00f; s.ram_b = 0.33f;
        s.fan_r = 0.22f; s.fan_g = 1.00f; s.fan_b = 0.08f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 1.00f; s.sep_g = 0.00f; s.sep_b = 0.33f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "//";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "esports") == 0 || strcmp(name, "3") == 0) {
        s.bg_r = 0.00f; s.bg_g = 0.00f; s.bg_b = 0.00f; s.bg_a = 0.70f;
        s.cpu_r = 0.40f; s.cpu_g = 1.00f; s.cpu_b = 0.40f;
        s.gpu_r = 0.70f; s.gpu_g = 0.40f; s.gpu_b = 1.00f;
        s.ram_r = 1.00f; s.ram_g = 0.70f; s.ram_b = 0.30f;
        s.fan_r = 0.20f; s.fan_g = 0.88f; s.fan_b = 1.00f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.75f; s.sep_g = 0.75f; s.sep_b = 0.75f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "|";
        s.is_ribbon = true; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "dock") == 0 || strcmp(name, "4") == 0) {
        s.bg_r = 0.04f; s.bg_g = 0.05f; s.bg_b = 0.09f; s.bg_a = 0.92f;
        s.cpu_r = 0.22f; s.cpu_g = 0.74f; s.cpu_b = 0.97f;
        s.gpu_r = 0.75f; s.gpu_g = 0.52f; s.gpu_b = 0.99f;
        s.ram_r = 0.98f; s.ram_g = 0.57f; s.ram_b = 0.24f;
        s.fan_r = 0.29f; s.fan_g = 0.87f; s.fan_b = 0.50f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.00f; s.sep_g = 0.00f; s.sep_b = 0.00f;
        s.cpu_lbl = "CPU:"; s.gpu_lbl = "GPU:"; s.ram_lbl = "RAM:"; s.fan_lbl = "FAN:";
        s.sep_char = "";
        s.is_ribbon = false; s.is_vertical = true;
        return s;
    }
    if (strcmp(name, "bento") == 0 || strcmp(name, "5") == 0) {
        s.bg_r = 0.07f; s.bg_g = 0.10f; s.bg_b = 0.16f; s.bg_a = 0.86f;
        s.cpu_r = 0.40f; s.cpu_g = 0.91f; s.cpu_b = 0.98f;
        s.gpu_r = 0.51f; s.gpu_g = 0.55f; s.gpu_b = 0.97f;
        s.ram_r = 0.99f; s.ram_g = 0.64f; s.ram_b = 0.69f;
        s.fan_r = 0.43f; s.fan_g = 0.91f; s.fan_b = 0.72f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.39f; s.sep_g = 0.45f; s.sep_b = 0.55f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = " - ";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "matrix") == 0 || strcmp(name, "6") == 0) {
        s.bg_r = 0.00f; s.bg_g = 0.00f; s.bg_b = 0.00f; s.bg_a = 0.95f;
        s.cpu_r = 0.13f; s.cpu_g = 0.77f; s.cpu_b = 0.37f;
        s.gpu_r = 0.29f; s.gpu_g = 0.87f; s.gpu_b = 0.50f;
        s.ram_r = 0.53f; s.ram_g = 0.94f; s.ram_b = 0.67f;
        s.fan_r = 0.65f; s.fan_g = 0.95f; s.fan_b = 0.82f;
        s.val_r = 0.29f; s.val_g = 0.87f; s.val_b = 0.50f;
        s.sep_r = 0.09f; s.sep_g = 0.40f; s.sep_b = 0.20f;
        s.cpu_lbl = "[CPU]"; s.gpu_lbl = "[GPU]"; s.ram_lbl = "[RAM]"; s.fan_lbl = "[FAN]";
        s.sep_char = "][";
        s.is_ribbon = true; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "radial") == 0 || strcmp(name, "gauges") == 0 || strcmp(name, "7") == 0) {
        s.bg_r = 0.05f; s.bg_g = 0.05f; s.bg_b = 0.07f; s.bg_a = 0.90f;
        s.cpu_r = 0.92f; s.cpu_g = 0.70f; s.cpu_b = 0.03f;
        s.gpu_r = 0.94f; s.gpu_g = 0.27f; s.gpu_b = 0.27f;
        s.ram_r = 0.02f; s.ram_g = 0.71f; s.ram_b = 0.83f;
        s.fan_r = 0.06f; s.fan_g = 0.73f; s.fan_b = 0.51f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.28f; s.sep_g = 0.33f; s.sep_b = 0.41f;
        s.cpu_lbl = "CPU:"; s.gpu_lbl = "GPU:"; s.ram_lbl = "RAM:"; s.fan_lbl = "FAN:";
        s.sep_char = "/";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "pills") == 0 || strcmp(name, "8") == 0) {
        s.bg_r = 0.06f; s.bg_g = 0.09f; s.bg_b = 0.16f; s.bg_a = 0.80f;
        s.cpu_r = 0.75f; s.cpu_g = 0.52f; s.cpu_b = 0.99f;
        s.gpu_r = 0.22f; s.gpu_g = 0.74f; s.gpu_b = 0.97f;
        s.ram_r = 0.98f; s.ram_g = 0.57f; s.ram_b = 0.24f;
        s.fan_r = 0.29f; s.fan_g = 0.87f; s.fan_b = 0.50f;
        s.val_r = 0.97f; s.val_g = 0.98f; s.val_b = 0.99f;
        s.sep_r = 0.20f; s.sep_g = 0.25f; s.sep_b = 0.33f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "|";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "rog") == 0 || strcmp(name, "9") == 0) {
        s.bg_r = 0.05f; s.bg_g = 0.05f; s.bg_b = 0.07f; s.bg_a = 0.95f;
        s.cpu_r = 1.00f; s.cpu_g = 0.09f; s.cpu_b = 0.27f;
        s.gpu_r = 0.84f; s.gpu_g = 0.00f; s.gpu_b = 0.00f;
        s.ram_r = 0.89f; s.ram_g = 0.91f; s.ram_b = 0.94f;
        s.fan_r = 1.00f; s.fan_g = 0.34f; s.fan_b = 0.13f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 1.00f; s.sep_g = 0.09f; s.sep_b = 0.27f;
        s.cpu_lbl = "R-CPU"; s.gpu_lbl = "R-GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "//";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }
    if (strcmp(name, "prism") == 0 || strcmp(name, "10") == 0) {
        s.bg_r = 0.07f; s.bg_g = 0.05f; s.bg_b = 0.14f; s.bg_a = 0.92f;
        s.cpu_r = 0.66f; s.cpu_g = 0.33f; s.cpu_b = 0.97f;
        s.gpu_r = 0.93f; s.gpu_g = 0.28f; s.gpu_b = 0.60f;
        s.ram_r = 0.02f; s.ram_g = 0.71f; s.ram_b = 0.83f;
        s.fan_r = 0.96f; s.fan_g = 0.62f; s.fan_b = 0.04f;
        s.val_r = 1.00f; s.val_g = 1.00f; s.val_b = 1.00f;
        s.sep_r = 0.66f; s.sep_g = 0.33f; s.sep_b = 0.97f;
        s.cpu_lbl = "CPU"; s.gpu_lbl = "GPU"; s.ram_lbl = "RAM"; s.fan_lbl = "FAN";
        s.sep_char = "~";
        s.is_ribbon = false; s.is_vertical = false;
        return s;
    }

    // Default fallback: esports
    return get_theme_style("esports");
}

static void init_hud_widgets(MonoDomain* domain, MonoImage* pui_img,
                             MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                             MonoObject* root_widget) {
    log_shellui("[SHELLUI] Initializing HUD widgets on root %p...\n", root_widget);

    /* 1. Background panel attached to root_widget */
    s_hud.bg_panel = mono_object_new(domain, panel_class);
    if (s_hud.bg_panel) {
        mono_runtime_object_init(s_hud.bg_panel);
        Set_Property(panel_class, s_hud.bg_panel, "Name", mono_string_new(domain, "ps5_hud_bg"));
        Set_Property(panel_class, s_hud.bg_panel, "X", 0.0f);
        Set_Property(panel_class, s_hud.bg_panel, "Y", 0.0f);
        Set_Property(panel_class, s_hud.bg_panel, "Width", 1920.0f);
        Set_Property(panel_class, s_hud.bg_panel, "Height", 34.0f);
        Set_Property(panel_class, s_hud.bg_panel, "BackgroundVisibility", true);
        Set_Property(panel_class, s_hud.bg_panel, "BackgroundOpacity", 1.0f);
        Set_Property(panel_class, s_hud.bg_panel, "BackgroundStyle", 1);
        widget_append_child(widget_class, root_widget, s_hud.bg_panel);
    }

    /* 2. Font: 18pt, bold=1, weight=900 (proven 100% stable in v1.0.8) */
    if (!s_font) {
        s_font = create_ui_font(pui_img, domain, 18, 1, 900);
        log_shellui("[SHELLUI] Font created: %p\n", s_font);
    }

    /* 3. Create the 11 items as in v1.0.8 */
    s_hud.cpu_lbl = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_cpu_lbl", 24.0f, 0.0f, 68.0f, 34.0f, "CPU", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.cpu_val = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_cpu_val", 94.0f, 0.0f, 56.0f, 34.0f, "--°C", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.sep1    = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_sep1", 152.0f, 0.0f, 20.0f, 34.0f, "|", s_font, 0.75f, 0.75f, 0.75f);

    s_hud.gpu_lbl = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_gpu_lbl", 174.0f, 0.0f, 68.0f, 34.0f, "GPU", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.gpu_val = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_gpu_val", 244.0f, 0.0f, 56.0f, 34.0f, "--°C", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.sep2    = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_sep2", 302.0f, 0.0f, 20.0f, 34.0f, "|", s_font, 0.75f, 0.75f, 0.75f);

    s_hud.ram_lbl = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_ram_lbl", 324.0f, 0.0f, 52.0f, 34.0f, "RAM", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.ram_val = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_ram_val", 378.0f, 0.0f, 78.0f, 34.0f, "-- GB", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.sep3    = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_sep3", 458.0f, 0.0f, 20.0f, 34.0f, "|", s_font, 0.75f, 0.75f, 0.75f);

    s_hud.fan_lbl = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_fan_lbl", 480.0f, 0.0f, 48.0f, 34.0f, "FAN", s_font, 1.0f, 1.0f, 1.0f);
    s_hud.fan_val = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root_widget, "id_fan_val", 530.0f, 0.0f, 56.0f, 34.0f, "--%", s_font, 1.0f, 1.0f, 1.0f);

    s_hud.initialized = true;
    s_hud.root_widget = root_widget;
    s_hud.current_theme[0] = '\0';
    s_hud.current_pos[0] = '\0';
    log_shellui("[SHELLUI] HUD widgets initialized successfully on RootWidget!\n");
}

static void apply_theme(const char* theme_name, const char* pos_name,
                        MonoDomain* domain, MonoImage* pui_img,
                        MonoClass* panel_class, MonoClass* label_class) {
    if (!s_hud.initialized) return;

    if (!theme_name || strcmp(theme_name, "none") == 0 || strcmp(theme_name, "off") == 0 || strcmp(theme_name, "hide") == 0) {
        Set_Property(panel_class, s_hud.bg_panel, "BackgroundVisibility", false);
        Set_Property(label_class, s_hud.cpu_lbl.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.cpu_val.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.sep1.label,    "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.gpu_lbl.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.gpu_val.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.sep2.label,    "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.ram_lbl.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.ram_val.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.sep3.label,    "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.fan_lbl.label, "Text", mono_string_new(domain, ""));
        Set_Property(label_class, s_hud.fan_val.label, "Text", mono_string_new(domain, ""));

        strncpy(s_hud.current_theme, theme_name ? theme_name : "none", sizeof(s_hud.current_theme) - 1);
        if (pos_name) strncpy(s_hud.current_pos, pos_name, sizeof(s_hud.current_pos) - 1);
        log_shellui("[SHELLUI] Overlay hidden\n");
        return;
    }

    ThemeStyle style = get_theme_style(theme_name);

    bool is_bottom = (pos_name && strstr(pos_name, "bottom") != nullptr);
    bool is_right  = (pos_name && strstr(pos_name, "right") != nullptr);
    bool is_left   = (pos_name && strstr(pos_name, "left") != nullptr);

    float bg_x = 0.0f, bg_y = 0.0f, bg_w = 0.0f, bg_h = 0.0f;

    if (style.is_ribbon) {
        bg_x = 0.0f;
        bg_y = is_bottom ? 1046.0f : 0.0f;
        bg_w = 1920.0f;
        bg_h = 34.0f;
    } else if (style.is_vertical) {
        bg_w = 185.0f;
        bg_h = 140.0f;
        bg_y = is_bottom ? 880.0f : 120.0f;
        bg_x = is_right ? (1920.0f - bg_w - 30.0f) : 30.0f;
    } else { // Box HUD
        bg_w = 620.0f;
        bg_h = 34.0f;
        bg_y = is_bottom ? 1026.0f : 20.0f;
        if (is_left) {
            bg_x = 30.0f;
        } else if (is_right) {
            bg_x = 1920.0f - bg_w - 30.0f;
        } else {
            bg_x = (1920.0f - bg_w) / 2.0f;
        }
    }

    /* 1. Update background panel */
    Set_Property(panel_class, s_hud.bg_panel, "X", bg_x);
    Set_Property(panel_class, s_hud.bg_panel, "Y", bg_y);
    Set_Property(panel_class, s_hud.bg_panel, "Width", bg_w);
    Set_Property(panel_class, s_hud.bg_panel, "Height", bg_h);
    Set_Property(panel_class, s_hud.bg_panel, "BackgroundVisibility", true);
    MonoObject* bg_color = create_ui_color(pui_img, domain, style.bg_r, style.bg_g, style.bg_b, style.bg_a);
    if (bg_color) {
        Set_Property_Invoke(panel_class, s_hud.bg_panel, "BackgroundColor", bg_color);
    }

    /* 2. Update 11 items */
    if (style.is_vertical) {
        // Vertical stack (4 rows)
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.cpu_lbl, bg_x + 14.0f, bg_y + 6.0f, 62.0f, 30.0f, style.cpu_lbl, style.cpu_r, style.cpu_g, style.cpu_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.cpu_val, bg_x + 78.0f, bg_y + 6.0f, 90.0f, 30.0f, "--°C", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep1,    bg_x,          bg_y,          1.0f,  1.0f,  "", 0.0f, 0.0f, 0.0f);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.gpu_lbl, bg_x + 14.0f, bg_y + 38.0f, 62.0f, 30.0f, style.gpu_lbl, style.gpu_r, style.gpu_g, style.gpu_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.gpu_val, bg_x + 78.0f, bg_y + 38.0f, 90.0f, 30.0f, "--°C", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep2,    bg_x,          bg_y,          1.0f,  1.0f,  "", 0.0f, 0.0f, 0.0f);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.ram_lbl, bg_x + 14.0f, bg_y + 70.0f, 62.0f, 30.0f, style.ram_lbl, style.ram_r, style.ram_g, style.ram_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.ram_val, bg_x + 78.0f, bg_y + 70.0f, 90.0f, 30.0f, "-- GB", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep3,    bg_x,          bg_y,          1.0f,  1.0f,  "", 0.0f, 0.0f, 0.0f);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.fan_lbl, bg_x + 14.0f, bg_y + 102.0f, 62.0f, 30.0f, style.fan_lbl, style.fan_r, style.fan_g, style.fan_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.fan_val, bg_x + 78.0f, bg_y + 102.0f, 90.0f, 30.0f, "--%", style.val_r, style.val_g, style.val_b);
    } else {
        // Horizontal bar / box
        float bx = style.is_ribbon ? 24.0f : bg_x;
        float by = bg_y;

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.cpu_lbl, bx + 14.0f,  by, 68.0f, 34.0f, style.cpu_lbl, style.cpu_r, style.cpu_g, style.cpu_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.cpu_val, bx + 84.0f,  by, 56.0f, 34.0f, "--°C", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep1,    bx + 142.0f, by, 20.0f, 34.0f, style.sep_char, style.sep_r, style.sep_g, style.sep_b);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.gpu_lbl, bx + 164.0f, by, 68.0f, 34.0f, style.gpu_lbl, style.gpu_r, style.gpu_g, style.gpu_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.gpu_val, bx + 234.0f, by, 56.0f, 34.0f, "--°C", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep2,    bx + 292.0f, by, 20.0f, 34.0f, style.sep_char, style.sep_r, style.sep_g, style.sep_b);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.ram_lbl, bx + 314.0f, by, 52.0f, 34.0f, style.ram_lbl, style.ram_r, style.ram_g, style.ram_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.ram_val, bx + 368.0f, by, 78.0f, 34.0f, "-- GB", style.val_r, style.val_g, style.val_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.sep3,    bx + 448.0f, by, 20.0f, 34.0f, style.sep_char, style.sep_r, style.sep_g, style.sep_b);

        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.fan_lbl, bx + 470.0f, by, 48.0f, 34.0f, style.fan_lbl, style.fan_r, style.fan_g, style.fan_b);
        update_hud_item(panel_class, label_class, pui_img, domain, s_hud.fan_val, bx + 520.0f, by, 56.0f, 34.0f, "--%", style.val_r, style.val_g, style.val_b);
    }

    strncpy(s_hud.current_theme, theme_name, sizeof(s_hud.current_theme) - 1);
    if (pos_name) strncpy(s_hud.current_pos, pos_name, sizeof(s_hud.current_pos) - 1);
    log_shellui("[SHELLUI] Applied theme '%s' (pos '%s')\n", theme_name, pos_name ? pos_name : "top");
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

    /* 2. Attach to Mono root domain */
    MonoDomain* domain = mono_get_root_domain();
    if (!domain) {
        log_shellui("[SHELLUI] mono_get_root_domain returned null\n");
        return -1;
    }
    mono_thread_attach(domain);
    log_shellui("[SHELLUI] Attached to Mono root domain: %p\n", domain);

    /* 3. Patch Sony's UI thread check */
    patch_main_thread_check(domain);

    /* 4. Load required assemblies */
    MonoImage* pui_img = nullptr;
    MonoImage* app_img = nullptr;

    for (int retry = 0; retry < 10 && (!pui_img || !app_img); retry++) {
        if (!pui_img) pui_img = load_system_dll(domain, "Sce.PlayStation.PUI.dll");
        if (!app_img) app_img = load_system_dll(domain, "Sce.PlayStation.AppSystem.dll");
        if (!pui_img || !app_img) {
            log_shellui("[SHELLUI] Waiting for system DLLs (%d/10)...\n", retry + 1);
            sleep(1);
        }
    }

    if (!pui_img || !app_img) {
        log_shellui("[SHELLUI] Failed to load required system assemblies\n");
        return -1;
    }
    log_shellui("[SHELLUI] Loaded PUI (%p) and AppSystem (%p)\n", pui_img, app_img);

    /* 5. Lookup classes and methods */
    MonoClass* layer_mgr_class = mono_class_from_name(app_img, "Sce.PlayStation.AppSystem", "LayerManager");
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

    bool attached_to_game = false;
    MonoObject* last_attached_scene = nullptr;
    MonoObject* last_root_widget = nullptr;

    char target_theme[32] = "esports";
    char target_pos[16] = "top";

    /* Continuous monitoring loop: attaches to Game scene whenever active */
    while (true) {
        /* Read desired theme from IPC file written by Web Server */
        FILE* tfp = fopen("/system_tmp/ps5_overlay_theme.txt", "r");
        if (tfp) {
            char file_theme[32] = {0};
            char file_pos[16] = {0};
            if (fscanf(tfp, "%31s %15s", file_theme, file_pos) >= 1) {
                strncpy(target_theme, file_theme, sizeof(target_theme) - 1);
                if (file_pos[0] != '\0') {
                    strncpy(target_pos, file_pos, sizeof(target_pos) - 1);
                }
            }
            fclose(tfp);
        }

        MonoString* game_str = mono_string_new(domain, "Game");
        void* scene_args[1] = { game_str };
        MonoObject* exc = nullptr;
        MonoObject* game_scene = mono_runtime_invoke(find_scene, nullptr, scene_args, &exc);

        if (game_scene) {
            MonoObject* root_widget = mono_runtime_invoke(get_root, game_scene, nullptr, &exc);
            if (root_widget) {
                bool scene_changed = (game_scene != last_attached_scene || root_widget != last_root_widget || !s_hud.initialized);
                bool theme_changed = (strcmp(s_hud.current_theme, target_theme) != 0 || strcmp(s_hud.current_pos, target_pos) != 0);

                if (scene_changed) {
                    log_shellui("[SHELLUI] Active Game Scene found/changed (%p -> %p). Initializing HUD...\n",
                                last_root_widget, root_widget);
                    init_hud_widgets(domain, pui_img, widget_class, panel_class, label_class, root_widget);
                    apply_theme(target_theme, target_pos, domain, pui_img, panel_class, label_class);
                    last_attached_scene = game_scene;
                    last_root_widget = root_widget;
                    attached_to_game = true;
                } else if (theme_changed) {
                    log_shellui("[SHELLUI] Live theme switch requested: '%s' -> '%s' (pos '%s')\n",
                                s_hud.current_theme, target_theme, target_pos);
                    apply_theme(target_theme, target_pos, domain, pui_img, panel_class, label_class);
                }
            }
        } else if (attached_to_game) {
            /* Game closed */
            log_shellui("[SHELLUI] Game closed, resetting HUD state...\n");
            s_hud.initialized = false;
            s_hud.root_widget = nullptr;
            s_hud.current_theme[0] = '\0';
            s_hud.current_pos[0] = '\0';
            last_root_widget = nullptr;
            last_attached_scene = nullptr;
            attached_to_game = false;
        }

        /* Update metrics if attached and HUD visible */
        if (attached_to_game && s_hud.initialized &&
            strcmp(s_hud.current_theme, "none") != 0 &&
            strcmp(s_hud.current_theme, "off") != 0 &&
            strcmp(s_hud.current_theme, "hide") != 0) {
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
            if (s_hud.cpu_val.label) {
                snprintf(buf, sizeof(buf), "%d°C", cpu_temp);
                Set_Property(label_class, s_hud.cpu_val.label, "Text", mono_string_new(domain, buf));
            }

            if (s_hud.gpu_val.label) {
                snprintf(buf, sizeof(buf), "%d°C", gpu_temp);
                Set_Property(label_class, s_hud.gpu_val.label, "Text", mono_string_new(domain, buf));
            }

            if (s_hud.ram_val.label) {
                int ram_total = 0, ram_free = 0;
                if (sys_get_page_table_stats && sys_get_page_table_stats(1, 1, &ram_total, &ram_free) == 0 && ram_total > 0) {
                    int used_mb = ram_total - ram_free;
                    snprintf(buf, sizeof(buf), "%.1f GB", (float)used_mb / 1024.0f);
                } else {
                    snprintf(buf, sizeof(buf), "N/A");
                }
                Set_Property(label_class, s_hud.ram_val.label, "Text", mono_string_new(domain, buf));
            }

            if (s_hud.fan_val.label) {
                snprintf(buf, sizeof(buf), "%.0f%%", fan_pct);
                Set_Property(label_class, s_hud.fan_val.label, "Text", mono_string_new(domain, buf));
            }
        }

        sleep(1);
    }
#else
    printf("[SHELLUI_OVERLAY] Host test stub.\n");
#endif

    return 0;
}
