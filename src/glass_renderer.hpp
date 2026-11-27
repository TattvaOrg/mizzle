#pragma once

#include "desktop_capture.hpp"
#include "shader_utils.hpp"

#include <epoxy/gl.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace lg {

struct GlassMaterial {
    float refraction = 1.0f;
    float dispersion = 1.0f;
    float frost = 0.28f;
    float specular = 1.0f;
    float tintR = 0.94f;
    float tintG = 0.97f;
    float tintB = 1.0f;
    float tintAlpha = 0.14f;
    float shadowAlpha = 0.34f;
    float blurScale = 1.15f;
};

struct GlassPanelSpec {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float radius = 20.0f;
    bool isPill = false;
    bool overrideMaterial = false;
    GlassMaterial customMaterial{};
    float customOpacity = -1.0f; // <0 uses auto translucent glass opacity when translucentGlass=true
};

class GlassRenderer {
public:
    GlassRenderer() = default;
    ~GlassRenderer();

    GlassRenderer(const GlassRenderer&) = delete;
    GlassRenderer& operator=(const GlassRenderer&) = delete;

    bool init(std::string& outError);
    void shutdown();

    void setBackdropFrame(const CapturedFrame& frame);
    bool hasBackdropFrame() const { return hasDesktopTex_; }

    void uploadUiOverlay(int width, int height, const uint8_t* bgraPixels);

    void render(int width,
                int height,
                float timeSec,
                bool useLiveBackdrop,
                int proceduralBgStyle,
                bool overlayTransparentMode,
                float clickX,
                float clickY,
                float clickAgeSec,
                bool clickActive,
                const GlassMaterial& material,
                const std::vector<GlassPanelSpec>& panels,
                bool translucentGlass = false);

private:
    static constexpr int kPyramidLevels = 5;

    struct RenderTarget {
        GLuint fbo = 0;
        GLuint tex = 0;
        int w = 0;
        int h = 0;
    };

    GLuint quadVao_ = 0;
    GLuint quadVbo_ = 0;

    GLuint desktopTex_ = 0;
    int desktopW_ = 0;
    int desktopH_ = 0;
    bool hasDesktopTex_ = false;

    GLuint uiTex_ = 0;
    int uiW_ = 0;
    int uiH_ = 0;

    RenderTarget backdropTarget_{};
    std::array<RenderTarget, kPyramidLevels> downTargets_{};
    std::array<RenderTarget, kPyramidLevels> upTargets_{};

    ShaderProgram progBlit_;
    ShaderProgram progProcedural_;
    ShaderProgram progDown_;
    ShaderProgram progUp_;
    ShaderProgram progGlass_;
    ShaderProgram progUi_;

    void ensureTargets(int width, int height);
    void destroyTarget(RenderTarget& rt);
    void allocTarget(RenderTarget& rt, int w, int h);
    void drawQuad() const;
};

} // namespace lg
