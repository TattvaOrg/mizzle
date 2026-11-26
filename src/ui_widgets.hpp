#pragma once

#include "glass_renderer.hpp"

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

struct UiActions {
    bool requestExit = false;
    bool requestRecapture = false;
    bool requestToggleBackdropMode = false;
};

class LiquidGlassUi {
public:
    LiquidGlassUi();
    ~LiquidGlassUi();

    LiquidGlassUi(const LiquidGlassUi&) = delete;
    LiquidGlassUi& operator=(const LiquidGlassUi&) = delete;

    void setViewport(int width, int height, int scaleFactor = 1);
    void applyPreset(GlassPreset preset);
    GlassPreset currentPreset() const { return preset_; }
    DockIconStyle dockIconStyle() const { return dockStyle_; }

    const GlassMaterial& material() const { return material_; }
    GlassMaterial& materialMutable() { return material_; }

    void setBackdropModeLabel(int modeIndex, bool hasCapture); // 0=Live, 1=Lake Tahoe, 2=Optics Lab

    float clickPixelX() const { return clickX_ * static_cast<float>(scale_); }
    float clickPixelY() const { return clickY_ * static_cast<float>(scale_); }
    float clickAgeSec() const { return clickAgeSec_; }
    bool clickActive() const { return clickActive_; }
    bool isDragging() const { return dragTarget_ != DragTarget::None; }

    bool tick(float dtSec, float timeSec);

    void onPointerMove(float x, float y);
    void onPointerLeave();
    void onPointerDown(float x, float y);
    void onPointerUp(float x, float y);
    bool onKeyPressed(uint32_t keyval, uint32_t unicodeChar, bool ctrl);

    std::vector<GlassPanelSpec> collectGlassPanels() const;
    bool renderUiIfDirty(int& outWidth, int& outHeight, const uint8_t*& outBgra);
    UiActions consumeActions();

private:
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        bool contains(float px, float py) const {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }
    };

    enum class DragTarget {
        None,
        Spotlight,
        ControlCenter,
        Widgets,
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

    int width_ = 1360;
    int height_ = 820;
    int scale_ = 1;
    bool layoutInitialized_ = false;
    bool dirty_ = true;

    GlassPreset preset_ = GlassPreset::Clear;
    DockIconStyle dockStyle_ = DockIconStyle::Default;
    GlassMaterial material_{};
    int backdropMode_ = 0; // 0=Live Desktop, 1=Lake Tahoe, 2=Optics Lab
    bool hasDesktopCapture_ = false;

    bool showWidgets_ = true;
    bool showControlCenter_ = true;
    bool showSpotlight_ = true;
    bool showFinder_ = false;
    bool showAlert_ = false;
    bool showTuner_ = false;

    bool spotlightFocused_ = false;
    std::string spotlightQuery_;

    // Control Center states
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

    float spotDx_ = 0.0f, spotDy_ = 0.0f;
    float ccDx_ = 0.0f, ccDy_ = 0.0f;
    float widDx_ = 0.0f, widDy_ = 0.0f;
    float findDx_ = 0.0f, findDy_ = 0.0f;
    float tunerDx_ = 0.0f, tunerDy_ = 0.0f;
    float alertDx_ = 0.0f, alertDy_ = 0.0f;

    float pointerX_ = -1000.0f;
    float pointerY_ = -1000.0f;
    bool pointerInside_ = false;
    DragTarget dragTarget_ = DragTarget::None;
    float dragStartX_ = 0.0f;
    float dragStartY_ = 0.0f;
    float dragBaseDx_ = 0.0f;
    float dragBaseDy_ = 0.0f;

    static constexpr int kDockItemCount = 12; // 11 apps + Trash
    int pressedDockIdx_ = -1;
    int bouncingDockIdx_ = -1;
    float dockBounceAge_ = 0.0f;
    float clickX_ = 0.0f;
    float clickY_ = 0.0f;
    float clickAgeSec_ = 0.0f;
    bool clickActive_ = false;
    int lastClockSecond_ = -1;

    UiActions pendingActions_{};

    cairo_surface_t* surface_ = nullptr;
    int surfW_ = 0;
    int surfH_ = 0;

    Rect menuBarRect() const;
    Rect spotlightRect() const;
    Rect spotlightResultsRect() const;

    // Slim Control Center matching macOS Tahoe proportions
    Rect ccOrigin() const;
    Rect ccWifiRect() const;
    Rect ccBtCircleRect() const;
    Rect ccAirCircleRect() const;
    Rect ccMediaRect() const;
    Rect ccFocusPillRect() const;
    Rect ccStageCircleRect() const;
    Rect ccMirrorCircleRect() const;
    Rect ccSoundRect() const;
    Rect ccDisplayRect() const;
    Rect ccQuickCircleRect(int index) const; // 0..7 in 2 rows of 4
    Rect ccEditBtnRect() const;
    Rect ccBoundsRect() const;

    // Slim Top-Left Widgets
    Rect widOrigin() const;
    Rect widCalendarRect() const;
    Rect widPhotoRect() const;
    Rect widWorldRect() const;
    Rect widBoundsRect() const;

    Rect finderRect() const;
    Rect alertRect() const;
    Rect tunerRect() const;
    Rect sliderRectInTuner(int index) const;
    Rect dockRect() const;
    Rect dockIconRect(int index) const;

    void updateSliderFromPointer(DragTarget target, float px);
    void ensureSurface();
    void destroySurface();
    void redrawCairo();

    void drawMenuBar(cairo_t* cr) const;
    void drawSpotlight(cairo_t* cr) const;
    void drawControlCenter(cairo_t* cr) const;
    void drawWidgets(cairo_t* cr) const;
    void drawFinderWindow(cairo_t* cr) const;
    void drawAlertDialog(cairo_t* cr) const;
    void drawTuner(cairo_t* cr) const;
    void drawDock(cairo_t* cr) const;
};

} // namespace lg
