#include "glass_renderer.hpp"

#include <algorithm>

namespace lg {

GlassRenderer::~GlassRenderer() {
    shutdown();
}

bool GlassRenderer::init(std::string& outError) {
    const std::string vert = resolveShaderPath("fullscreen.vert");
    if (!progBlit_.loadFromFiles(vert, resolveShaderPath("blit.frag"), outError)) return false;
    if (!progProcedural_.loadFromFiles(vert, resolveShaderPath("procedural_bg.frag"), outError)) return false;
    if (!progDown_.loadFromFiles(vert, resolveShaderPath("kawase_down.frag"), outError)) return false;
    if (!progUp_.loadFromFiles(vert, resolveShaderPath("kawase_up.frag"), outError)) return false;
    if (!progGlass_.loadFromFiles(vert, resolveShaderPath("liquid_glass.frag"), outError)) return false;
    if (!progUi_.loadFromFiles(vert, resolveShaderPath("composite_ui.frag"), outError)) return false;

    const float quadVerts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    glGenVertexArrays(1, &quadVao_);
    glBindVertexArray(quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    glGenTextures(1, &desktopTex_);
    glBindTexture(GL_TEXTURE_2D, desktopTex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &uiTex_);
    glBindTexture(GL_TEXTURE_2D, uiTex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void GlassRenderer::shutdown() {
    destroyTarget(backdropTarget_);
    for (auto& rt : downTargets_) destroyTarget(rt);
    for (auto& rt : upTargets_) destroyTarget(rt);

    if (desktopTex_ != 0) {
        glDeleteTextures(1, &desktopTex_);
        desktopTex_ = 0;
    }
    if (uiTex_ != 0) {
        glDeleteTextures(1, &uiTex_);
        uiTex_ = 0;
    }
    if (quadVbo_ != 0) {
        glDeleteBuffers(1, &quadVbo_);
        quadVbo_ = 0;
    }
    if (quadVao_ != 0) {
        glDeleteVertexArrays(1, &quadVao_);
        quadVao_ = 0;
    }
    hasDesktopTex_ = false;
}

void GlassRenderer::destroyTarget(RenderTarget& rt) {
    if (rt.tex != 0) {
        glDeleteTextures(1, &rt.tex);
        rt.tex = 0;
    }
    if (rt.fbo != 0) {
        glDeleteFramebuffers(1, &rt.fbo);
        rt.fbo = 0;
    }
    rt.w = 0;
    rt.h = 0;
}

void GlassRenderer::allocTarget(RenderTarget& rt, int w, int h) {
    if (rt.w == w && rt.h == h && rt.fbo != 0) return;
    destroyTarget(rt);

    rt.w = std::max(w, 1);
    rt.h = std::max(h, 1);

    glGenTextures(1, &rt.tex);
    glBindTexture(GL_TEXTURE_2D, rt.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt.w, rt.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &rt.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlassRenderer::ensureTargets(int width, int height) {
    allocTarget(backdropTarget_, width, height);
    int w = width;
    int h = height;
    for (int i = 0; i < kPyramidLevels; ++i) {
        w = std::max(w / 2, 1);
        h = std::max(h / 2, 1);
        allocTarget(downTargets_[i], w, h);
    }
    for (int i = 0; i < kPyramidLevels; ++i) {
        int upW = (i == kPyramidLevels - 1) ? width : downTargets_[kPyramidLevels - 2 - i].w;
        int upH = (i == kPyramidLevels - 1) ? height : downTargets_[kPyramidLevels - 2 - i].h;
        allocTarget(upTargets_[i], upW, upH);
    }
}

void GlassRenderer::setBackdropFrame(const CapturedFrame& frame) {
    if (!frame.valid()) return;
    glBindTexture(GL_TEXTURE_2D, desktopTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB8,
                 frame.width,
                 frame.height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 frame.rgb.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    desktopW_ = frame.width;
    desktopH_ = frame.height;
    hasDesktopTex_ = true;
}

void GlassRenderer::uploadUiOverlay(int width, int height, const uint8_t* bgraPixels) {
    if (width <= 0 || height <= 0 || !bgraPixels) return;
    glBindTexture(GL_TEXTURE_2D, uiTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (uiW_ != width || uiH_ != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bgraPixels);
        uiW_ = width;
        uiH_ = height;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bgraPixels);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlassRenderer::drawQuad() const {
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void GlassRenderer::render(int width,
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
                           bool translucentGlass) {
    if (width <= 0 || height <= 0) return;

    // GtkGLArea binds its own target FBO; preserve it so we render into GTK's framebuffer at the end.
    GLint gtkFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &gtkFbo);

    ensureTargets(width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // 1. Render or copy backdrop into backdropTarget_ (bottom-up GL orientation)
    glBindFramebuffer(GL_FRAMEBUFFER, backdropTarget_.fbo);
    glViewport(0, 0, backdropTarget_.w, backdropTarget_.h);
    if (useLiveBackdrop && hasDesktopTex_) {
        progBlit_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, desktopTex_);
        progBlit_.setInt("uTex", 0);
        progBlit_.setInt("uFlipY", 1);
        drawQuad();
    } else if (!overlayTransparentMode) {
        progProcedural_.use();
        progProcedural_.setFloat("uTime", timeSec);
        progProcedural_.setVec2("uResolution", static_cast<float>(width), static_cast<float>(height));
        progProcedural_.setInt("uBgStyle", proceduralBgStyle);
        drawQuad();
    } else {
        // Desktop Environment translucent mode without static capture: subtle cool ambient base for refraction
        glClearColor(0.12f, 0.16f, 0.24f, 0.35f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // 2. Dual-Kawase blur pyramid
    GLuint prevTex = backdropTarget_.tex;
    int prevW = backdropTarget_.w;
    int prevH = backdropTarget_.h;

    progDown_.use();
    progDown_.setInt("uTex", 0);
    for (int i = 0; i < kPyramidLevels; ++i) {
        const auto& dst = downTargets_[i];
        glBindFramebuffer(GL_FRAMEBUFFER, dst.fbo);
        glViewport(0, 0, dst.w, dst.h);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, prevTex);
        progDown_.setVec2("uHalfPixel",
                          (0.5f * material.blurScale) / static_cast<float>(prevW),
                          (0.5f * material.blurScale) / static_cast<float>(prevH));
        drawQuad();
        prevTex = dst.tex;
        prevW = dst.w;
        prevH = dst.h;
    }

    progUp_.use();
    progUp_.setInt("uTex", 0);
    for (int i = 0; i < kPyramidLevels; ++i) {
        const auto& dst = upTargets_[i];
        glBindFramebuffer(GL_FRAMEBUFFER, dst.fbo);
        glViewport(0, 0, dst.w, dst.h);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, prevTex);
        progUp_.setVec2("uHalfPixel",
                        (0.5f * material.blurScale) / static_cast<float>(prevW),
                        (0.5f * material.blurScale) / static_cast<float>(prevH));
        drawQuad();
        prevTex = dst.tex;
        prevW = dst.w;
        prevH = dst.h;
    }

    const GLuint blurredTex = upTargets_[kPyramidLevels - 1].tex;

    // 3. Final composite to GTK's framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(gtkFbo));
    glViewport(0, 0, width, height);

    if (overlayTransparentMode) {
        // Transparent around glass panels so the real live desktop shows cleanly underneath.
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        progBlit_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backdropTarget_.tex);
        progBlit_.setInt("uTex", 0);
        progBlit_.setInt("uFlipY", 0);
        drawQuad();
    }

    // 4. Draw each Liquid Glass panel using SDF + refraction + chromatic aberration + specular rim
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    progGlass_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backdropTarget_.tex);
    progGlass_.setInt("uBackdrop", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurredTex);
    progGlass_.setInt("uBlurred", 1);

    progGlass_.setVec2("uResolution", static_cast<float>(width), static_cast<float>(height));
    progGlass_.setFloat("uTime", timeSec);
    progGlass_.setVec4("uClick", clickX, clickY, clickAgeSec, clickActive ? 1.0f : 0.0f);
    progGlass_.setFloat("uTranslucentGlass", translucentGlass ? 1.0f : 0.0f);

    for (const auto& p : panels) {
        if (p.w <= 1.0f || p.h <= 1.0f) continue;
        const GlassMaterial& m = p.overrideMaterial ? p.customMaterial : material;
        progGlass_.setVec4("uRect", p.x, p.y, p.w, p.h);
        progGlass_.setFloat("uRadius", p.radius);
        progGlass_.setFloat("uRefraction", m.refraction);
        progGlass_.setFloat("uDispersion", m.dispersion);
        progGlass_.setFloat("uFrost", m.frost);
        progGlass_.setFloat("uSpecular", m.specular);
        progGlass_.setVec4("uTintColor", m.tintR, m.tintG, m.tintB, m.tintAlpha);
        progGlass_.setFloat("uShadowAlpha", m.shadowAlpha);
        progGlass_.setFloat("uIsPill", p.isPill ? 1.0f : 0.0f);
        progGlass_.setFloat("uCustomOpacity", p.customOpacity);
        drawQuad();
    }

    // 5. Composite crisp Cairo/Pango UI overlay (premultiplied alpha)
    if (uiW_ > 0 && uiH_ > 0) {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        progUi_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, uiTex_);
        progUi_.setInt("uUiTex", 0);
        drawQuad();
    }

    glDisable(GL_BLEND);
}

} // namespace lg
