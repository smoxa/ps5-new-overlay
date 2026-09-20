#include "overlay_ui.h"
#include "notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#if defined(__PS5__) || defined(PS5)
#include <ps5/kernel.h>

typedef void* MonoDomain;
typedef void* MonoAssembly;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoObject;
typedef void* MonoMethod;
typedef void* MonoString;
typedef void* MonoProperty;

extern "C" {
    MonoDomain mono_get_root_domain(void);
    void mono_thread_attach(MonoDomain domain);
    MonoDomain mono_domain_get(void);
    MonoImage mono_image_open(const char* name, int* status);
    MonoClass* mono_class_from_name(MonoImage image, const char* name_space, const char* name);
    MonoMethod* mono_class_get_method_from_name(MonoClass* klass, const char* name, int param_count);
    MonoProperty* mono_class_get_property_from_name(MonoClass* klass, const char* name);
    MonoMethod* mono_property_get_get_method(MonoProperty* prop);
    MonoMethod* mono_property_get_set_method(MonoProperty* prop);
    MonoObject* mono_runtime_invoke(MonoMethod* method, void* obj, void** params, MonoObject** exc);
    MonoString* mono_string_new(MonoDomain domain, const char* text);
    MonoObject* mono_object_new(MonoDomain domain, MonoClass* klass);
    void mono_runtime_object_init(MonoObject* obj);
}

static MonoDomain s_root_domain = nullptr;
static MonoImage s_pui_image = nullptr;
static MonoObject* s_root_widget = nullptr;
static MonoObject* s_bg_panel = nullptr;
static MonoObject* s_hud_label = nullptr;
static MonoClass* s_widget_class = nullptr;
static MonoClass* s_label_class = nullptr;
static MonoClass* s_panel_class = nullptr;
static bool s_shellui_ready = false;

static MonoObject* invoke_method(MonoMethod* method, void* obj, void** params) {
    if (!method) return nullptr;
    MonoObject* exc = nullptr;
    return mono_runtime_invoke(method, obj, params, &exc);
}

static void set_property_string(MonoClass* cls, MonoObject* obj, const char* prop_name, const char* val) {
    if (!cls || !obj) return;
    MonoProperty* prop = mono_class_get_property_from_name(cls, prop_name);
    if (!prop) return;
    MonoMethod* setter = mono_property_get_set_method(prop);
    if (!setter) return;
    MonoString* str = mono_string_new(s_root_domain, val);
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

static bool init_shellui_pui(const OverlayConfig* config) {
    if (s_shellui_ready) return true;

    s_root_domain = mono_get_root_domain();
    if (!s_root_domain) return false;

    mono_thread_attach(s_root_domain);

    int status = 0;
    s_pui_image = mono_image_open("/app0/pui.dll", &status);
    if (!s_pui_image) {
        s_pui_image = mono_image_open("Sce.PlayStation.PUI.UI2.dll", &status);
    }
    if (!s_pui_image) return false;

    s_widget_class = mono_class_from_name(s_pui_image, "Sce.PlayStation.PUI.UI2", "Widget");
    s_label_class = mono_class_from_name(s_pui_image, "Sce.PlayStation.PUI.UI2", "Label");
    s_panel_class = mono_class_from_name(s_pui_image, "Sce.PlayStation.PUI.UI2", "Panel");

    MonoClass* scene_class = mono_class_from_name(s_pui_image, "Sce.PlayStation.PUI.UI2", "Scene");
    if (!s_widget_class || !s_label_class || !s_panel_class || !scene_class) {
        return false;
    }

    /* Locate RootWidget from Game/Current scene */
    MonoProperty* root_prop = mono_class_get_property_from_name(scene_class, "RootWidget");
    if (root_prop) {
        MonoMethod* getter = mono_property_get_get_method(root_prop);
        if (getter) {
            s_root_widget = invoke_method(getter, nullptr, nullptr);
        }
    }

    if (!s_root_widget) return false;

    /* Build HUD Background Panel */
    s_bg_panel = mono_object_new(s_root_domain, s_panel_class);
    if (s_bg_panel) {
        mono_runtime_object_init(s_bg_panel);
        set_property_string(s_panel_class, s_bg_panel, "Name", "ps5_overlay_panel");
        set_property_float(s_panel_class, s_bg_panel, "X", 0.0f);
        set_property_float(s_panel_class, s_bg_panel, "Y", config->position == 1 ? 1040.0f : 0.0f);
        set_property_float(s_panel_class, s_bg_panel, "Width", 1920.0f);
        set_property_float(s_panel_class, s_bg_panel, "Height", (float)(config->font_size + 14));
        set_property_bool(s_panel_class, s_bg_panel, "BackgroundVisibility", config->background_panel);
        set_property_float(s_panel_class, s_bg_panel, "BackgroundOpacity", 0.75f);

        /* Append panel to root */
        MonoMethod* append_method = mono_class_get_method_from_name(s_widget_class, "AppendChild", 1);
        if (append_method) {
            void* args[1] = { s_bg_panel };
            invoke_method(append_method, s_root_widget, args);
        }
    }

    /* Build HUD Label */
    s_hud_label = mono_object_new(s_root_domain, s_label_class);
    if (s_hud_label) {
        mono_runtime_object_init(s_hud_label);
        set_property_string(s_label_class, s_hud_label, "Name", "ps5_overlay_label");
        set_property_float(s_label_class, s_hud_label, "MarginLeft", 24.0f);
        set_property_float(s_label_class, s_hud_label, "MarginTop", 4.0f);
        set_property_float(s_label_class, s_hud_label, "Width", 1872.0f);
        set_property_int(s_label_class, s_hud_label, "HorizontalAlignment", 0); /* Left */
        set_property_int(s_label_class, s_hud_label, "VerticalAlignment", 0);   /* Top */
        set_property_string(s_label_class, s_hud_label, "Text", "PS5 Overlay Loading...");

        /* Append label to panel or root */
        MonoMethod* append_method = mono_class_get_method_from_name(s_widget_class, "AppendChild", 1);
        if (append_method) {
            void* args[1] = { s_hud_label };
            invoke_method(append_method, s_bg_panel ? s_bg_panel : s_root_widget, args);
        }
    }

    s_shellui_ready = (s_hud_label != nullptr);
    return s_shellui_ready;
}

#endif

static bool s_is_visible = true;
static OverlayConfig s_current_config;
static pthread_mutex_t s_ui_mutex = PTHREAD_MUTEX_INITIALIZER;

bool overlay_ui_init(const OverlayConfig* config) {
    if (!config) return false;

    pthread_mutex_lock(&s_ui_mutex);
    s_current_config = *config;
    s_is_visible = config->enabled;

#if defined(__PS5__) || defined(PS5)
    init_shellui_pui(config);
#else
    printf("[OVERLAY_UI] Initialized (Position=%s, FontSize=%d)\n",
           config->position == 1 ? "Bottom" : "Top", config->font_size);
#endif

    pthread_mutex_unlock(&s_ui_mutex);
    return true;
}

bool overlay_ui_update(const char* hud_text) {
    if (!hud_text || !s_is_visible) return false;

    pthread_mutex_lock(&s_ui_mutex);

#if defined(__PS5__) || defined(PS5)
    if (s_shellui_ready && s_hud_label && s_label_class) {
        set_property_string(s_label_class, s_hud_label, "Text", hud_text);
    }
#else
    printf("[HUD] %s\n", hud_text);
#endif

    pthread_mutex_unlock(&s_ui_mutex);
    return true;
}

void overlay_ui_set_visible(bool visible) {
    pthread_mutex_lock(&s_ui_mutex);
    s_is_visible = visible;

#if defined(__PS5__) || defined(PS5)
    if (s_shellui_ready && s_bg_panel && s_panel_class) {
        set_property_bool(s_panel_class, s_bg_panel, "Visibility", visible);
    }
#endif

    pthread_mutex_unlock(&s_ui_mutex);
}

void overlay_ui_shutdown(void) {
    pthread_mutex_lock(&s_ui_mutex);

#if defined(__PS5__) || defined(PS5)
    if (s_shellui_ready && s_root_widget && s_widget_class) {
        MonoMethod* remove_method = mono_class_get_method_from_name(s_widget_class, "RemoveChild", 1);
        if (remove_method && s_bg_panel) {
            void* args[1] = { s_bg_panel };
            invoke_method(remove_method, s_root_widget, args);
        }
    }
    s_shellui_ready = false;
    s_bg_panel = nullptr;
    s_hud_label = nullptr;
    s_root_widget = nullptr;
#endif

    pthread_mutex_unlock(&s_ui_mutex);
}
