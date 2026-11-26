#include "preview_dump.hpp"
#include "glass_renderer.hpp"
#include "ui_widgets.hpp"

#include <epoxy/egl.h>
#include <epoxy/gl.h>
#include <cairo.h>

#include <algorithm>
#include <vector>

namespace lg {

bool renderPreviewPng(const std::string& outPath, int width, int height, std::string& outError) {
    EGLDisplay dpy = EGL_NO_DISPLAY;
    if (epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_base")) {
        dpy = eglGetPlatformDisplayEXT(0x31DD /* EGL_PLATFORM_SURFACELESS_MESA */, EGL_DEFAULT_DISPLAY, nullptr);
    }
    if (dpy == EGL_NO_DISPLAY) {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (dpy == EGL_NO_DISPLAY) {
        outError = "eglGetDisplay failed";
        return false;
    }
    if (!eglInitialize(dpy, nullptr, nullptr)) {
        outError = "eglInitialize failed";
        return false;
    }

    const EGLint cfgAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg = nullptr;
    EGLint numCfg = 0;
    if (!eglChooseConfig(dpy, cfgAttribs, &cfg, 1, &numCfg) || numCfg == 0) {
        outError = "eglChooseConfig failed";
        eglTerminate(dpy);
        return false;
    }

    eglBindAPI(EGL_OPENGL_API);
    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttribs);
    if (ctx == EGL_NO_CONTEXT) {
        outError = "eglCreateContext failed";
        eglTerminate(dpy);
        return false;
    }

    const EGLint pbufAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    EGLSurface surfEgl = eglCreatePbufferSurface(dpy, cfg, pbufAttribs);
    if (!eglMakeCurrent(dpy, surfEgl, surfEgl, ctx)) {
        outError = "eglMakeCurrent failed";
        if (surfEgl != EGL_NO_SURFACE) eglDestroySurface(dpy, surfEgl);
        eglDestroyContext(dpy, ctx);
        eglTerminate(dpy);
        return false;
    }

    GLuint fbo = 0, tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    cairo_status_t st = CAIRO_STATUS_WRITE_ERROR;
    {
        GlassRenderer renderer;
        if (!renderer.init(outError)) {
            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &tex);
            eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surfEgl != EGL_NO_SURFACE) eglDestroySurface(dpy, surfEgl);
            eglDestroyContext(dpy, ctx);
            eglTerminate(dpy);
            return false;
        }

        LiquidGlassUi ui;
        ui.setViewport(width, height);
        ui.setBackdropModeLabel(1, false);

        int uiW = 0, uiH = 0;
        const uint8_t* uiPixels = nullptr;
        if (ui.renderUiIfDirty(uiW, uiH, uiPixels)) {
            renderer.uploadUiOverlay(uiW, uiH, uiPixels);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        auto panels = ui.collectGlassPanels();
        renderer.render(width, height, 1.8f, false, 0, false, 0.0f, 0.0f, 0.0f, false, ui.material(), panels);

        std::vector<uint8_t> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        // Convert bottom-up RGBA to top-down premultiplied BGRA for Cairo PNG export
        cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        uint8_t* dst = cairo_image_surface_get_data(surf);
        int stride = cairo_image_surface_get_stride(surf);
        for (int y = 0; y < height; ++y) {
            const uint8_t* srcRow = &rgba[static_cast<size_t>(height - 1 - y) * static_cast<size_t>(width) * 4];
            uint8_t* dstRow = dst + y * stride;
            for (int x = 0; x < width; ++x) {
                dstRow[x * 4 + 0] = srcRow[x * 4 + 2]; // B
                dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
                dstRow[x * 4 + 2] = srcRow[x * 4 + 0]; // R
                dstRow[x * 4 + 3] = srcRow[x * 4 + 3]; // A
            }
        }
        cairo_surface_mark_dirty(surf);
        st = cairo_surface_write_to_png(surf, outPath.c_str());
        cairo_surface_destroy(surf);
        renderer.shutdown();
    }

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surfEgl != EGL_NO_SURFACE) eglDestroySurface(dpy, surfEgl);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);

    if (st != CAIRO_STATUS_SUCCESS) {
        outError = cairo_status_to_string(st);
        return false;
    }
    return true;
}

} // namespace lg
