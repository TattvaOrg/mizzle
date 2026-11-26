#include "desktop_capture.hpp"
#include "glass_renderer.hpp"
#include "preview_dump.hpp"
#include "ui_widgets.hpp"

#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

namespace {

struct AppState {
    GtkWindow* window = nullptr;
    GtkGLArea* glArea = nullptr;

    bool overlayMode = true;
    int backdropMode = 0; // 0=Live Desktop, 1=Silk Wave, 2=Optics Lab
    bool glReady = false;
    bool capturing = false;

    lg::GlassRenderer renderer;
    lg::LiquidGlassUi ui;
    lg::CapturedFrame pendingFrame;
    bool hasPendingUpload = false;

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
};

void refreshDesktopBackdrop(AppState* state) {
    lg::CapturedFrame frame;
    std::string err;
    if (lg::captureLiveDesktop(frame, err)) {
        state->pendingFrame = std::move(frame);
        state->hasPendingUpload = true;
        state->ui.setBackdropModeLabel(state->backdropMode, true);
    } else {
        std::cerr << "[liquid-glass-ui] Desktop capture unavailable (" << err
                  << "); using animated Optics Lab backdrop.\n";
        if (state->backdropMode == 0) state->backdropMode = 2;
        state->ui.setBackdropModeLabel(state->backdropMode, false);
    }
}

gboolean onRecaptureAfterHide(gpointer data) {
    auto* state = static_cast<AppState*>(data);
    refreshDesktopBackdrop(state);
    gtk_widget_set_visible(GTK_WIDGET(state->window), TRUE);
    gtk_gl_area_queue_render(state->glArea);
    state->capturing = false;
    return G_SOURCE_REMOVE;
}

void triggerRecapture(AppState* state) {
    if (state->capturing) return;
    state->backdropMode = 0;
    if (!state->overlayMode) {
        refreshDesktopBackdrop(state);
        state->ui.setBackdropModeLabel(state->backdropMode, state->hasPendingUpload || state->renderer.hasBackdropFrame());
        gtk_gl_area_queue_render(state->glArea);
        return;
    }
    state->capturing = true;
    gtk_widget_set_visible(GTK_WIDGET(state->window), FALSE);
    g_timeout_add(110, onRecaptureAfterHide, state);
}

void onRealize(GtkGLArea* area, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != nullptr) {
        std::cerr << "[liquid-glass-ui] GtkGLArea realize error\n";
        return;
    }

    std::string err;
    if (!state->renderer.init(err)) {
        std::cerr << "[liquid-glass-ui] Renderer init failed: " << err << "\n";
        return;
    }
    if (state->hasPendingUpload) {
        state->renderer.setBackdropFrame(state->pendingFrame);
        state->hasPendingUpload = false;
    }
    state->glReady = true;
}

void onUnrealize(GtkGLArea* area, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    gtk_gl_area_make_current(area);
    state->renderer.shutdown();
    state->glReady = false;
}

gboolean onRender(GtkGLArea* area, GdkGLContext* context, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    if (!state->glReady) return FALSE;

    if (state->hasPendingUpload) {
        state->renderer.setBackdropFrame(state->pendingFrame);
        state->hasPendingUpload = false;
    }

    int w = gtk_widget_get_width(GTK_WIDGET(area));
    int h = gtk_widget_get_height(GTK_WIDGET(area));
    int scale = gtk_widget_get_scale_factor(GTK_WIDGET(area));
    int fbW = w * scale;
    int fbH = h * scale;
    if (fbW <= 0 || fbH <= 0) return TRUE;

    state->ui.setViewport(w, h, scale);

    int uiW = 0, uiH = 0;
    const uint8_t* uiPixels = nullptr;
    if (state->ui.renderUiIfDirty(uiW, uiH, uiPixels)) {
        state->renderer.uploadUiOverlay(uiW, uiH, uiPixels);
    }

    auto now = std::chrono::steady_clock::now();
    float timeSec = std::chrono::duration<float>(now - state->startTime).count();

    auto panels = state->ui.collectGlassPanels();
    for (auto& p : panels) {
        p.x *= static_cast<float>(scale);
        p.y *= static_cast<float>(scale);
        p.w *= static_cast<float>(scale);
        p.h *= static_cast<float>(scale);
        p.radius *= static_cast<float>(scale);
    }

    bool useLive = (state->backdropMode == 0) && state->renderer.hasBackdropFrame();
    int bgStyle = (state->backdropMode == 2) ? 1 : 0;
    bool transparentOutsidePanels = state->overlayMode && useLive;
    state->renderer.render(fbW,
                           fbH,
                           timeSec,
                           useLive,
                           bgStyle,
                           transparentOutsidePanels,
                           state->ui.clickPixelX(),
                           state->ui.clickPixelY(),
                           state->ui.clickAgeSec(),
                           state->ui.clickActive(),
                           state->ui.material(),
                           panels);
    return TRUE;
}

gboolean onFrameTick(GtkWidget* widget, GdkFrameClock* clock, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - state->lastTick).count();
    state->lastTick = now;
    dt = std::min(dt, 0.05f);

    float timeSec = std::chrono::duration<float>(now - state->startTime).count();
    bool uiChanged = state->ui.tick(dt, timeSec);

    auto actions = state->ui.consumeActions();
    if (actions.requestExit) {
        gtk_window_close(state->window);
        return G_SOURCE_REMOVE;
    }
    if (actions.requestRecapture) {
        triggerRecapture(state);
    }
    if (actions.requestToggleBackdropMode) {
        state->backdropMode = (state->backdropMode + 1) % 3;
        if (state->backdropMode == 0 && !state->renderer.hasBackdropFrame() && !state->hasPendingUpload) {
            refreshDesktopBackdrop(state);
        }
        if (state->backdropMode == 0 && !state->renderer.hasBackdropFrame() && !state->hasPendingUpload) {
            state->backdropMode = 1;
        }
        state->ui.setBackdropModeLabel(state->backdropMode, state->renderer.hasBackdropFrame() || state->hasPendingUpload);
        uiChanged = true;
    }

    if (uiChanged || state->backdropMode != 0 || !state->renderer.hasBackdropFrame()) {
        gtk_gl_area_queue_render(state->glArea);
    }
    return G_SOURCE_CONTINUE;
}

void onMotion(GtkEventControllerMotion* ctrl, double x, double y, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    state->ui.onPointerMove(static_cast<float>(x), static_cast<float>(y));
    if (state->ui.isDragging()) {
        gtk_gl_area_queue_render(state->glArea);
    }
}

void onLeave(GtkEventControllerMotion* ctrl, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    state->ui.onPointerLeave();
}

void onPressed(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    gtk_widget_grab_focus(GTK_WIDGET(state->glArea));
    state->ui.onPointerDown(static_cast<float>(x), static_cast<float>(y));
    gtk_gl_area_queue_render(state->glArea);
}

void onReleased(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    state->ui.onPointerUp(static_cast<float>(x), static_cast<float>(y));
    gtk_gl_area_queue_render(state->glArea);
}

gboolean onKeyPressed(GtkEventControllerKey* ctrl, guint keyval, guint keycode, GdkModifierType mods, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    guint32 uni = gdk_keyval_to_unicode(keyval);
    bool isCtrl = (mods & GDK_CONTROL_MASK) != 0;
    if (state->ui.onKeyPressed(keyval, uni, isCtrl)) {
        gtk_gl_area_queue_render(state->glArea);
        return TRUE;
    }
    return FALSE;
}

void onActivate(GtkApplication* app, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);

    // Capture live desktop BEFORE mapping our overlay so our panels never appear in the backdrop
    if (state->backdropMode == 0) {
        refreshDesktopBackdrop(state);
    } else {
        state->ui.setBackdropModeLabel(state->backdropMode, false);
    }

    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(
        css,
        "window.glass-root { background-color: transparent; background-image: none; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkWidget* win = gtk_application_window_new(app);
    state->window = GTK_WINDOW(win);
    gtk_window_set_title(state->window, "Liquid Glass UI");
    gtk_widget_add_css_class(win, "glass-root");

    if (state->overlayMode && gtk_layer_is_supported()) {
        gtk_layer_init_for_window(state->window);
        gtk_layer_set_namespace(state->window, "liquid-glass-ui");
        gtk_layer_set_layer(state->window, GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(state->window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(state->window, -1);
        gtk_layer_set_keyboard_mode(state->window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
    } else {
        if (state->overlayMode) {
            std::cerr << "[liquid-glass-ui] Layer-shell unsupported here; falling back to window mode.\n";
        }
        state->overlayMode = false;
        gtk_window_set_default_size(state->window, 1360, 820);
    }

    GtkWidget* glWidget = gtk_gl_area_new();
    state->glArea = GTK_GL_AREA(glWidget);
    gtk_gl_area_set_auto_render(state->glArea, FALSE);
    gtk_widget_set_hexpand(glWidget, TRUE);
    gtk_widget_set_vexpand(glWidget, TRUE);
    gtk_widget_set_can_focus(glWidget, TRUE);
    gtk_widget_set_focusable(glWidget, TRUE);

    g_signal_connect(glWidget, "realize", G_CALLBACK(onRealize), state);
    g_signal_connect(glWidget, "unrealize", G_CALLBACK(onUnrealize), state);
    g_signal_connect(glWidget, "render", G_CALLBACK(onRender), state);

    GtkEventController* motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(onMotion), state);
    g_signal_connect(motion, "leave", G_CALLBACK(onLeave), state);
    gtk_widget_add_controller(glWidget, motion);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(onPressed), state);
    g_signal_connect(click, "released", G_CALLBACK(onReleased), state);
    gtk_widget_add_controller(glWidget, GTK_EVENT_CONTROLLER(click));

    GtkEventController* keyCtrl = gtk_event_controller_key_new();
    g_signal_connect(keyCtrl, "key-pressed", G_CALLBACK(onKeyPressed), state);
    gtk_widget_add_controller(win, keyCtrl);

    gtk_widget_add_tick_callback(glWidget, onFrameTick, state, nullptr);

    gtk_window_set_child(state->window, glWidget);
    gtk_window_present(state->window);
    gtk_widget_grab_focus(glWidget);
}

void printHelp() {
    std::cout << "Usage: liquid-glass-ui [options]\n"
              << "  --overlay   Launch as full-screen Wayland layer-shell desktop HUD (default)\n"
              << "  --window    Launch in a normal resizable window\n"
              << "  --demo-bg   Start with animated procedural motion backdrop\n"
              << "  --help      Show this help\n\n"
              << "Hotkeys:\n"
              << "  1/2/3/4     Presets: Clear / Frosted / Smoked / Aqua\n"
              << "  I           Cycle Dock Icon Styles: Default / Dark / Clear Glass / Tinted Glass\n"
              << "  R           Re-capture live desktop behind the glass\n"
              << "  B           Toggle Live Desktop vs Animated Silk-Wave backdrop\n"
              << "  F / A / W   Toggle Finder Glass Window / Alert Modal / Desktop Widgets\n"
              << "  T / C / S   Toggle Shader Lab / Control Center / Spotlight\n"
              << "  Esc / Q     Exit\n";
}

} // namespace

int main(int argc, char** argv) {
    AppState state;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--window") == 0) {
            state.overlayMode = false;
        } else if (std::strcmp(argv[i], "--overlay") == 0) {
            state.overlayMode = true;
        } else if (std::strcmp(argv[i], "--demo-bg") == 0) {
            state.backdropMode = 2; // Optics Lab (stripes + glowing orbs)
        } else if (std::strcmp(argv[i], "--silk-bg") == 0) {
            state.backdropMode = 1;
        } else if (std::strcmp(argv[i], "--render-preview") == 0 && i + 1 < argc) {
            std::string err;
            if (!lg::renderPreviewPng(argv[i + 1], 1440, 900, err)) {
                std::cerr << "Preview render failed: " << err << "\n";
                return 1;
            }
            return 0;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        }
    }

    GtkApplication* app = gtk_application_new("org.tattva.mizzle", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(onActivate), &state);
    int fakeArgc = 1;
    int status = g_application_run(G_APPLICATION(app), fakeArgc, argv);
    g_object_unref(app);
    return status;
}
