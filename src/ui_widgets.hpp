#pragma once

#include "config_store.hpp"
#include "glass_renderer.hpp"
#include "system_info.hpp"

#include <array>
#include <cairo.h>
#include <cstdint>
#include <string>
#include <vector>

namespace lg {

enum class GlassPreset {
    Clear = 0,
    Frosted = 1,
    Smoked = 2,
    Aqua = 3,
};

enum class DockIconStyle {
    Default = 0,
    Dark = 1,
    ClearGlass = 2,
    TintedGlass = 3,
};

enum class SurfaceRole {
    All = 0,
    MenuBar = 1,
    Dock = 2,
    Widgets = 3,
    Popups = 4,
};

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct UiActions {
    bool requestExit = false;
    bool requestRestart = false;
    bool requestRecapture = false;
    bool requestToggleBackdropMode = false;
    bool requestOpenSpotlight = false;
    bool configChanged = false;
};

class LiquidGlassUi {
public:
    LiquidGlassUi();
    ~LiquidGlassUi();

    LiquidGlassUi(const LiquidGlassUi&) = delete;
    LiquidGlassUi& operator=(const LiquidGlassUi&) = delete;

    void setViewport(int width, int height, int scaleFactor = 1);
    void setSurfaceViewport(SurfaceRole role, int width, int height, int scaleFactor = 1);

    void applyConfig(const MizzleConfig& cfg);
    MizzleConfig exportConfig() const;

    void setTelemetry(const SystemTelemetry& telemetry);
    void refreshDesktopApps();

    void applyPreset(GlassPreset preset);
    GlassPreset currentPreset() const { return preset_; }
    DockIconStyle dockIconStyle() const { return dockStyle_; }
    bool dockAutoHide() const { return dockAutoHide_; }
    bool showWidgets() const { return showWidgets_; }
    bool hasAnyPopupVisible() const;
    bool needsExclusiveKeyboard() const { return spotlightFocused_; }

    const GlassMaterial& material() const { return material_; }
    GlassMaterial& materialMutable() { return material_; }

    void setBackdropModeLabel(int modeIndex, bool hasCapture); // 0=Live, 1=Lake Tahoe, 2=Optics Lab

    float clickPixelX() const { return clickX_ * static_cast<float>(lastClickScale_); }
    float clickPixelY() const { return clickY_ * static_cast<float>(lastClickScale_); }
    float clickAgeSec() const { return clickAgeSec_; }
    bool clickActive() const { return clickActive_; }
    SurfaceRole clickSurface() const { return clickSurface_; }
    bool isDragging() const { return dragTarget_ != DragTarget::None; }

    bool tick(float dtSec, float timeSec);

    void onPointerMove(float x, float y, SurfaceRole role = SurfaceRole::All);
    void onPointerLeave(SurfaceRole role = SurfaceRole::All);
    void onPointerDown(float x, float y, int button = 1, SurfaceRole role = SurfaceRole::All);
    void onPointerUp(float x, float y, SurfaceRole role = SurfaceRole::All);
    bool onKeyPressed(uint32_t keyval, uint32_t unicodeChar, bool ctrl);

    // IPC toggles
    void toggleSpotlight(bool focus = true);
    void toggleControlCenter();
    void toggleWidgets();
    void toggleTuner();
    void toggleFinder();
    void toggleWidgetEditMode();
    void resetWidgetLayout();

    std::vector<GlassPanelSpec> collectGlassPanels(SurfaceRole role = SurfaceRole::All) const;
    std::vector<UiRect> collectInputRects(SurfaceRole role, bool& wantFullScreenCapture) const;
    bool renderUiIfDirty(int& outWidth, int& outHeight, const uint8_t*& outBgra, SurfaceRole role = SurfaceRole::All);
    UiActions consumeActions();

    static constexpr float kMenuBarHeight = 28.0f;
    static constexpr int kDockWindowWidth = 900;
    static constexpr int kDockWindowHeight = 124;

private:
    enum class DragTarget {
        None,
        Spotlight,
        ControlCenter,
        WidgetMove0,
        WidgetMove1,
        WidgetMove2,
        WidgetResize0,
        WidgetResize1,
        WidgetResize2,
        FinderWindow,
        Tuner,
        AlertDialog,
        BrightnessSlider,
        VolumeSlider,
        TunerRefraction,
        TunerDispersion,
        TunerFrost,
        TunerSpecular,
        TunerTint,
    };

    struct SurfaceBuffer {
        cairo_surface_t* surface = nullptr;
        int surfW = 0;
        int surfH = 0;
        int viewW = 1360;
        int viewH = 820;
        int scale = 1;
        bool initialized = false;
        bool dirty = true;
    };

    struct WidgetState {
        int sizeTier = 1; // 0=Small, 1=Medium, 2=Large
        float scale = 1.0f;
        float dx = 0.0f;
        float dy = 0.0f;
    };

    std::array<SurfaceBuffer, 5> surfaces_{};
    int width_ = 1360;
    int height_ = 820;
    int scale_ = 1;

    GlassPreset preset_ = GlassPreset::Clear;
    DockIconStyle dockStyle_ = DockIconStyle::Default;
    bool dockAutoHide_ = false;
    float dockHideProgress_ = 0.0f; // 0=visible, 1=hidden
    bool dockPointerInside_ = false;
    float dockPointerX_ = -1000.0f;
    float dockPointerY_ = -1000.0f;

    GlassMaterial material_{};
    int backdropMode_ = 0; // 0=Live Desktop, 1=Lake Tahoe, 2=Optics Lab
    bool hasDesktopCapture_ = false;
    bool envMode_ = false;

    bool showWidgets_ = true;
    bool editWidgets_ = false;
    int hoveredWidgetIdx_ = -1;
    bool showControlCenter_ = false;
    bool showSpotlight_ = false;
    bool showFinder_ = false;
    bool showAlert_ = false;
    bool showTuner_ = false;

    // Menu bar dropdowns: -1=none, 0=Apple/Mizzle, 1=File, 2=Edit, 3=View, 4=Window, 5=Help
    int activeMenu_ = -1;

    bool spotlightFocused_ = false;
    std::string spotlightQuery_;
    int spotlightSelected_ = 0;

    int finderPage_ = 0;
    int finderCategory_ = 0; // 0=All, 1=System, 2=Dev, 3=Media
    std::vector<DesktopApp> desktopApps_;

    SystemTelemetry telemetry_{};
    bool wifiOn_ = true;
    bool btOn_ = true;
    bool airdropOn_ = false;
    bool focusOn_ = false;
    bool darkAppearance_ = false;
    bool musicPlaying_ = false;
    int trackIndex_ = 0;
    bool quickCircleState_[8]{};

    float brightness_ = 0.72f;
    float volume_ = 0.76f;

    std::array<WidgetState, 3> widgets_{};

    float spotDx_ = 0.0f, spotDy_ = 0.0f;
    float ccDx_ = 0.0f, ccDy_ = 0.0f;
    float findDx_ = 0.0f, findDy_ = 0.0f;
    float tunerDx_ = 0.0f, tunerDy_ = 0.0f;
    float alertDx_ = 0.0f, alertDy_ = 0.0f;

    float pointerX_ = -1000.0f;
    float pointerY_ = -1000.0f;
    DragTarget dragTarget_ = DragTarget::None;
    float dragStartX_ = 0.0f;
    float dragStartY_ = 0.0f;
    float dragBaseDx_ = 0.0f;
    float dragBaseDy_ = 0.0f;
    float dragBaseScale_ = 1.0f;

    static constexpr int kDockItemCount = 12; // 11 apps + Trash
    std::array<float, kDockItemCount> dockIconScale_{};
    int hoveredDockIdx_ = -1;
    int pressedDockIdx_ = -1;
    int bouncingDockIdx_ = -1;
    float dockBounceAge_ = 0.0f;

    float clickX_ = 0.0f;
    float clickY_ = 0.0f;
    float clickAgeSec_ = 0.0f;
    bool clickActive_ = false;
    SurfaceRole clickSurface_ = SurfaceRole::All;
    int lastClickScale_ = 1;
    int lastClockSecond_ = -1;

    UiActions pendingActions_{};

    void markDirty(SurfaceRole role);
    void markAllDirty();

    UiRect menuBarRect(int viewW) const;
    UiRect menuTitleRect(int menuIdx) const;
    UiRect menuDropdownRect() const;
    UiRect spotlightRect() const;
    UiRect spotlightResultsRect() const;

    // Slim Control Center
    UiRect ccOrigin() const;
    UiRect ccWifiRect() const;
    UiRect ccBtCircleRect() const;
    UiRect ccAirCircleRect() const;
    UiRect ccMediaRect() const;
    UiRect ccFocusPillRect() const;
    UiRect ccStageCircleRect() const;
    UiRect ccMirrorCircleRect() const;
    UiRect ccSoundRect() const;
    UiRect ccDisplayRect() const;
    UiRect ccQuickCircleRect(int index) const;
    UiRect ccEditBtnRect() const;
    UiRect ccBoundsRect() const;

    // Individually positioned & resizable Desktop Widgets
    UiRect widgetBaseSize(int index) const;
    UiRect widgetRect(int index) const;
    UiRect widgetSizeBtnRect(int index, int tier) const; // tier 0=S,1=M,2=L
    UiRect widgetResizeHandleRect(int index) const;

    UiRect finderRect() const;
    UiRect alertRect() const;
    UiRect tunerRect() const;
    UiRect sliderRectInTuner(int index) const;

    UiRect dockRectFor(int viewW, int viewH) const;
    UiRect dockIconRectFor(int viewW, int viewH, int index) const;
    UiRect dockTooltipRectFor(int viewW, int viewH) const;

    void setWidgetSizeTier(int index, int tier);
    void updateSliderFromPointer(DragTarget target, float px);
    void ensureSurface(SurfaceRole role);
    void destroySurfaces();
    void redrawSurface(SurfaceRole role);

    void drawMenuBar(cairo_t* cr, int viewW) const;
    void drawMenuDropdown(cairo_t* cr) const;
    void drawSpotlight(cairo_t* cr) const;
    void drawControlCenter(cairo_t* cr) const;
    void drawWidgets(cairo_t* cr) const;
    void drawSingleWidget(cairo_t* cr, int idx) const;
    void drawFinderWindow(cairo_t* cr) const;
    void drawAlertDialog(cairo_t* cr) const;
    void drawTuner(cairo_t* cr) const;
    void drawDock(cairo_t* cr, int viewW, int viewH) const;

    struct SpotlightResultItem {
        std::string title;
        std::string subtitle;
        double r = 0.24, g = 0.56, b = 0.96;
        int actionId = -1; // >=0 builtin, -1 desktop app
        int appIdx = -1;
    };
    std::vector<SpotlightResultItem> buildSpotlightResults() const;
    void executeSpotlightItem(const SpotlightResultItem& item);
    std::vector<int> filteredFinderApps() const;
};

} // namespace lg
