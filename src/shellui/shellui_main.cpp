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
#include <vector>

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

static void widget_append_child(MonoClass* widget_class, MonoObject* parent, MonoObject* child) {
    if (!widget_class || !parent || !child) return;
    MonoMethod* append_child = mono_class_get_method_from_name(widget_class, "AppendChild", 1);
    if (append_child) {
        void* args[1] = { child };
        mono_runtime_invoke(append_child, parent, args, nullptr);
    }
}

static std::vector<MonoObject*> s_active_theme_widgets;
static MonoMethod* s_method_remove_from_parent = nullptr;

static void cleanup_active_theme() {
    if (!s_method_remove_from_parent) return;
    for (size_t i = 0; i < s_active_theme_widgets.size(); i++) {
        MonoObject* w = s_active_theme_widgets[i];
        if (w) {
            mono_runtime_invoke(s_method_remove_from_parent, w, nullptr, nullptr);
        }
    }
    s_active_theme_widgets.clear();
}

static MonoObject* create_hud_item(MonoDomain* domain, MonoImage* pui_img,
                                   MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                                   MonoObject* root, const char* name, float x, float y, float cell_y,
                                   const char* text, MonoObject* font,
                                   float r, float g, float b, float a = 1.0f) {
    /* 1. Container cell (Panel) positioned at (X = x, Y = cell_y) */
    MonoObject* cell = mono_object_new(domain, panel_class);
    if (!cell) return nullptr;
    mono_runtime_object_init(cell);

    char cell_name[96];
    snprintf(cell_name, sizeof(cell_name), "%s_cell", name);
    Set_Property(panel_class, cell, "Name", mono_string_new(domain, cell_name));
    Set_Property(panel_class, cell, "X", x);
    Set_Property(panel_class, cell, "Y", cell_y);
    Set_Property(panel_class, cell, "Width", 300.0f);
    Set_Property(panel_class, cell, "Height", 34.0f);
    Set_Property(panel_class, cell, "BackgroundVisibility", false);
    widget_append_child(widget_class, root, cell);
    s_active_theme_widgets.push_back(cell);

    /* 2. Label inside container cell */
    MonoObject* label = mono_object_new(domain, label_class);
    if (!label) return nullptr;
    mono_runtime_object_init(label);

    Set_Property(label_class, label, "Name", mono_string_new(domain, name));
    Set_Property(label_class, label, "PositionType", 1);
    Set_Property(label_class, label, "MarginLeft", 0.0f);
    Set_Property(label_class, label, "MarginTop", y);
    Set_Property(label_class, label, "Width", 300.0f);
    Set_Property(label_class, label, "Height", 34.0f);
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

    widget_append_child(widget_class, cell, label);
    return label;
}

static MonoObject* create_modular_card(MonoDomain* domain, MonoImage* pui_img,
                                       MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                                       MonoObject* root, float x, float y,
                                       const char* title, const char* default_val, const char* subtext,
                                       float bg_r, float bg_g, float bg_b, float bg_a,
                                       float accent_r, float accent_g, float accent_b,
                                       float title_r, float title_g, float title_b,
                                       float val_r, float val_g, float val_b,
                                       MonoObject*& out_val_label) {
    // 1. Base card panel
    MonoObject* card = mono_object_new(domain, panel_class);
    mono_runtime_object_init(card);
    Set_Property(panel_class, card, "X", x);
    Set_Property(panel_class, card, "Y", y);
    Set_Property(panel_class, card, "Width", 136.0f);
    Set_Property(panel_class, card, "Height", 76.0f);
    MonoObject* bg_col = create_ui_color(pui_img, domain, bg_r, bg_g, bg_b, bg_a);
    if (bg_col) Set_Property_Invoke(panel_class, card, "BackgroundColor", bg_col);
    Set_Property(panel_class, card, "BackgroundVisibility", true);
    Set_Property(panel_class, card, "BackgroundOpacity", 1.0f);
    Set_Property(panel_class, card, "BackgroundStyle", 1);
    widget_append_child(widget_class, root, card);
    s_active_theme_widgets.push_back(card);

    // 2. Top accent stripe (3px high)
    MonoObject* strip = mono_object_new(domain, panel_class);
    mono_runtime_object_init(strip);
    Set_Property(panel_class, strip, "X", 0.0f);
    Set_Property(panel_class, strip, "Y", 0.0f);
    Set_Property(panel_class, strip, "Width", 136.0f);
    Set_Property(panel_class, strip, "Height", 3.0f);
    MonoObject* strip_col = create_ui_color(pui_img, domain, accent_r, accent_g, accent_b, 1.0f);
    if (strip_col) Set_Property_Invoke(panel_class, strip, "BackgroundColor", strip_col);
    Set_Property(panel_class, strip, "BackgroundVisibility", true);
    Set_Property(panel_class, strip, "BackgroundOpacity", 1.0f);
    widget_append_child(widget_class, card, strip);

    // 3. Header title (11pt)
    MonoObject* font_head = create_ui_font(pui_img, domain, 11, 1, 700);
    MonoObject* head_lbl = mono_object_new(domain, label_class);
    mono_runtime_object_init(head_lbl);
    Set_Property(label_class, head_lbl, "PositionType", 1);
    Set_Property(label_class, head_lbl, "MarginLeft", 8.0f);
    Set_Property(label_class, head_lbl, "MarginTop", 8.0f);
    Set_Property(label_class, head_lbl, "Width", 120.0f);
    Set_Property(label_class, head_lbl, "Height", 16.0f);
    Set_Property(label_class, head_lbl, "Text", mono_string_new(domain, title));
    Set_Property_Invoke(label_class, head_lbl, "Font", font_head);
    MonoObject* head_col = create_ui_color(pui_img, domain, title_r, title_g, title_b, 1.0f);
    if (head_col) Set_Property_Invoke(label_class, head_lbl, "TextColor", head_col);
    widget_append_child(widget_class, card, head_lbl);

    // 4. Big value label (22pt bold)
    MonoObject* font_val = create_ui_font(pui_img, domain, 22, 1, 900);
    MonoObject* val_lbl = mono_object_new(domain, label_class);
    mono_runtime_object_init(val_lbl);
    Set_Property(label_class, val_lbl, "PositionType", 1);
    Set_Property(label_class, val_lbl, "MarginLeft", 8.0f);
    Set_Property(label_class, val_lbl, "MarginTop", 24.0f);
    Set_Property(label_class, val_lbl, "Width", 120.0f);
    Set_Property(label_class, val_lbl, "Height", 30.0f);
    Set_Property(label_class, val_lbl, "Text", mono_string_new(domain, default_val));
    Set_Property_Invoke(label_class, val_lbl, "Font", font_val);
    MonoObject* val_col = create_ui_color(pui_img, domain, val_r, val_g, val_b, 1.0f);
    if (val_col) Set_Property_Invoke(label_class, val_lbl, "TextColor", val_col);
    Set_Property(label_class, val_lbl, "EnableThemedTextShadow", true);
    widget_append_child(widget_class, card, val_lbl);
    out_val_label = val_lbl;

    // 5. Sub label (10pt)
    MonoObject* font_sub = create_ui_font(pui_img, domain, 10, 1, 600);
    MonoObject* sub_lbl = mono_object_new(domain, label_class);
    mono_runtime_object_init(sub_lbl);
    Set_Property(label_class, sub_lbl, "PositionType", 1);
    Set_Property(label_class, sub_lbl, "MarginLeft", 8.0f);
    Set_Property(label_class, sub_lbl, "MarginTop", 56.0f);
    Set_Property(label_class, sub_lbl, "Width", 120.0f);
    Set_Property(label_class, sub_lbl, "Height", 16.0f);
    Set_Property(label_class, sub_lbl, "Text", mono_string_new(domain, subtext));
    Set_Property_Invoke(label_class, sub_lbl, "Font", font_sub);
    MonoObject* sub_col = create_ui_color(pui_img, domain, accent_r, accent_g, accent_b, 0.90f);
    if (sub_col) Set_Property_Invoke(label_class, sub_lbl, "TextColor", sub_col);
    widget_append_child(widget_class, card, sub_lbl);

    return card;
}

static void build_theme_esports(MonoDomain* domain, MonoImage* pui_img,
                                MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                                MonoObject* root, float base_y,
                                MonoObject*& out_cpu, MonoObject*& out_gpu,
                                MonoObject*& out_ram, MonoObject*& out_fan) {
    MonoObject* bg = mono_object_new(domain, panel_class);
    mono_runtime_object_init(bg);
    Set_Property(panel_class, bg, "X", 0.0f);
    Set_Property(panel_class, bg, "Y", base_y);
    Set_Property(panel_class, bg, "Width", 1920.0f);
    Set_Property(panel_class, bg, "Height", 34.0f);
    MonoObject* col = create_ui_color(pui_img, domain, 0.0f, 0.0f, 0.0f, 0.70f);
    if (col) Set_Property_Invoke(panel_class, bg, "BackgroundColor", col);
    Set_Property(panel_class, bg, "BackgroundVisibility", true);
    Set_Property(panel_class, bg, "BackgroundOpacity", 1.0f);
    Set_Property(panel_class, bg, "BackgroundStyle", 1);
    widget_append_child(widget_class, root, bg);
    s_active_theme_widgets.push_back(bg);

    MonoObject* font = create_ui_font(pui_img, domain, 18, 1, 900);
    float y = 5.0f;

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_cpu_lbl", 24.0f, y, base_y, "CPU", font, 102.0f/255.0f, 1.0f, 102.0f/255.0f);
    out_cpu = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "es_cpu_val", 72.0f, y, base_y, "--°C", font, 1.0f, 1.0f, 1.0f);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_sep1", 138.0f, y, base_y, "|", font, 0.75f, 0.75f, 0.75f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_gpu_lbl", 158.0f, y, base_y, "GPU", font, 179.0f/255.0f, 102.0f/255.0f, 1.0f);
    out_gpu = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "es_gpu_val", 206.0f, y, base_y, "--°C", font, 1.0f, 1.0f, 1.0f);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_sep2", 270.0f, y, base_y, "|", font, 0.75f, 0.75f, 0.75f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_ram_lbl", 290.0f, y, base_y, "RAM", font, 1.0f, 179.0f/255.0f, 77.0f/255.0f);
    out_ram = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "es_ram_val", 344.0f, y, base_y, "-- GB", font, 1.0f, 1.0f, 1.0f);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_sep3", 452.0f, y, base_y, "|", font, 0.75f, 0.75f, 0.75f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "es_fan_lbl", 472.0f, y, base_y, "FAN", font, 51.0f/255.0f, 224.0f/255.0f, 1.0f);
    out_fan = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "es_fan_val", 518.0f, y, base_y, "--%", font, 1.0f, 1.0f, 1.0f);
}

static void build_theme_matrix(MonoDomain* domain, MonoImage* pui_img,
                               MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                               MonoObject* root, float base_y,
                               MonoObject*& out_cpu, MonoObject*& out_gpu,
                               MonoObject*& out_ram, MonoObject*& out_fan) {
    MonoObject* bg = mono_object_new(domain, panel_class);
    mono_runtime_object_init(bg);
    Set_Property(panel_class, bg, "X", 0.0f);
    Set_Property(panel_class, bg, "Y", base_y);
    Set_Property(panel_class, bg, "Width", 1920.0f);
    Set_Property(panel_class, bg, "Height", 34.0f);
    MonoObject* col = create_ui_color(pui_img, domain, 5.0f/255.0f, 11.0f/255.0f, 7.0f/255.0f, 0.88f);
    if (col) Set_Property_Invoke(panel_class, bg, "BackgroundColor", col);
    Set_Property(panel_class, bg, "BackgroundVisibility", true);
    Set_Property(panel_class, bg, "BackgroundOpacity", 1.0f);
    Set_Property(panel_class, bg, "BackgroundStyle", 1);
    widget_append_child(widget_class, root, bg);
    s_active_theme_widgets.push_back(bg);

    MonoObject* font = create_ui_font(pui_img, domain, 18, 1, 900);
    float y = 5.0f;
    float gr = 34.0f/255.0f, gg = 197.0f/255.0f, gb = 94.0f/255.0f;

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_cpu_lbl", 24.0f, y, base_y, "CPU:", font, gr, gg, gb);
    out_cpu = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "mt_cpu_val", 76.0f, y, base_y, "--°C", font, gr, gg, gb);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_sep1", 142.0f, y, base_y, "][", font, gr*0.6f, gg*0.6f, gb*0.6f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_gpu_lbl", 168.0f, y, base_y, "GPU:", font, gr, gg, gb);
    out_gpu = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "mt_gpu_val", 220.0f, y, base_y, "--°C", font, gr, gg, gb);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_sep2", 284.0f, y, base_y, "][", font, gr*0.6f, gg*0.6f, gb*0.6f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_ram_lbl", 310.0f, y, base_y, "RAM:", font, gr, gg, gb);
    out_ram = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "mt_ram_val", 364.0f, y, base_y, "-- GB", font, gr, gg, gb);
    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_sep3", 472.0f, y, base_y, "][", font, gr*0.6f, gg*0.6f, gb*0.6f);

    create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                    "mt_fan_lbl", 498.0f, y, base_y, "FAN:", font, gr, gg, gb);
    out_fan = create_hud_item(domain, pui_img, widget_class, panel_class, label_class, root,
                              "mt_fan_val", 550.0f, y, base_y, "--%", font, gr, gg, gb);
}

static void build_theme_rog(MonoDomain* domain, MonoImage* pui_img,
                            MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                            MonoObject* root, float base_x, float base_y,
                            MonoObject*& out_cpu, MonoObject*& out_gpu,
                            MonoObject*& out_ram, MonoObject*& out_fan) {
    float bg_r = 18.0f/255.0f, bg_g = 19.0f/255.0f, bg_b = 24.0f/255.0f, bg_a = 0.90f;
    float rog_r = 255.0f/255.0f, rog_g = 23.0f/255.0f, rog_b = 68.0f/255.0f;
    float silver_r = 161.0f/255.0f, silver_g = 161.0f/255.0f, silver_b = 170.0f/255.0f;
    float white = 1.0f;

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y, "CPU [ZEN 2]", "--°C", "CORE TEMP",
                        bg_r, bg_g, bg_b, bg_a, rog_r, rog_g, rog_b, silver_r, silver_g, silver_b, white, white, white, out_cpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 146.0f, base_y, "GPU [RDNA 2]", "--°C", "SOC TEMP",
                        bg_r, bg_g, bg_b, bg_a, rog_r, rog_g, rog_b, silver_r, silver_g, silver_b, white, white, white, out_gpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 292.0f, base_y, "SYS MEMORY", "-- GB", "16.0 GB POOL",
                        bg_r, bg_g, bg_b, bg_a, 200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f, silver_r, silver_g, silver_b, white, white, white, out_ram);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 438.0f, base_y, "COOLING FAN", "--%", "QUIET BEARING",
                        bg_r, bg_g, bg_b, bg_a, 16.0f/255.0f, 185.0f/255.0f, 129.0f/255.0f, silver_r, silver_g, silver_b, white, white, white, out_fan);
}

static void build_theme_deck(MonoDomain* domain, MonoImage* pui_img,
                             MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                             MonoObject* root, float base_x, float base_y,
                             MonoObject*& out_cpu, MonoObject*& out_gpu,
                             MonoObject*& out_ram, MonoObject*& out_fan) {
    float bg_r = 15.0f/255.0f, bg_g = 23.0f/255.0f, bg_b = 37.0f/255.0f, bg_a = 0.90f;
    float cyan_r = 56.0f/255.0f, cyan_g = 189.0f/255.0f, cyan_b = 248.0f/255.0f;
    float slate_r = 148.0f/255.0f, slate_g = 163.0f/255.0f, slate_b = 184.0f/255.0f;

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y, "CPU (ZEN 2)", "--°C", "8 CORES",
                        bg_r, bg_g, bg_b, bg_a, cyan_r, cyan_g, cyan_b, slate_r, slate_g, slate_b, cyan_r, cyan_g, cyan_b, out_cpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 146.0f, base_y, "GPU (RDNA 2)", "--°C", "RDNA 2 APU",
                        bg_r, bg_g, bg_b, bg_a, cyan_r, cyan_g, cyan_b, slate_r, slate_g, slate_b, cyan_r, cyan_g, cyan_b, out_gpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 292.0f, base_y, "SYSTEM RAM", "-- GB", "LPDDR5",
                        bg_r, bg_g, bg_b, bg_a, cyan_r, cyan_g, cyan_b, slate_r, slate_g, slate_b, 1.0f, 1.0f, 1.0f, out_ram);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 438.0f, base_y, "COOLING FAN", "--%", "FAN DUTY",
                        bg_r, bg_g, bg_b, bg_a, 52.0f/255.0f, 211.0f/255.0f, 153.0f/255.0f, slate_r, slate_g, slate_b, 1.0f, 1.0f, 1.0f, out_fan);
}

static void build_theme_cyber(MonoDomain* domain, MonoImage* pui_img,
                              MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                              MonoObject* root, float base_x, float base_y,
                              MonoObject*& out_cpu, MonoObject*& out_gpu,
                              MonoObject*& out_ram, MonoObject*& out_fan) {
    float bg_r = 10.0f/255.0f, bg_g = 13.0f/255.0f, bg_b = 24.0f/255.0f, bg_a = 0.92f;
    float yel_r = 252.0f/255.0f, yel_g = 238.0f/255.0f, yel_b = 10.0f/255.0f;
    float cyn_r = 0.0f/255.0f, cyn_g = 240.0f/255.0f, cyn_b = 255.0f/255.0f;

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y, "CPU // ZEN2", "--°C", "CORE // ACTIVE",
                        bg_r, bg_g, bg_b, bg_a, yel_r, yel_g, yel_b, cyn_r, cyn_g, cyn_b, yel_r, yel_g, yel_b, out_cpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 146.0f, base_y, "GPU // RDNA2", "--°C", "RASTER // NOMINAL",
                        bg_r, bg_g, bg_b, bg_a, cyn_r, cyn_g, cyn_b, yel_r, yel_g, yel_b, cyn_r, cyn_g, cyn_b, out_gpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 292.0f, base_y, "RAM // MEMORY", "-- GB", "ALLOC // 16GB",
                        bg_r, bg_g, bg_b, bg_a, 168.0f/255.0f, 85.0f/255.0f, 247.0f/255.0f, cyn_r, cyn_g, cyn_b, 1.0f, 1.0f, 1.0f, out_ram);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 438.0f, base_y, "FAN // TACHO", "--%", "SPEED // PWM",
                        bg_r, bg_g, bg_b, bg_a, 16.0f/255.0f, 185.0f/255.0f, 129.0f/255.0f, cyn_r, cyn_g, cyn_b, 1.0f, 1.0f, 1.0f, out_fan);
}

static void build_theme_dock(MonoDomain* domain, MonoImage* pui_img,
                             MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                             MonoObject* root, float base_x, float base_y,
                             MonoObject*& out_cpu, MonoObject*& out_gpu,
                             MonoObject*& out_ram, MonoObject*& out_fan) {
    float bg_r = 14.0f/255.0f, bg_g = 20.0f/255.0f, bg_b = 34.0f/255.0f, bg_a = 0.94f;
    float purp_r = 168.0f/255.0f, purp_g = 85.0f/255.0f, purp_b = 247.0f/255.0f;
    float cyn_r = 56.0f/255.0f, cyn_g = 189.0f/255.0f, cyn_b = 248.0f/255.0f;

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y, "CPU ZEN 2", "--°C", "LIVE TEMP",
                        bg_r, bg_g, bg_b, bg_a, cyn_r, cyn_g, cyn_b, purp_r, purp_g, purp_b, cyn_r, cyn_g, cyn_b, out_cpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y + 82.0f, "GPU RDNA 2", "--°C", "SOC TEMP",
                        bg_r, bg_g, bg_b, bg_a, cyn_r, cyn_g, cyn_b, purp_r, purp_g, purp_b, cyn_r, cyn_g, cyn_b, out_gpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y + 164.0f, "MEM POOL", "-- GB", "SYSTEM RAM",
                        bg_r, bg_g, bg_b, bg_a, purp_r, purp_g, purp_b, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, out_ram);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y + 246.0f, "FAN SPEED", "--%", "TACHO PWM",
                        bg_r, bg_g, bg_b, bg_a, 16.0f/255.0f, 185.0f/255.0f, 129.0f/255.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, out_fan);
}

static void build_theme_prism(MonoDomain* domain, MonoImage* pui_img,
                              MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                              MonoObject* root, float base_x, float base_y,
                              MonoObject*& out_cpu, MonoObject*& out_gpu,
                              MonoObject*& out_ram, MonoObject*& out_fan) {
    float bg_r = 14.0f/255.0f, bg_g = 16.0f/255.0f, bg_b = 28.0f/255.0f, bg_a = 0.92f;
    float pink_r = 236.0f/255.0f, pink_g = 72.0f/255.0f, pink_b = 153.0f/255.0f;
    float cyn_r = 56.0f/255.0f, cyn_g = 189.0f/255.0f, cyn_b = 248.0f/255.0f;

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x, base_y, "CPU • ZEN 2", "--°C", "CORE HEAT",
                        bg_r, bg_g, bg_b, bg_a, pink_r, pink_g, pink_b, cyn_r, cyn_g, cyn_b, 1.0f, 1.0f, 1.0f, out_cpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 146.0f, base_y, "GPU • RDNA 2", "--°C", "SOC SILICON",
                        bg_r, bg_g, bg_b, bg_a, cyn_r, cyn_g, cyn_b, pink_r, pink_g, pink_b, 1.0f, 1.0f, 1.0f, out_gpu);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 292.0f, base_y, "RAM • POOL", "-- GB", "LPDDR5",
                        bg_r, bg_g, bg_b, bg_a, 168.0f/255.0f, 85.0f/255.0f, 247.0f/255.0f, cyn_r, cyn_g, cyn_b, 1.0f, 1.0f, 1.0f, out_ram);

    create_modular_card(domain, pui_img, widget_class, panel_class, label_class, root,
                        base_x + 438.0f, base_y, "FAN • EXHAUST", "--%", "PWM DUTY",
                        bg_r, bg_g, bg_b, bg_a, 16.0f/255.0f, 185.0f/255.0f, 129.0f/255.0f, pink_r, pink_g, pink_b, 1.0f, 1.0f, 1.0f, out_fan);
}

static char s_current_theme[32] = "";
static char s_current_pos[16] = "";

static void apply_theme(const char* theme_name, const char* pos_name,
                        MonoDomain* domain, MonoImage* pui_img,
                        MonoClass* widget_class, MonoClass* panel_class, MonoClass* label_class,
                        MonoObject* root_widget,
                        MonoObject*& out_cpu, MonoObject*& out_gpu,
                        MonoObject*& out_ram, MonoObject*& out_fan) {
    cleanup_active_theme();
    out_cpu = nullptr;
    out_gpu = nullptr;
    out_ram = nullptr;
    out_fan = nullptr;

    if (!theme_name || strcmp(theme_name, "none") == 0 || strcmp(theme_name, "off") == 0 || strcmp(theme_name, "hide") == 0) {
        strncpy(s_current_theme, theme_name ? theme_name : "none", sizeof(s_current_theme) - 1);
        if (pos_name) strncpy(s_current_pos, pos_name, sizeof(s_current_pos) - 1);
        log_shellui("[SHELLUI] Overlay hidden\n");
        return;
    }

    bool is_bottom = (pos_name && strstr(pos_name, "bottom") != nullptr);
    bool is_right = (pos_name && strstr(pos_name, "right") != nullptr);

    float card_base_x = is_right ? (1920.0f - 584.0f - 24.0f) : 24.0f;
    float card_base_y = is_bottom ? 988.0f : 16.0f;
    float ribbon_base_y = is_bottom ? 1046.0f : 0.0f;

    if (strcmp(theme_name, "rog") == 0 || strcmp(theme_name, "9") == 0) {
        build_theme_rog(domain, pui_img, widget_class, panel_class, label_class, root_widget, card_base_x, card_base_y,
                        out_cpu, out_gpu, out_ram, out_fan);
    } else if (strcmp(theme_name, "deck") == 0 || strcmp(theme_name, "1") == 0 || strcmp(theme_name, "5") == 0 || strcmp(theme_name, "bento") == 0) {
        build_theme_deck(domain, pui_img, widget_class, panel_class, label_class, root_widget, card_base_x, card_base_y,
                         out_cpu, out_gpu, out_ram, out_fan);
    } else if (strcmp(theme_name, "cyber") == 0 || strcmp(theme_name, "2") == 0) {
        build_theme_cyber(domain, pui_img, widget_class, panel_class, label_class, root_widget, card_base_x, card_base_y,
                          out_cpu, out_gpu, out_ram, out_fan);
    } else if (strcmp(theme_name, "dock") == 0 || strcmp(theme_name, "4") == 0) {
        float dock_x = is_right ? (1920.0f - 136.0f - 24.0f) : 24.0f;
        float dock_y = is_bottom ? (1080.0f - 330.0f - 24.0f) : 24.0f;
        build_theme_dock(domain, pui_img, widget_class, panel_class, label_class, root_widget, dock_x, dock_y,
                         out_cpu, out_gpu, out_ram, out_fan);
    } else if (strcmp(theme_name, "prism") == 0 || strcmp(theme_name, "10") == 0 || strcmp(theme_name, "7") == 0 || strcmp(theme_name, "8") == 0) {
        build_theme_prism(domain, pui_img, widget_class, panel_class, label_class, root_widget, card_base_x, card_base_y,
                          out_cpu, out_gpu, out_ram, out_fan);
    } else if (strcmp(theme_name, "matrix") == 0 || strcmp(theme_name, "6") == 0) {
        build_theme_matrix(domain, pui_img, widget_class, panel_class, label_class, root_widget, ribbon_base_y,
                           out_cpu, out_gpu, out_ram, out_fan);
    } else {
        /* Default: esports (minimalist top/bottom bar) */
        build_theme_esports(domain, pui_img, widget_class, panel_class, label_class, root_widget, ribbon_base_y,
                            out_cpu, out_gpu, out_ram, out_fan);
    }

    strncpy(s_current_theme, theme_name, sizeof(s_current_theme) - 1);
    if (pos_name) strncpy(s_current_pos, pos_name, sizeof(s_current_pos) - 1);
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

    s_method_remove_from_parent = mono_class_get_method_from_name(widget_class, "RemoveFromParent", 0);
    log_shellui("[SHELLUI] Widget.RemoveFromParent: %p\n", s_method_remove_from_parent);

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

    MonoObject* cpu_val = nullptr;
    MonoObject* gpu_val = nullptr;
    MonoObject* ram_val = nullptr;
    MonoObject* fan_val = nullptr;
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
                bool scene_changed = (game_scene != last_attached_scene || root_widget != last_root_widget);
                bool theme_changed = (strcmp(s_current_theme, target_theme) != 0 || strcmp(s_current_pos, target_pos) != 0);

                if (scene_changed || theme_changed) {
                    if (scene_changed) {
                        log_shellui("[SHELLUI] Active Game Scene/Root changed (%p -> %p). Applying theme '%s' (pos '%s')...\n",
                                    last_root_widget, root_widget, target_theme, target_pos);
                    } else {
                        log_shellui("[SHELLUI] Live theme switch requested: '%s' -> '%s' (pos '%s')\n",
                                    s_current_theme, target_theme, target_pos);
                    }

                    apply_theme(target_theme, target_pos, domain, pui_img,
                                widget_class, panel_class, label_class, root_widget,
                                cpu_val, gpu_val, ram_val, fan_val);

                    last_attached_scene = game_scene;
                    last_root_widget = root_widget;
                    attached_to_game = true;
                }
            }
        } else if (attached_to_game) {
            /* Game closed */
            log_shellui("[SHELLUI] Game closed, resetting HUD state...\n");
            cleanup_active_theme();
            s_current_theme[0] = '\0';
            s_current_pos[0] = '\0';
            cpu_val = nullptr;
            gpu_val = nullptr;
            ram_val = nullptr;
            fan_val = nullptr;
            last_root_widget = nullptr;
            last_attached_scene = nullptr;
            attached_to_game = false;
        }

        /* Update metrics if attached */
        if (attached_to_game && (cpu_val || gpu_val || ram_val || fan_val)) {
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

            if (ram_val) {
                int ram_total = 0, ram_free = 0;
                if (sys_get_page_table_stats && sys_get_page_table_stats(1, 1, &ram_total, &ram_free) == 0 && ram_total > 0) {
                    int used_mb = ram_total - ram_free;
                    snprintf(buf, sizeof(buf), "%.1f GB", (float)used_mb / 1024.0f);
                } else {
                    snprintf(buf, sizeof(buf), "N/A");
                }
                Set_Property(label_class, ram_val, "Text", mono_string_new(domain, buf));
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
