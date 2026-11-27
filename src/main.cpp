#include "config_store.hpp"
#include "desktop_capture.hpp"
#include "glass_renderer.hpp"
#include "preview_dump.hpp"
#include "system_info.hpp"
#include "ui_widgets.hpp"

#include <glib-unix.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

namespace {

enum class RunMode {
    DesktopEnv,
    OverlayDemo,
    WindowDemo,
};

struct AppState;

struct SurfaceWindow {
    lg::SurfaceRole role = lg::SurfaceRole::All;
    GtkWindow* window = nullptr;
    GtkGLArea* glArea = nullptr;
    lg::GlassRenderer renderer;
    bool glReady = false;
    bool pendingFrameUpload = false;
    AppState* app = nullptr;
};

struct AppState {
    RunMode mode = RunMode::DesktopEnv;
    int backdropMode = 0; // 0=Live Desktop, 1=Lake Tahoe, 2=Optics Lab
    bool backdropExplicit = false;
    bool capturing = false;
    bool lastDockAutoHide = false;
    bool lastExclusiveKbd = false;

    std::array<SurfaceWindow, 5> surfaces{};
    lg::LiquidGlassUi ui;
    lg::SystemMonitor monitor;
    lg::CapturedFrame pendingFrame;
    bool hasBackdropFrame = false;

    int ipcListenFd = -1;
    guint ipcSourceId = 0;
    std::string ipcSockPath;
    std::string exePath;

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
};

std::string resolveIpcSocketPath() {
    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR")) {
        if (*xdg) return std::string(xdg) + "/mizzle.sock";
    }
    return "/tmp/mizzle-" + std::to_string(getuid()) + ".sock";
}

void refreshDesktopBackdrop(AppState* state) {
    lg::CapturedFrame frame;
    std::string err;
    if (lg::captureLiveDesktop(frame, err)) {
        state->pendingFrame = std::move(frame);
        state->hasBackdropFrame = true;
        for (auto& sw : state->surfaces) {
            if (sw.window) sw.pendingFrameUpload = true;
        }
        state->ui.setBackdropModeLabel(state->backdropMode, true);
    } else {
        if (state->mode != RunMode::DesktopEnv) {
            std::cerr << "[mizzle] Desktop capture unavailable (" << err
                      << "); using animated backdrop.\n";
            if (state->backdropMode == 0) state->backdropMode = 2;
        }
        state->ui.setBackdropModeLabel(state->backdropMode, false);
    }
}

void updateSurfaceInputRegion(SurfaceWindow* sw) {
    if (!sw || !sw->window || sw->app->mode != RunMode::DesktopEnv) return;
    if (sw->role == lg::SurfaceRole::MenuBar) return;

    GdkSurface* gdkSurf = gtk_native_get_surface(GTK_NATIVE(sw->window));
    if (!gdkSurf) return;

    bool fullCapture = false;
    auto rects = sw->app->ui.collectInputRects(sw->role, fullCapture);
    if (fullCapture) {
        gdk_surface_set_input_region(gdkSurf, nullptr);
        return;
    }

    cairo_region_t* region = cairo_region_create();
    for (const auto& r : rects) {
        if (r.w <= 0.0f || r.h <= 0.0f) continue;
        cairo_rectangle_int_t cr{
            static_cast<int>(std::floor(r.x)),
            static_cast<int>(std::floor(r.y)),
            static_cast<int>(std::ceil(r.w)),
            static_cast<int>(std::ceil(r.h)),
        };
        cairo_region_union_rectangle(region, &cr);
    }
    gdk_surface_set_input_region(gdkSurf, region);
    cairo_region_destroy(region);
}

void syncEnvWindowVisibilityAndZones(AppState* state) {
    if (state->mode != RunMode::DesktopEnv) return;

    auto& widWin = state->surfaces[static_cast<int>(lg::SurfaceRole::Widgets)];
    if (widWin.window) {
        bool shouldShow = state->ui.showWidgets();
        if (gtk_widget_get_visible(GTK_WIDGET(widWin.window)) != (shouldShow ? TRUE : FALSE)) {
            gtk_widget_set_visible(GTK_WIDGET(widWin.window), shouldShow ? TRUE : FALSE);
        }
        if (shouldShow) updateSurfaceInputRegion(&widWin);
    }

    auto& dockWin = state->surfaces[static_cast<int>(lg::SurfaceRole::Dock)];
    if (dockWin.window) {
        bool autoHide = state->ui.dockAutoHide();
        if (autoHide != state->lastDockAutoHide) {
            state->lastDockAutoHide = autoHide;
            gtk_layer_set_exclusive_zone(dockWin.window, autoHide ? 0 : 62);
        }
        updateSurfaceInputRegion(&dockWin);
    }

    auto& popWin = state->surfaces[static_cast<int>(lg::SurfaceRole::Popups)];
    if (popWin.window) {
        bool showPop = state->ui.hasAnyPopupVisible();
        bool wasVisible = gtk_widget_get_visible(GTK_WIDGET(popWin.window)) != FALSE;
        if (showPop != wasVisible) {
            gtk_widget_set_visible(GTK_WIDGET(popWin.window), showPop ? TRUE : FALSE);
            if (showPop && popWin.glArea) {
                gtk_widget_grab_focus(GTK_WIDGET(popWin.glArea));
            }
        }
        if (showPop) {
            bool excl = state->ui.needsExclusiveKeyboard();
            if (excl != state->lastExclusiveKbd) {
                state->lastExclusiveKbd = excl;
                gtk_layer_set_keyboard_mode(popWin.window,
                    excl ? GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE : GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
            }
            updateSurfaceInputRegion(&popWin);
        }
    }
}

gboolean onRecaptureAfterHide(gpointer data) {
    auto* state = static_cast<AppState*>(data);
    refreshDesktopBackdrop(state);
    for (auto& sw : state->surfaces) {
        if (!sw.window) continue;
        if (state->mode == RunMode::DesktopEnv) {
            if (sw.role == lg::SurfaceRole::Widgets && !state->ui.showWidgets()) continue;
            if (sw.role == lg::SurfaceRole::Popups && !state->ui.hasAnyPopupVisible()) continue;
        }
        gtk_widget_set_visible(GTK_WIDGET(sw.window), TRUE);
        if (sw.glArea) gtk_gl_area_queue_render(sw.glArea);
    }
    state->capturing = false;
    return G_SOURCE_REMOVE;
}

void triggerRecapture(AppState* state) {
    if (state->capturing) return;
    state->backdropMode = 0;
    if (state->mode == RunMode::WindowDemo) {
        refreshDesktopBackdrop(state);
        for (auto& sw : state->surfaces) {
            if (sw.glArea) gtk_gl_area_queue_render(sw.glArea);
        }
        return;
    }
    state->capturing = true;
    for (auto& sw : state->surfaces) {
        if (sw.window) gtk_widget_set_visible(GTK_WIDGET(sw.window), FALSE);
    }
    g_timeout_add(120, onRecaptureAfterHide, state);
}

void onRealize(GtkGLArea* area, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != nullptr) {
        std::cerr << "[mizzle] GtkGLArea realize error\n";
        return;
    }
    std::string err;
    if (!sw->renderer.init(err)) {
        std::cerr << "[mizzle] Renderer init failed: " << err << "\n";
        return;
    }
    if (sw->app->hasBackdropFrame) {
        sw->renderer.setBackdropFrame(sw->app->pendingFrame);
        sw->pendingFrameUpload = false;
    }
    sw->glReady = true;
    updateSurfaceInputRegion(sw);
}

void onUnrealize(GtkGLArea* area, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    gtk_gl_area_make_current(area);
    sw->renderer.shutdown();
    sw->glReady = false;
}

gboolean onRender(GtkGLArea* area, GdkGLContext* context, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    AppState* state = sw->app;
    if (!sw->glReady) return FALSE;

    if (sw->pendingFrameUpload && state->hasBackdropFrame) {
        sw->renderer.setBackdropFrame(state->pendingFrame);
        sw->pendingFrameUpload = false;
    }

    int w = gtk_widget_get_width(GTK_WIDGET(area));
    int h = gtk_widget_get_height(GTK_WIDGET(area));
    int scale = gtk_widget_get_scale_factor(GTK_WIDGET(area));
    int fbW = w * scale;
    int fbH = h * scale;
    if (fbW <= 0 || fbH <= 0) return TRUE;

    state->ui.setSurfaceViewport(sw->role, w, h, scale);

    int uiW = 0, uiH = 0;
    const uint8_t* uiPixels = nullptr;
    if (state->ui.renderUiIfDirty(uiW, uiH, uiPixels, sw->role)) {
        sw->renderer.uploadUiOverlay(uiW, uiH, uiPixels);
    }

    auto now = std::chrono::steady_clock::now();
    float timeSec = std::chrono::duration<float>(now - state->startTime).count();

    auto panels = state->ui.collectGlassPanels(sw->role);
    for (auto& p : panels) {
        p.x *= static_cast<float>(scale);
        p.y *= static_cast<float>(scale);
        p.w *= static_cast<float>(scale);
        p.h *= static_cast<float>(scale);
        p.radius *= static_cast<float>(scale);
    }

    bool isEnv = (state->mode == RunMode::DesktopEnv);
    bool useLive = (state->backdropMode == 0) && sw->renderer.hasBackdropFrame();
    int bgStyle = (state->backdropMode == 2) ? 1 : 0;
    bool transparentOutsidePanels = isEnv || (state->mode == RunMode::OverlayDemo && useLive);
    bool translucentGlass = isEnv && (state->backdropMode == 0);

    bool clickMatch = state->ui.clickActive() &&
                      (sw->role == lg::SurfaceRole::All || state->ui.clickSurface() == sw->role);

    sw->renderer.render(fbW,
                        fbH,
                        timeSec,
                        useLive,
                        bgStyle,
                        transparentOutsidePanels,
                        state->ui.clickPixelX(),
                        state->ui.clickPixelY(),
                        state->ui.clickAgeSec(),
                        clickMatch,
                        state->ui.material(),
                        panels,
                        translucentGlass);

    updateSurfaceInputRegion(sw);
    return TRUE;
}

void queueAllVisibleSurfaces(AppState* state) {
    for (auto& sw : state->surfaces) {
        if (sw.window && sw.glArea && gtk_widget_get_visible(GTK_WIDGET(sw.window))) {
            gtk_gl_area_queue_render(sw.glArea);
        }
    }
}

void saveCurrentConfig(const AppState* state) {
    lg::MizzleConfig cfg = state->ui.exportConfig();
    cfg.backdropMode = state->backdropMode;
    lg::saveMizzleConfig(cfg);
}

gboolean onTickTimeout(gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - state->lastTick).count();
    state->lastTick = now;
    dt = std::min(dt, 0.05f);

    state->ui.setTelemetry(state->monitor.snapshot());

    float timeSec = std::chrono::duration<float>(now - state->startTime).count();
    bool uiChanged = state->ui.tick(dt, timeSec);

    auto actions = state->ui.consumeActions();
    if (actions.configChanged) {
        saveCurrentConfig(state);
    }
    if (actions.requestExit) {
        for (auto& sw : state->surfaces) {
            if (sw.window) gtk_window_close(sw.window);
        }
        return G_SOURCE_REMOVE;
    }
    if (actions.requestRestart) {
        saveCurrentConfig(state);
        lg::spawnDetached("mizzle restart >/dev/null 2>&1 || (" + state->exePath + " --env >/dev/null 2>&1)");
        for (auto& sw : state->surfaces) {
            if (sw.window) gtk_window_close(sw.window);
        }
        return G_SOURCE_REMOVE;
    }
    if (actions.requestRecapture) {
        triggerRecapture(state);
    }
    if (actions.requestToggleBackdropMode) {
        state->backdropMode = (state->backdropMode + 1) % 3;
        if (state->backdropMode == 0 && !state->hasBackdropFrame) {
            refreshDesktopBackdrop(state);
        }
        state->ui.setBackdropModeLabel(state->backdropMode, state->hasBackdropFrame);
        saveCurrentConfig(state);
        uiChanged = true;
    }

    syncEnvWindowVisibilityAndZones(state);

    bool proceduralAnimated = (state->backdropMode != 0) ||
                              (state->mode != RunMode::DesktopEnv && !state->hasBackdropFrame);
    if (uiChanged || proceduralAnimated) {
        queueAllVisibleSurfaces(state);
    }
    return G_SOURCE_CONTINUE;
}

void onMotion(GtkEventControllerMotion* ctrl, double x, double y, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    sw->app->ui.onPointerMove(static_cast<float>(x), static_cast<float>(y), sw->role);
    if (sw->app->ui.isDragging() || sw->role == lg::SurfaceRole::Dock || sw->role == lg::SurfaceRole::Widgets || sw->role == lg::SurfaceRole::Popups) {
        gtk_gl_area_queue_render(sw->glArea);
    }
}

void onLeave(GtkEventControllerMotion* ctrl, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    sw->app->ui.onPointerLeave(sw->role);
    if (sw->glArea) gtk_gl_area_queue_render(sw->glArea);
}

void onPressed(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    int btn = static_cast<int>(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)));
    if (sw->glArea) gtk_widget_grab_focus(GTK_WIDGET(sw->glArea));
    sw->app->ui.onPointerDown(static_cast<float>(x), static_cast<float>(y), btn, sw->role);
    syncEnvWindowVisibilityAndZones(sw->app);
    queueAllVisibleSurfaces(sw->app);
}

void onReleased(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    sw->app->ui.onPointerUp(static_cast<float>(x), static_cast<float>(y), sw->role);
    syncEnvWindowVisibilityAndZones(sw->app);
    queueAllVisibleSurfaces(sw->app);
}

gboolean onKeyPressed(GtkEventControllerKey* ctrl, guint keyval, guint keycode, GdkModifierType mods, gpointer userData) {
    auto* sw = static_cast<SurfaceWindow*>(userData);
    guint32 uni = gdk_keyval_to_unicode(keyval);
    bool isCtrl = (mods & GDK_CONTROL_MASK) != 0;
    if (sw->app->ui.onKeyPressed(keyval, uni, isCtrl)) {
        syncEnvWindowVisibilityAndZones(sw->app);
        queueAllVisibleSurfaces(sw->app);
        return TRUE;
    }
    return FALSE;
}

void setupGlWindowCommon(GtkApplication* app, SurfaceWindow* sw, const char* title) {
    GtkWidget* win = gtk_application_window_new(app);
    sw->window = GTK_WINDOW(win);
    gtk_window_set_title(sw->window, title);
    gtk_widget_add_css_class(win, "glass-root");

    GtkWidget* glWidget = gtk_gl_area_new();
    sw->glArea = GTK_GL_AREA(glWidget);
    gtk_gl_area_set_auto_render(sw->glArea, FALSE);
    gtk_widget_set_hexpand(glWidget, TRUE);
    gtk_widget_set_vexpand(glWidget, TRUE);
    gtk_widget_set_can_focus(glWidget, TRUE);
    gtk_widget_set_focusable(glWidget, TRUE);

    g_signal_connect(glWidget, "realize", G_CALLBACK(onRealize), sw);
    g_signal_connect(glWidget, "unrealize", G_CALLBACK(onUnrealize), sw);
    g_signal_connect(glWidget, "render", G_CALLBACK(onRender), sw);

    GtkEventController* motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(onMotion), sw);
    g_signal_connect(motion, "leave", G_CALLBACK(onLeave), sw);
    gtk_widget_add_controller(glWidget, motion);

    GtkGesture* click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0); // Any button (left & right)
    g_signal_connect(click, "pressed", G_CALLBACK(onPressed), sw);
    g_signal_connect(click, "released", G_CALLBACK(onReleased), sw);
    gtk_widget_add_controller(glWidget, GTK_EVENT_CONTROLLER(click));

    GtkEventController* keyCtrl = gtk_event_controller_key_new();
    g_signal_connect(keyCtrl, "key-pressed", G_CALLBACK(onKeyPressed), sw);
    gtk_widget_add_controller(win, keyCtrl);

    gtk_window_set_child(sw->window, glWidget);
}

void handleIpcCommand(AppState* state, const std::string& cmd) {
    if (cmd == "spotlight") state->ui.toggleSpotlight(true);
    else if (cmd == "cc" || cmd == "control-center") state->ui.toggleControlCenter();
    else if (cmd == "settings" || cmd == "tuner") state->ui.toggleTuner();
    else if (cmd == "apps" || cmd == "finder") state->ui.toggleFinder();
    else if (cmd == "widgets") state->ui.toggleWidgets();
    else if (cmd == "edit-widgets") state->ui.toggleWidgetEditMode();
    else if (cmd == "recapture") triggerRecapture(state);
    else if (cmd == "quit" || cmd == "stop") {
        for (auto& sw : state->surfaces) {
            if (sw.window) gtk_window_close(sw.window);
        }
        return;
    }
    syncEnvWindowVisibilityAndZones(state);
    queueAllVisibleSurfaces(state);
}

gboolean onIpcSocketReadable(gint fd, GIOCondition cond, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);
    int client = accept(fd, nullptr, nullptr);
    if (client >= 0) {
        char buf[256];
        ssize_t n = read(client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::string cmd(buf);
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r' || cmd.back() == ' ')) {
                cmd.pop_back();
            }
            handleIpcCommand(state, cmd);
        }
        close(client);
    }
    return G_SOURCE_CONTINUE;
}

void startIpcServer(AppState* state) {
    state->ipcSockPath = resolveIpcSocketPath();
    unlink(state->ipcSockPath.c_str());

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (state->ipcSockPath.size() >= sizeof(addr.sun_path)) {
        close(fd);
        return;
    }
    std::strncpy(addr.sun_path, state->ipcSockPath.c_str(), sizeof(addr.sun_path) - 1);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || listen(fd, 8) < 0) {
        close(fd);
        return;
    }
    state->ipcListenFd = fd;
    state->ipcSourceId = g_unix_fd_add(fd, G_IO_IN, onIpcSocketReadable, state);
}

void onActivate(GtkApplication* app, gpointer userData) {
    auto* state = static_cast<AppState*>(userData);

    lg::MizzleConfig cfg;
    if (lg::loadMizzleConfig(cfg)) {
        state->ui.applyConfig(cfg);
        if (!state->backdropExplicit) {
            state->backdropMode = cfg.backdropMode;
        }
    }

    // Capture live desktop BEFORE mapping any mizzle surfaces
    if (state->backdropMode == 0) {
        refreshDesktopBackdrop(state);
    } else {
        state->ui.setBackdropModeLabel(state->backdropMode, false);
    }

    state->monitor.start();
    state->ui.setTelemetry(state->monitor.snapshot());

    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(
        css,
        "window.glass-root { background-color: transparent; background-image: none; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    if (state->mode != RunMode::WindowDemo && !gtk_layer_is_supported()) {
        std::cerr << "[mizzle] Wayland layer-shell unsupported here; falling back to window mode.\n";
        state->mode = RunMode::WindowDemo;
    }

    if (state->mode == RunMode::DesktopEnv) {
        // 1. Top Menu Bar (28px exclusive zone)
        auto& bar = state->surfaces[static_cast<int>(lg::SurfaceRole::MenuBar)];
        bar.role = lg::SurfaceRole::MenuBar;
        bar.app = state;
        setupGlWindowCommon(app, &bar, "Mizzle Menu Bar");
        gtk_layer_init_for_window(bar.window);
        gtk_layer_set_namespace(bar.window, "mizzle-bar");
        gtk_layer_set_layer(bar.window, GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(bar.window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(bar.window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(bar.window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(bar.window, static_cast<int>(lg::LiquidGlassUi::kMenuBarHeight));
        gtk_layer_set_keyboard_mode(bar.window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        gtk_window_set_default_size(bar.window, 0, static_cast<int>(lg::LiquidGlassUi::kMenuBarHeight));
        gtk_window_present(bar.window);

        // 2. Bottom Dock
        auto& dock = state->surfaces[static_cast<int>(lg::SurfaceRole::Dock)];
        dock.role = lg::SurfaceRole::Dock;
        dock.app = state;
        setupGlWindowCommon(app, &dock, "Mizzle Dock");
        gtk_layer_init_for_window(dock.window);
        gtk_layer_set_namespace(dock.window, "mizzle-dock");
        gtk_layer_set_layer(dock.window, GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(dock.window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        state->lastDockAutoHide = state->ui.dockAutoHide();
        gtk_layer_set_exclusive_zone(dock.window, state->lastDockAutoHide ? 0 : 62);
        gtk_layer_set_keyboard_mode(dock.window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        gtk_window_set_default_size(dock.window, lg::LiquidGlassUi::kDockWindowWidth, lg::LiquidGlassUi::kDockWindowHeight);
        gtk_window_present(dock.window);

        // 3. Desktop Widgets (BOTTOM layer, input region restricted to widget rects)
        auto& wid = state->surfaces[static_cast<int>(lg::SurfaceRole::Widgets)];
        wid.role = lg::SurfaceRole::Widgets;
        wid.app = state;
        setupGlWindowCommon(app, &wid, "Mizzle Widgets");
        gtk_layer_init_for_window(wid.window);
        gtk_layer_set_namespace(wid.window, "mizzle-widgets");
        gtk_layer_set_layer(wid.window, GTK_LAYER_SHELL_LAYER_BOTTOM);
        gtk_layer_set_anchor(wid.window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(wid.window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(wid.window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(wid.window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(wid.window, -1);
        gtk_layer_set_keyboard_mode(wid.window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        if (state->ui.showWidgets()) {
            gtk_window_present(wid.window);
        }

        // 4. Floating Popups / HUD (OVERLAY layer, shown only when a popup/panel is open)
        auto& pop = state->surfaces[static_cast<int>(lg::SurfaceRole::Popups)];
        pop.role = lg::SurfaceRole::Popups;
        pop.app = state;
        setupGlWindowCommon(app, &pop, "Mizzle Overlay");
        gtk_layer_init_for_window(pop.window);
        gtk_layer_set_namespace(pop.window, "mizzle-overlay");
        gtk_layer_set_layer(pop.window, GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_anchor(pop.window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(pop.window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(pop.window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(pop.window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(pop.window, -1);
        gtk_layer_set_keyboard_mode(pop.window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
        if (state->ui.hasAnyPopupVisible()) {
            gtk_window_present(pop.window);
        }

        startIpcServer(state);
    } else {
        auto& full = state->surfaces[static_cast<int>(lg::SurfaceRole::All)];
        full.role = lg::SurfaceRole::All;
        full.app = state;
        setupGlWindowCommon(app, &full, "Mizzle Liquid Glass UI");
        if (state->mode == RunMode::OverlayDemo) {
            gtk_layer_init_for_window(full.window);
            gtk_layer_set_namespace(full.window, "mizzle-overlay");
            gtk_layer_set_layer(full.window, GTK_LAYER_SHELL_LAYER_OVERLAY);
            gtk_layer_set_anchor(full.window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
            gtk_layer_set_anchor(full.window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
            gtk_layer_set_anchor(full.window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
            gtk_layer_set_anchor(full.window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
            gtk_layer_set_exclusive_zone(full.window, -1);
            gtk_layer_set_keyboard_mode(full.window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
        } else {
            gtk_window_set_default_size(full.window, 1360, 820);
        }
        gtk_window_present(full.window);
        if (full.glArea) gtk_widget_grab_focus(GTK_WIDGET(full.glArea));
    }

    g_timeout_add(16, onTickTimeout, state);
}

bool sendIpcCommand(const std::string& cmd) {
    std::string sockPath = resolveIpcSocketPath();
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (sockPath.size() >= sizeof(addr.sun_path)) {
        close(fd);
        return false;
    }
    std::strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return false;
    }
    ssize_t unused = write(fd, cmd.data(), cmd.size());
    (void)unused;
    close(fd);
    return true;
}

void printHelp() {
    std::cout << "Usage: liquid-glass-ui [options]\n"
              << "  --env           Launch as non-blocking Wayland Desktop Environment UI (default)\n"
              << "  --overlay       Launch as full-screen Wayland layer-shell HUD showcase\n"
              << "  --window        Launch in a normal resizable preview window\n"
              << "  --silk-bg       Start with animated Lake Tahoe backdrop\n"
              << "  --demo-bg       Start with animated Optics Lab backdrop\n"
              << "  --ipc <cmd>     Send command to running Mizzle instance (spotlight, cc, settings, apps, widgets, edit-widgets, recapture, quit)\n"
              << "  --help          Show this help\n\n"
              << "Hotkeys:\n"
              << "  1/2/3/4         Presets: Clear / Frosted / Smoked / Aqua\n"
              << "  E               Toggle Widget Edit / Resize Mode\n"
              << "  I               Cycle Dock Icon Styles: Default / Dark / Clear Glass / Tinted Glass\n"
              << "  R               Re-capture live desktop behind the glass\n"
              << "  B               Cycle backdrop mode\n"
              << "  F / A / W       Toggle Applications Grid / Alert Modal / Desktop Widgets\n"
              << "  T / C / S       Toggle Shader Lab / Control Center / Spotlight\n"
              << "  Esc / Q         Close popup (or Exit in window/overlay mode)\n";
}

} // namespace

int main(int argc, char** argv) {
    AppState state;
    if (argc > 0 && argv[0]) state.exePath = argv[0];

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--env") == 0) {
            state.mode = RunMode::DesktopEnv;
        } else if (std::strcmp(argv[i], "--window") == 0) {
            state.mode = RunMode::WindowDemo;
        } else if (std::strcmp(argv[i], "--overlay") == 0) {
            state.mode = RunMode::OverlayDemo;
        } else if (std::strcmp(argv[i], "--demo-bg") == 0) {
            state.backdropMode = 2;
            state.backdropExplicit = true;
        } else if (std::strcmp(argv[i], "--silk-bg") == 0) {
            state.backdropMode = 1;
            state.backdropExplicit = true;
        } else if (std::strcmp(argv[i], "--ipc") == 0 && i + 1 < argc) {
            return sendIpcCommand(argv[i + 1]) ? 0 : 1;
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

    state.monitor.stop();
    if (state.ipcSourceId != 0) g_source_remove(state.ipcSourceId);
    if (state.ipcListenFd >= 0) close(state.ipcListenFd);
    if (!state.ipcSockPath.empty()) unlink(state.ipcSockPath.c_str());

    g_object_unref(app);
    return status;
}
