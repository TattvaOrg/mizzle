#include "ui_widgets.hpp"

#include <pango/pangocairo.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace lg {

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Rgba {
    double r, g, b, a;
};

void pathRoundedRect(cairo_t* cr, float x, float y, float w, float h, float r) {
    float rad = std::min(r, std::min(w, h) * 0.5f);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - rad, y + rad, rad, -kPi * 0.5, 0.0);
    cairo_arc(cr, x + w - rad, y + h - rad, rad, 0.0, kPi * 0.5);
    cairo_arc(cr, x + rad, y + h - rad, rad, kPi * 0.5, kPi);
    cairo_arc(cr, x + rad, y + rad, rad, kPi, kPi * 1.5);
    cairo_close_path(cr);
}

void drawText(cairo_t* cr,
              const std::string& text,
              float x,
              float y,
              float maxWidth,
              const char* fontDescStr,
              double r,
              double g,
              double b,
              double a,
              PangoAlignment align = PANGO_ALIGN_LEFT,
              bool centerVert = false,
              float boxHeight = 0.0f) {
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* desc = pango_font_description_from_string(fontDescStr);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_text(layout, text.c_str(), -1);
    if (maxWidth > 0.0f) {
        pango_layout_set_width(layout, static_cast<int>(maxWidth * PANGO_SCALE));
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    }
    pango_layout_set_alignment(layout, align);

    int tw = 0, th = 0;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double drawY = y;
    if (centerVert && boxHeight > 0.0f) {
        drawY = y + (boxHeight - static_cast<double>(th)) * 0.5;
    }

    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_move_to(cr, x, drawY);
    pango_cairo_show_layout(cr, layout);
    cairo_new_path(cr);
    cairo_restore(cr);

    g_object_unref(layout);
}

Rgba inkPrimary(bool darkInk) {
    return darkInk ? Rgba{0.08, 0.10, 0.14, 0.94} : Rgba{0.98, 0.99, 1.00, 0.96};
}

Rgba inkSecondary(bool darkInk) {
    return darkInk ? Rgba{0.20, 0.24, 0.30, 0.72} : Rgba{0.84, 0.89, 0.96, 0.72};
}

void drawWifiGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, size * 0.13);
    for (int i = 0; i < 3; ++i) {
        float rad = size * (0.25f + i * 0.21f);
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx, cy + size * 0.28f, rad, -kPi * 0.78, -kPi * 0.22);
        cairo_stroke(cr);
    }
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx, cy + size * 0.30f, size * 0.085f, 0.0, 2.0 * kPi);
    cairo_fill(cr);
    cairo_restore(cr);
}

void drawBluetoothGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_width(cr, size * 0.12);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    float s = size * 0.40f;
    cairo_move_to(cr, cx - s * 0.52f, cy - s * 0.42f);
    cairo_line_to(cr, cx + s * 0.52f, cy + s * 0.42f);
    cairo_line_to(cr, cx, cy + s * 0.92f);
    cairo_line_to(cr, cx, cy - s * 0.92f);
    cairo_line_to(cr, cx + s * 0.52f, cy - s * 0.42f);
    cairo_line_to(cr, cx - s * 0.52f, cy + s * 0.42f);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void drawAirDropGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_width(cr, size * 0.11);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (int i = 0; i < 3; ++i) {
        float r = size * (0.16f + i * 0.15f);
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx, cy + size * 0.06f, r, -kPi * 1.18, kPi * 0.18);
        cairo_stroke(cr);
    }
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx, cy + size * 0.10f, size * 0.065f, 0.0, 2.0 * kPi);
    cairo_fill(cr);
    cairo_restore(cr);
}

void drawSearchGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_width(cr, size * 0.13);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    float r = size * 0.30f;
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx - size * 0.08f, cy - size * 0.08f, r, 0.0, 2.0 * kPi);
    cairo_stroke(cr);
    cairo_move_to(cr, cx + r * 0.52f, cy + r * 0.52f);
    cairo_line_to(cr, cx + size * 0.40f, cy + size * 0.40f);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void drawSunGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_arc(cr, cx, cy, size * 0.22f, 0.0, 2.0 * kPi);
    cairo_fill(cr);
    cairo_set_line_width(cr, size * 0.11);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (int i = 0; i < 8; ++i) {
        double ang = i * (kPi / 4.0);
        cairo_move_to(cr, cx + std::cos(ang) * size * 0.33, cy + std::sin(ang) * size * 0.33);
        cairo_line_to(cr, cx + std::cos(ang) * size * 0.46, cy + std::sin(ang) * size * 0.46);
    }
    cairo_stroke(cr);
    cairo_restore(cr);
}

void drawSpeakerGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col, bool waves) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    float s = size * 0.42f;
    cairo_move_to(cr, cx - s * 0.85f, cy - s * 0.34f);
    cairo_line_to(cr, cx - s * 0.36f, cy - s * 0.34f);
    cairo_line_to(cr, cx + s * 0.14f, cy - s * 0.78f);
    cairo_line_to(cr, cx + s * 0.14f, cy + s * 0.78f);
    cairo_line_to(cr, cx - s * 0.36f, cy + s * 0.34f);
    cairo_line_to(cr, cx - s * 0.85f, cy + s * 0.34f);
    cairo_close_path(cr);
    cairo_fill(cr);
    if (waves) {
        cairo_set_line_width(cr, size * 0.10);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx + s * 0.22f, cy, s * 0.44f, -0.7, 0.7);
        cairo_stroke(cr);
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx + s * 0.22f, cy, s * 0.78f, -0.75, 0.75);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

void drawMoonGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_arc(cr, cx, cy, size * 0.36f, 0.35, 2.0 * kPi - 0.75);
    cairo_arc_negative(cr, cx + size * 0.18f, cy - size * 0.12f, size * 0.26f, 2.0 * kPi - 0.45, 0.75);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_restore(cr);
}

void drawControlCenterGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_width(cr, 1.15);
    float w = size * 0.82f;
    float h = size * 0.36f;
    pathRoundedRect(cr, cx - w * 0.5f, cy - h - 1.5f, w, h, h * 0.5f);
    cairo_stroke(cr);
    cairo_arc(cr, cx + w * 0.22f, cy - h * 0.5f - 1.5f, h * 0.34f, 0.0, 2.0 * kPi);
    cairo_fill(cr);

    pathRoundedRect(cr, cx - w * 0.5f, cy + 1.5f, w, h, h * 0.5f);
    cairo_stroke(cr);
    cairo_arc(cr, cx - w * 0.22f, cy + h * 0.5f + 1.5f, h * 0.34f, 0.0, 2.0 * kPi);
    cairo_fill(cr);
    cairo_restore(cr);
}

// Translucent glass clock face matching liquid-glass-macos.jpg World Clocks
void drawTranslucentClock(cairo_t* cr, float cx, float cy, float radius, int hour, int minute) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, radius, 0.0, 2.0 * kPi);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.32);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    for (int i = 0; i < 12; ++i) {
        double a = (i * 30.0 - 90.0) * kPi / 180.0;
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%d", i == 0 ? 12 : i);
        float tx = cx + static_cast<float>(std::cos(a)) * radius * 0.72f - 6.0f;
        float ty = cy + static_cast<float>(std::sin(a)) * radius * 0.72f - 4.0f;
        drawText(cr, buf, tx, ty, 12.0f, "Sans 4.5", 0.96, 0.98, 1.0, 0.80, PANGO_ALIGN_CENTER);
    }

    double hAng = (((hour % 12) + minute / 60.0) * 30.0 - 90.0) * kPi / 180.0;
    double mAng = (minute * 6.0 - 90.0) * kPi / 180.0;

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr, 0.98, 0.99, 1.0, 0.95);
    cairo_set_line_width(cr, 1.6);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, cx + std::cos(hAng) * radius * 0.44, cy + std::sin(hAng) * radius * 0.44);
    cairo_stroke(cr);

    cairo_set_line_width(cr, 1.15);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, cx + std::cos(mAng) * radius * 0.68, cy + std::sin(mAng) * radius * 0.68);
    cairo_stroke(cr);

    cairo_arc(cr, cx, cy, 1.8, 0.0, 2.0 * kPi);
    cairo_set_source_rgba(cr, 1.0, 0.62, 0.18, 0.98);
    cairo_fill(cr);
    cairo_restore(cr);
}

void drawGearIcon(cairo_t* cr, float cx, float cy, float rad, Rgba col, int teeth = 12) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_set_line_width(cr, rad * 0.20);
    cairo_arc(cr, cx, cy, rad * 0.72, 0.0, 2.0 * kPi);
    cairo_stroke(cr);
    cairo_set_line_width(cr, rad * 0.11);
    cairo_arc(cr, cx, cy, rad * 0.42, 0.0, 2.0 * kPi);
    cairo_stroke(cr);
    cairo_arc(cr, cx, cy, rad * 0.15, 0.0, 2.0 * kPi);
    cairo_fill(cr);
    for (int i = 0; i < teeth; ++i) {
        double a = i * (2.0 * kPi / teeth);
        cairo_set_line_width(cr, rad * 0.16);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_move_to(cr, cx + std::cos(a) * rad * 0.74, cy + std::sin(a) * rad * 0.74);
        cairo_line_to(cr, cx + std::cos(a) * rad * 0.98, cy + std::sin(a) * rad * 0.98);
        cairo_stroke(cr);
    }
    cairo_set_line_width(cr, rad * 0.10);
    for (int i = 0; i < 3; ++i) {
        double a = i * (2.0 * kPi / 3.0) - kPi * 0.5;
        cairo_move_to(cr, cx + std::cos(a) * rad * 0.14, cy + std::sin(a) * rad * 0.14);
        cairo_line_to(cr, cx + std::cos(a) * rad * 0.44, cy + std::sin(a) * rad * 0.44);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

void drawVectorAppIcon(cairo_t* cr, int idx, float x, float y, float size, DockIconStyle style) {
    const bool monochromeGlass = (style == DockIconStyle::ClearGlass || style == DockIconStyle::TintedGlass);
    const bool darkTile = (style == DockIconStyle::Dark);

    cairo_save(cr);
    cairo_new_path(cr);
    pathRoundedRect(cr, x, y, size, size, size * 0.23f);

    if (monochromeGlass) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.18);
        cairo_set_line_width(cr, 0.85);
        cairo_stroke(cr);
    } else {
        cairo_pattern_t* pat = cairo_pattern_create_linear(x, y, x, y + size);
        if (darkTile) {
            cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.18, 0.20, 0.25, 0.96);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.09, 0.10, 0.14, 0.96);
        } else {
            switch (idx) {
                case 0: // Finder
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.36, 0.78, 1.00, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.08, 0.44, 0.96, 0.98);
                    break;
                case 1: // Launchpad
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.97, 0.98, 1.00, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.85, 0.88, 0.93, 0.98);
                    break;
                case 2: // Browser
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.32, 0.76, 0.99, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.12, 0.42, 0.92, 0.98);
                    break;
                case 3: // Messages
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.42, 0.92, 0.48, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.12, 0.72, 0.26, 0.98);
                    break;
                case 4: // Mail
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.34, 0.72, 0.99, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.14, 0.46, 0.94, 0.98);
                    break;
                case 5: // Photos
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.00, 1.00, 1.00, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.91, 0.93, 0.96, 0.98);
                    break;
                case 6: // Calendar
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.99, 0.99, 1.00, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.90, 0.92, 0.95, 0.98);
                    break;
                case 7: // Notes
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.99, 0.99, 0.98, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.92, 0.93, 0.95, 0.98);
                    break;
                case 8: // Music
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.99, 0.36, 0.46, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.88, 0.10, 0.26, 0.98);
                    break;
                case 9: // Terminal
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.18, 0.20, 0.26, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.07, 0.08, 0.11, 0.98);
                    break;
                case 10: // Settings
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.64, 0.68, 0.74, 0.98);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.38, 0.42, 0.48, 0.98);
                    break;
                default: // Trash
                    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.76, 0.80, 0.86, 0.88);
                    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.44, 0.48, 0.55, 0.88);
                    break;
            }
        }
        cairo_set_source(cr, pat);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.26);
        cairo_set_line_width(cr, 0.85);
        cairo_stroke(cr);
    }

    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    Rgba white{0.97, 0.98, 1.0, 0.95};
    Rgba glassInk{0.94, 0.96, 1.0, 0.88};

    if (idx == 0) {
        pathRoundedRect(cr, x + size * 0.08f, y + size * 0.08f, size * 0.84f, size * 0.84f, size * 0.16f);
        cairo_clip(cr);
        cairo_set_source_rgba(cr, monochromeGlass ? 0.95 : 0.90, monochromeGlass ? 0.97 : 0.95, 1.0, monochromeGlass ? 0.42 : 0.88);
        cairo_move_to(cr, cx + size * 0.03f, y);
        cairo_line_to(cr, x + size, y);
        cairo_line_to(cr, x + size, y + size);
        cairo_line_to(cr, cx - size * 0.03f, y + size);
        cairo_line_to(cr, cx - size * 0.06f, cy + size * 0.08f);
        cairo_line_to(cr, cx + size * 0.05f, cy + size * 0.08f);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_reset_clip(cr);

        Rgba feat = monochromeGlass ? glassInk : Rgba{0.08, 0.24, 0.52, 0.88};
        cairo_set_source_rgba(cr, feat.r, feat.g, feat.b, feat.a);
        cairo_set_line_width(cr, size * 0.055);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, cx - size * 0.17f, cy - size * 0.16f);
        cairo_line_to(cr, cx - size * 0.17f, cy - size * 0.04f);
        cairo_move_to(cr, cx + size * 0.17f, cy - size * 0.16f);
        cairo_line_to(cr, cx + size * 0.17f, cy - size * 0.04f);
        cairo_stroke(cr);
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx, cy + size * 0.02f, size * 0.25f, 0.35, kPi - 0.35);
        cairo_stroke(cr);
    } else if (idx == 1) {
        const double cols[9][3] = {
            {0.96, 0.32, 0.42}, {0.98, 0.62, 0.18}, {0.96, 0.82, 0.20},
            {0.28, 0.80, 0.46}, {0.22, 0.68, 0.98}, {0.42, 0.46, 0.98},
            {0.72, 0.36, 0.96}, {0.96, 0.42, 0.72}, {0.46, 0.56, 0.68},
        };
        float cell = size * 0.16f;
        float gap = size * 0.06f;
        float start = -1.5f * cell - gap;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                int k = r * 3 + c;
                pathRoundedRect(cr, cx + start + c * (cell + gap), cy + start + r * (cell + gap), cell, cell, cell * 0.32f);
                if (monochromeGlass) cairo_set_source_rgba(cr, 0.95, 0.97, 1.0, 0.68);
                else cairo_set_source_rgba(cr, cols[k][0], cols[k][1], cols[k][2], 0.96);
                cairo_fill(cr);
            }
        }
    } else if (idx == 2) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, monochromeGlass ? 0.24 : 0.92);
        cairo_arc(cr, cx, cy, size * 0.34f, 0.0, 2.0 * kPi);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, monochromeGlass ? 0.98 : 0.94, monochromeGlass ? 0.99 : 0.24, monochromeGlass ? 1.0 : 0.28, 0.95);
        cairo_move_to(cr, cx + size * 0.20f, cy - size * 0.20f);
        cairo_line_to(cr, cx + size * 0.05f, cy + size * 0.05f);
        cairo_line_to(cr, cx - size * 0.05f, cy - size * 0.05f);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, monochromeGlass ? 0.82 : 0.20, monochromeGlass ? 0.86 : 0.54, monochromeGlass ? 0.95 : 0.96, 0.90);
        cairo_move_to(cr, cx - size * 0.20f, cy + size * 0.20f);
        cairo_line_to(cr, cx + size * 0.05f, cy + size * 0.05f);
        cairo_line_to(cr, cx - size * 0.05f, cy - size * 0.05f);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (idx == 3) {
        // Speech bubble
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_save(cr);
        cairo_translate(cr, cx, cy - size * 0.03f);
        cairo_scale(cr, 1.2, 0.95);
        cairo_arc(cr, 0.0, 0.0, size * 0.26f, 0.0, 2.0 * kPi);
        cairo_fill(cr);
        cairo_restore(cr);
        cairo_move_to(cr, cx - size * 0.08f, cy + size * 0.16f);
        cairo_line_to(cr, cx - size * 0.24f, cy + size * 0.30f);
        cairo_line_to(cr, cx - size * 0.01f, cy + size * 0.22f);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (idx == 4) {
        // Mail envelope
        pathRoundedRect(cr, x + size * 0.20f, y + size * 0.29f, size * 0.60f, size * 0.42f, size * 0.07f);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.20, 0.56, 0.96, 0.88);
        cairo_set_line_width(cr, 1.3);
        cairo_move_to(cr, x + size * 0.22f, y + size * 0.33f);
        cairo_line_to(cr, cx, cy + size * 0.04f);
        cairo_line_to(cr, x + size * 0.78f, y + size * 0.33f);
        cairo_stroke(cr);
    } else if (idx == 5) {
        const double petals[8][3] = {
            {0.98, 0.34, 0.24}, {0.98, 0.64, 0.18}, {0.96, 0.84, 0.18}, {0.36, 0.82, 0.36},
            {0.20, 0.74, 0.88}, {0.26, 0.48, 0.96}, {0.64, 0.34, 0.94}, {0.94, 0.32, 0.66},
        };
        for (int p = 0; p < 8; ++p) {
            double a = p * (kPi / 4.0);
            cairo_save(cr);
            cairo_translate(cr, cx + std::cos(a) * size * 0.17, cy + std::sin(a) * size * 0.17);
            cairo_rotate(cr, a);
            cairo_scale(cr, 1.5, 0.88);
            cairo_arc(cr, 0.0, 0.0, size * 0.105, 0.0, 2.0 * kPi);
            if (monochromeGlass) cairo_set_source_rgba(cr, 0.95, 0.97, 1.0, 0.56);
            else cairo_set_source_rgba(cr, petals[p][0], petals[p][1], petals[p][2], 0.84);
            cairo_fill(cr);
            cairo_restore(cr);
        }
    } else if (idx == 6) {
        pathRoundedRect(cr, x + 1.0f, y + 1.0f, size - 2.0f, size * 0.27f, size * 0.20f);
        if (monochromeGlass) cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.36);
        else cairo_set_source_rgba(cr, 0.92, 0.22, 0.24, 0.96);
        cairo_fill(cr);
        Rgba numCol = (darkTile || monochromeGlass) ? white : Rgba{0.10, 0.12, 0.16, 0.92};
        drawText(cr, "26", x, y + size * 0.26f, size, "Sans Bold 13", numCol.r, numCol.g, numCol.b, numCol.a, PANGO_ALIGN_CENTER);
    } else if (idx == 7) {
        pathRoundedRect(cr, x + 1.0f, y + 1.0f, size - 2.0f, size * 0.26f, size * 0.20f);
        if (monochromeGlass) cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.40);
        else cairo_set_source_rgba(cr, 0.98, 0.76, 0.14, 0.98);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, monochromeGlass ? 0.95 : 0.55, monochromeGlass ? 0.97 : 0.58, monochromeGlass ? 1.0 : 0.65, 0.45);
        cairo_set_line_width(cr, 1.0);
        for (int l = 0; l < 3; ++l) {
            float ly = y + size * (0.45f + l * 0.17f);
            cairo_move_to(cr, x + size * 0.16f, ly);
            cairo_line_to(cr, x + size * 0.84f, ly);
        }
        cairo_stroke(cr);
    } else if (idx == 8) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_set_line_width(cr, size * 0.08);
        cairo_arc(cr, cx - size * 0.13f, cy + size * 0.18f, size * 0.095f, 0.0, 2.0 * kPi);
        cairo_arc(cr, cx + size * 0.14f, cy + size * 0.13f, size * 0.095f, 0.0, 2.0 * kPi);
        cairo_fill(cr);
        cairo_move_to(cr, cx - size * 0.05f, cy + size * 0.18f);
        cairo_line_to(cr, cx - size * 0.05f, cy - size * 0.18f);
        cairo_line_to(cr, cx + size * 0.22f, cy - size * 0.24f);
        cairo_line_to(cr, cx + size * 0.22f, cy + size * 0.13f);
        cairo_stroke(cr);
    } else if (idx == 9) {
        drawText(cr, ">_", x + size * 0.14f, y + size * 0.14f, size * 0.76f, "Sans Bold 11", white.r, white.g, white.b, 0.96);
    } else if (idx == 10) {
        drawGearIcon(cr, cx, cy, size * 0.34f, white, 12);
    } else {
        cairo_set_source_rgba(cr, 0.92, 0.95, 1.0, monochromeGlass ? 0.55 : 0.86);
        cairo_set_line_width(cr, 1.2);
        float bw = size * 0.44f;
        float bh = size * 0.52f;
        cairo_move_to(cr, cx - bw * 0.55f, cy - bh * 0.42f);
        cairo_line_to(cr, cx + bw * 0.55f, cy - bh * 0.42f);
        cairo_line_to(cr, cx + bw * 0.42f, cy + bh * 0.50f);
        cairo_line_to(cr, cx - bw * 0.42f, cy + bh * 0.50f);
        cairo_close_path(cr);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.2, 0.25, 0.32, 0.45);
        cairo_stroke(cr);
        cairo_move_to(cr, cx - bw * 0.62f, cy - bh * 0.46f);
        cairo_line_to(cr, cx + bw * 0.62f, cy - bh * 0.46f);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}

struct SpotlightEntry {
    const char* name;
    const char* category;
    double r, g, b;
};

constexpr SpotlightEntry kSpotlightEntries[] = {
    {"Applications", "Finder Grid Window", 0.20, 0.56, 0.98},
    {"Terminal", "Developer Tool", 0.16, 0.20, 0.28},
    {"Shader Lab", "Liquid Glass Tuner", 0.52, 0.32, 0.94},
    {"Control Center", "System Controls", 0.26, 0.66, 0.96},
    {"Desktop Widgets", "Calendar & World Clocks", 0.96, 0.66, 0.20},
    {"System Alert", "Glass Dialog Demo", 0.24, 0.52, 0.92},
};

const char* kTracks[] = {
    "Not Playing",
    "Alpine Horizon",
    "Quartz Ripple",
};

std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

} // namespace

LiquidGlassUi::LiquidGlassUi() {
    quickCircleState_[0] = true;
    applyPreset(GlassPreset::Clear);
}

LiquidGlassUi::~LiquidGlassUi() {
    destroySurface();
}

void LiquidGlassUi::setViewport(int width, int height, int scaleFactor) {
    int w = std::max(width, 640);
    int h = std::max(height, 480);
    int sc = std::clamp(scaleFactor, 1, 4);
    if (w != width_ || h != height_ || sc != scale_ || !layoutInitialized_) {
        width_ = w;
        height_ = h;
        scale_ = sc;
        layoutInitialized_ = true;
        dirty_ = true;
    }
}

void LiquidGlassUi::applyPreset(GlassPreset preset) {
    preset_ = preset;
    switch (preset) {
        case GlassPreset::Clear:
            material_ = GlassMaterial{
                .refraction = 1.10f,
                .dispersion = 1.15f,
                .frost = 0.26f,
                .specular = 0.95f,
                .tintR = 0.92f,
                .tintG = 0.96f,
                .tintB = 1.00f,
                .tintAlpha = 0.08f,
                .shadowAlpha = 0.14f,
                .blurScale = 1.35f,
            };
            darkAppearance_ = false;
            break;
        case GlassPreset::Frosted:
            material_ = GlassMaterial{
                .refraction = 0.90f,
                .dispersion = 0.85f,
                .frost = 0.65f,
                .specular = 0.90f,
                .tintR = 0.95f,
                .tintG = 0.97f,
                .tintB = 1.00f,
                .tintAlpha = 0.20f,
                .shadowAlpha = 0.16f,
                .blurScale = 1.75f,
            };
            darkAppearance_ = true;
            break;
        case GlassPreset::Smoked:
            material_ = GlassMaterial{
                .refraction = 1.05f,
                .dispersion = 1.10f,
                .frost = 0.42f,
                .specular = 0.92f,
                .tintR = 0.09f,
                .tintG = 0.12f,
                .tintB = 0.18f,
                .tintAlpha = 0.30f,
                .shadowAlpha = 0.22f,
                .blurScale = 1.50f,
            };
            darkAppearance_ = false;
            break;
        case GlassPreset::Aqua:
            material_ = GlassMaterial{
                .refraction = 1.25f,
                .dispersion = 1.35f,
                .frost = 0.30f,
                .specular = 1.08f,
                .tintR = 0.20f,
                .tintG = 0.56f,
                .tintB = 0.96f,
                .tintAlpha = 0.14f,
                .shadowAlpha = 0.15f,
                .blurScale = 1.40f,
            };
            darkAppearance_ = false;
            break;
    }
    dirty_ = true;
}

void LiquidGlassUi::setBackdropModeLabel(int modeIndex, bool hasCapture) {
    if (backdropMode_ != modeIndex || hasDesktopCapture_ != hasCapture) {
        backdropMode_ = modeIndex;
        hasDesktopCapture_ = hasCapture;
        dirty_ = true;
    }
}

LiquidGlassUi::Rect LiquidGlassUi::menuBarRect() const {
    return {0.0f, 0.0f, static_cast<float>(width_), 26.0f};
}

LiquidGlassUi::Rect LiquidGlassUi::spotlightRect() const {
    float w = std::min(460.0f, static_cast<float>(width_) - 120.0f);
    float x = (static_cast<float>(width_) - w) * 0.5f + spotDx_;
    float y = 74.0f + spotDy_;
    return {x, y, w, 42.0f};
}

LiquidGlassUi::Rect LiquidGlassUi::spotlightResultsRect() const {
    Rect s = spotlightRect();
    return {s.x, s.y + s.h + 8.0f, s.w, 218.0f};
}

// Slim, jewel-like Control Center matching liquid-glass-macos.jpg (248px wide)
LiquidGlassUi::Rect LiquidGlassUi::ccOrigin() const {
    return {static_cast<float>(width_) - 266.0f + ccDx_, 38.0f + ccDy_, 248.0f, 444.0f};
}
LiquidGlassUi::Rect LiquidGlassUi::ccWifiRect() const { Rect o = ccOrigin(); return {o.x, o.y, 120.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccBtCircleRect() const { Rect o = ccOrigin(); return {o.x, o.y + 52.0f, 56.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccAirCircleRect() const { Rect o = ccOrigin(); return {o.x + 64.0f, o.y + 52.0f, 56.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccMediaRect() const { Rect o = ccOrigin(); return {o.x + 128.0f, o.y, 120.0f, 96.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccFocusPillRect() const { Rect o = ccOrigin(); return {o.x, o.y + 104.0f, 120.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccStageCircleRect() const { Rect o = ccOrigin(); return {o.x + 128.0f, o.y + 104.0f, 56.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccMirrorCircleRect() const { Rect o = ccOrigin(); return {o.x + 192.0f, o.y + 104.0f, 56.0f, 44.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccSoundRect() const { Rect o = ccOrigin(); return {o.x, o.y + 156.0f, 248.0f, 50.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccDisplayRect() const { Rect o = ccOrigin(); return {o.x, o.y + 214.0f, 248.0f, 50.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccQuickCircleRect(int index) const {
    Rect o = ccOrigin();
    int col = index % 4;
    int row = index / 4;
    return {o.x + col * 64.0f, o.y + 272.0f + row * 52.0f, 56.0f, 44.0f};
}
LiquidGlassUi::Rect LiquidGlassUi::ccEditBtnRect() const { Rect o = ccOrigin(); return {o.x + 76.0f, o.y + 380.0f, 96.0f, 24.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::ccBoundsRect() const { Rect o = ccOrigin(); return {o.x, o.y, 248.0f, 408.0f}; }

// Slim Top-Left Widgets matching liquid-glass-macos.jpg
LiquidGlassUi::Rect LiquidGlassUi::widOrigin() const {
    return {18.0f + widDx_, 38.0f + widDy_, 196.0f, 190.0f};
}
LiquidGlassUi::Rect LiquidGlassUi::widCalendarRect() const { Rect o = widOrigin(); return {o.x, o.y, 94.0f, 94.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::widPhotoRect() const { Rect o = widOrigin(); return {o.x + 102.0f, o.y, 94.0f, 94.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::widWorldRect() const { Rect o = widOrigin(); return {o.x, o.y + 104.0f, 196.0f, 80.0f}; }
LiquidGlassUi::Rect LiquidGlassUi::widBoundsRect() const { Rect o = widOrigin(); return {o.x, o.y, 196.0f, 184.0f}; }

LiquidGlassUi::Rect LiquidGlassUi::finderRect() const {
    float w = std::min(620.0f, static_cast<float>(width_) - 120.0f);
    float h = 390.0f;
    float x = (static_cast<float>(width_) - w) * 0.44f + findDx_;
    float y = 140.0f + findDy_;
    return {x, y, w, h};
}

LiquidGlassUi::Rect LiquidGlassUi::alertRect() const {
    float w = 264.0f;
    float h = 216.0f;
    float x = (static_cast<float>(width_) - w) * 0.5f + alertDx_;
    float y = (static_cast<float>(height_) - h) * 0.52f + alertDy_;
    return {x, y, w, h};
}

LiquidGlassUi::Rect LiquidGlassUi::tunerRect() const {
    float w = 286.0f;
    float h = 310.0f;
    float x = 18.0f + tunerDx_;
    float y = static_cast<float>(height_) - h - 86.0f + tunerDy_;
    return {x, y, w, h};
}

LiquidGlassUi::Rect LiquidGlassUi::sliderRectInTuner(int index) const {
    Rect t = tunerRect();
    return {t.x + 16.0f, t.y + 50.0f + index * 36.0f, t.w - 32.0f, 24.0f};
}

LiquidGlassUi::Rect LiquidGlassUi::dockRect() const {
    float baseIcon = 38.0f;
    float gap = 9.0f;
    float padX = 12.0f;
    float totalW = padX * 2.0f + kDockItemCount * baseIcon + (kDockItemCount - 1) * gap + 12.0f;
    float h = 54.0f;
    float x = (static_cast<float>(width_) - totalW) * 0.5f;
    float y = static_cast<float>(height_) - h - 10.0f;
    return {x, y, totalW, h};
}

LiquidGlassUi::Rect LiquidGlassUi::dockIconRect(int index) const {
    Rect dk = dockRect();
    float scale = (pressedDockIdx_ == index) ? 0.90f : 1.0f;
    float size = 38.0f * scale;
    float cx = dk.x + 12.0f + 19.0f + index * 47.0f + (index == kDockItemCount - 1 ? 12.0f : 0.0f);
    float bounceLift = 0.0f;
    if (bouncingDockIdx_ == index && dockBounceAge_ < 0.55f) {
        float t = dockBounceAge_ / 0.55f;
        bounceLift = std::abs(std::sin(t * 2.0f * static_cast<float>(kPi))) * std::exp(-t * 3.0f) * 14.0f;
    }
    float cy = dk.y + dk.h * 0.5f - 1.5f - bounceLift;
    return {cx - size * 0.5f, cy - size * 0.5f, size, size};
}

bool LiquidGlassUi::tick(float dtSec, float timeSec) {
    bool changed = false;

    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);
    if (tmNow.tm_sec != lastClockSecond_) {
        lastClockSecond_ = tmNow.tm_sec;
        dirty_ = true;
        changed = true;
    }

    if (clickActive_) {
        clickAgeSec_ += dtSec;
        if (clickAgeSec_ >= 0.35f) {
            clickActive_ = false;
        }
        changed = true;
    }

    if (bouncingDockIdx_ >= 0) {
        dockBounceAge_ += dtSec;
        if (dockBounceAge_ >= 0.55f) {
            bouncingDockIdx_ = -1;
            dockBounceAge_ = 0.0f;
        }
        dirty_ = true;
        changed = true;
    }

    return changed;
}

void LiquidGlassUi::updateSliderFromPointer(DragTarget target, float px) {
    auto norm = [px](const Rect& r, float padLeft, float padRight) {
        return std::clamp((px - (r.x + padLeft)) / std::max(r.w - padLeft - padRight, 1.0f), 0.0f, 1.0f);
    };

    switch (target) {
        case DragTarget::VolumeSlider:
            volume_ = norm(ccSoundRect(), 26.0f, 52.0f);
            break;
        case DragTarget::BrightnessSlider:
            brightness_ = norm(ccDisplayRect(), 26.0f, 26.0f);
            break;
        case DragTarget::TunerRefraction:
            material_.refraction = norm(sliderRectInTuner(0), 6.0f, 6.0f) * 2.0f;
            break;
        case DragTarget::TunerDispersion:
            material_.dispersion = norm(sliderRectInTuner(1), 6.0f, 6.0f) * 2.5f;
            break;
        case DragTarget::TunerFrost:
            material_.frost = norm(sliderRectInTuner(2), 6.0f, 6.0f);
            break;
        case DragTarget::TunerSpecular:
            material_.specular = norm(sliderRectInTuner(3), 6.0f, 6.0f) * 2.0f;
            break;
        case DragTarget::TunerTint:
            material_.tintAlpha = norm(sliderRectInTuner(4), 6.0f, 6.0f) * 0.45f;
            break;
        default:
            return;
    }
    dirty_ = true;
}

void LiquidGlassUi::onPointerMove(float x, float y) {
    pointerX_ = x;
    pointerY_ = y;
    pointerInside_ = true;

    if (dragTarget_ == DragTarget::None) return;

    float dx = x - dragStartX_;
    float dy = y - dragStartY_;
    switch (dragTarget_) {
        case DragTarget::Spotlight: spotDx_ = dragBaseDx_ + dx; spotDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        case DragTarget::ControlCenter: ccDx_ = dragBaseDx_ + dx; ccDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        case DragTarget::Widgets: widDx_ = dragBaseDx_ + dx; widDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        case DragTarget::FinderWindow: findDx_ = dragBaseDx_ + dx; findDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        case DragTarget::Tuner: tunerDx_ = dragBaseDx_ + dx; tunerDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        case DragTarget::AlertDialog: alertDx_ = dragBaseDx_ + dx; alertDy_ = dragBaseDy_ + dy; dirty_ = true; break;
        default: updateSliderFromPointer(dragTarget_, x); break;
    }
}

void LiquidGlassUi::onPointerLeave() {
    pointerInside_ = false;
}

void LiquidGlassUi::onPointerDown(float x, float y) {
    pointerX_ = x;
    pointerY_ = y;
    pointerInside_ = true;
    clickX_ = x;
    clickY_ = y;
    clickAgeSec_ = 0.0f;
    clickActive_ = true;

    Rect mb = menuBarRect();
    if (mb.contains(x, y)) {
        float rx = mb.w - 14.0f;
        if (Rect{rx - 106.0f, 0.0f, 106.0f, 26.0f}.contains(x, y)) { showWidgets_ = !showWidgets_; dirty_ = true; return; }
        if (Rect{rx - 134.0f, 0.0f, 26.0f, 26.0f}.contains(x, y)) { showControlCenter_ = !showControlCenter_; dirty_ = true; return; }
        if (Rect{rx - 160.0f, 0.0f, 24.0f, 26.0f}.contains(x, y)) { showSpotlight_ = !showSpotlight_; spotlightFocused_ = showSpotlight_; dirty_ = true; return; }
        if (Rect{rx - 252.0f, 3.0f, 82.0f, 20.0f}.contains(x, y)) { pendingActions_.requestToggleBackdropMode = true; return; }
        return;
    }

    if (showAlert_ && alertRect().contains(x, y)) {
        Rect al = alertRect();
        if (Rect{al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, 30.0f}.contains(x, y) ||
            Rect{al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, 30.0f}.contains(x, y)) {
            showAlert_ = false;
            dirty_ = true;
            return;
        }
        dragTarget_ = DragTarget::AlertDialog;
        dragStartX_ = x; dragStartY_ = y;
        dragBaseDx_ = alertDx_; dragBaseDy_ = alertDy_;
        return;
    }

    if (showControlCenter_) {
        if (ccWifiRect().contains(x, y)) { wifiOn_ = !wifiOn_; dirty_ = true; return; }
        if (ccBtCircleRect().contains(x, y)) { btOn_ = !btOn_; dirty_ = true; return; }
        if (ccAirCircleRect().contains(x, y)) { airdropOn_ = !airdropOn_; dirty_ = true; return; }
        if (ccFocusPillRect().contains(x, y)) { focusOn_ = !focusOn_; dirty_ = true; return; }
        if (ccStageCircleRect().contains(x, y)) { showFinder_ = !showFinder_; dirty_ = true; return; }
        if (ccMirrorCircleRect().contains(x, y)) { dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4); dirty_ = true; return; }
        if (ccEditBtnRect().contains(x, y)) { showTuner_ = !showTuner_; dirty_ = true; return; }

        for (int i = 0; i < 8; ++i) {
            if (ccQuickCircleRect(i).contains(x, y)) {
                quickCircleState_[i] = !quickCircleState_[i];
                if (i == 0) {
                    darkAppearance_ = !darkAppearance_;
                    applyPreset(darkAppearance_ ? GlassPreset::Frosted : GlassPreset::Clear);
                } else if (i == 3) {
                    showAlert_ = !showAlert_;
                } else if (i == 7) {
                    showTuner_ = !showTuner_;
                }
                dirty_ = true;
                return;
            }
        }

        Rect mp = ccMediaRect();
        if (mp.contains(x, y)) {
            if (Rect{mp.x + 8.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) { trackIndex_ = (trackIndex_ + 2) % 3; dirty_ = true; return; }
            if (Rect{mp.x + 44.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) { musicPlaying_ = !musicPlaying_; if (musicPlaying_ && trackIndex_ == 0) trackIndex_ = 1; dirty_ = true; return; }
            if (Rect{mp.x + 80.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) { trackIndex_ = (trackIndex_ + 1) % 3; dirty_ = true; return; }
        }

        if (ccSoundRect().contains(x, y)) {
            dragTarget_ = DragTarget::VolumeSlider;
            updateSliderFromPointer(dragTarget_, x);
            return;
        }
        if (ccDisplayRect().contains(x, y)) {
            dragTarget_ = DragTarget::BrightnessSlider;
            updateSliderFromPointer(dragTarget_, x);
            return;
        }
        if (ccBoundsRect().contains(x, y)) {
            dragTarget_ = DragTarget::ControlCenter;
            dragStartX_ = x; dragStartY_ = y;
            dragBaseDx_ = ccDx_; dragBaseDy_ = ccDy_;
            return;
        }
    }

    if (showSpotlight_ && spotlightRect().contains(x, y)) {
        spotlightFocused_ = true;
        dragTarget_ = DragTarget::Spotlight;
        dragStartX_ = x; dragStartY_ = y;
        dragBaseDx_ = spotDx_; dragBaseDy_ = spotDy_;
        dirty_ = true;
        return;
    } else if (spotlightFocused_) {
        spotlightFocused_ = false;
        dirty_ = true;
    }

    if (showTuner_ && tunerRect().contains(x, y)) {
        Rect tn = tunerRect();
        const DragTarget targets[] = {
            DragTarget::TunerRefraction,
            DragTarget::TunerDispersion,
            DragTarget::TunerFrost,
            DragTarget::TunerSpecular,
            DragTarget::TunerTint,
        };
        for (int i = 0; i < 5; ++i) {
            if (sliderRectInTuner(i).contains(x, y)) {
                dragTarget_ = targets[i];
                updateSliderFromPointer(dragTarget_, x);
                return;
            }
        }
        if (Rect{tn.x + 16.0f, tn.y + tn.h - 68.0f, 122.0f, 22.0f}.contains(x, y)) { applyPreset(static_cast<GlassPreset>((static_cast<int>(preset_) + 1) % 4)); return; }
        if (Rect{tn.x + 148.0f, tn.y + tn.h - 68.0f, 122.0f, 22.0f}.contains(x, y)) { dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4); dirty_ = true; return; }
        if (Rect{tn.x + 16.0f, tn.y + tn.h - 36.0f, 122.0f, 22.0f}.contains(x, y)) { pendingActions_.requestRecapture = true; return; }
        if (Rect{tn.x + 148.0f, tn.y + tn.h - 36.0f, 122.0f, 22.0f}.contains(x, y)) { pendingActions_.requestToggleBackdropMode = true; return; }

        dragTarget_ = DragTarget::Tuner;
        dragStartX_ = x; dragStartY_ = y;
        dragBaseDx_ = tunerDx_; dragBaseDy_ = tunerDy_;
        return;
    }

    if (showFinder_ && finderRect().contains(x, y)) {
        Rect f = finderRect();
        if (Rect{f.x + 14.0f, f.y + 14.0f, 16.0f, 16.0f}.contains(x, y)) { showFinder_ = false; dirty_ = true; return; }
        dragTarget_ = DragTarget::FinderWindow;
        dragStartX_ = x; dragStartY_ = y;
        dragBaseDx_ = findDx_; dragBaseDy_ = findDy_;
        return;
    }

    if (showWidgets_ && widBoundsRect().contains(x, y)) {
        dragTarget_ = DragTarget::Widgets;
        dragStartX_ = x; dragStartY_ = y;
        dragBaseDx_ = widDx_; dragBaseDy_ = widDy_;
        return;
    }

    if (dockRect().contains(x, y)) {
        for (int i = 0; i < kDockItemCount; ++i) {
            if (dockIconRect(i).contains(x, y)) {
                pressedDockIdx_ = i;
                bouncingDockIdx_ = i;
                dockBounceAge_ = 0.0f;
                if (i == 0) showFinder_ = !showFinder_;
                else if (i == 1) showWidgets_ = !showWidgets_;
                else if (i == 2) pendingActions_.requestToggleBackdropMode = true;
                else if (i == 3) showAlert_ = !showAlert_;
                else if (i == 8) { musicPlaying_ = !musicPlaying_; if (musicPlaying_ && trackIndex_ == 0) trackIndex_ = 1; }
                else if (i == 10) showTuner_ = !showTuner_;
                else if (i == 11) dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4);
                dirty_ = true;
                break;
            }
        }
    }
}

void LiquidGlassUi::onPointerUp(float x, float y) {
    dragTarget_ = DragTarget::None;
    if (pressedDockIdx_ >= 0) {
        pressedDockIdx_ = -1;
        dirty_ = true;
    }
}

bool LiquidGlassUi::onKeyPressed(uint32_t keyval, uint32_t unicodeChar, bool ctrl) {
    if (keyval == 0xff1b) {
        if (spotlightFocused_ && !spotlightQuery_.empty()) {
            spotlightQuery_.clear();
            dirty_ = true;
            return true;
        }
        pendingActions_.requestExit = true;
        return true;
    }

    if (spotlightFocused_) {
        if (keyval == 0xff08) {
            if (!spotlightQuery_.empty()) {
                spotlightQuery_.pop_back();
                dirty_ = true;
            }
            return true;
        }
        if (keyval == 0xff0d) {
            spotlightFocused_ = false;
            dirty_ = true;
            return true;
        }
        if (!ctrl && unicodeChar >= 32 && unicodeChar < 127 && spotlightQuery_.size() < 40) {
            spotlightQuery_.push_back(static_cast<char>(unicodeChar));
            dirty_ = true;
            return true;
        }
    }

    if (unicodeChar == '1') { applyPreset(GlassPreset::Clear); return true; }
    if (unicodeChar == '2') { applyPreset(GlassPreset::Frosted); return true; }
    if (unicodeChar == '3') { applyPreset(GlassPreset::Smoked); return true; }
    if (unicodeChar == '4') { applyPreset(GlassPreset::Aqua); return true; }
    if (unicodeChar == 'i' || unicodeChar == 'I') { dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4); dirty_ = true; return true; }
    if (unicodeChar == 'r' || unicodeChar == 'R') { pendingActions_.requestRecapture = true; return true; }
    if (unicodeChar == 'b' || unicodeChar == 'B') { pendingActions_.requestToggleBackdropMode = true; return true; }
    if (unicodeChar == 't' || unicodeChar == 'T') { showTuner_ = !showTuner_; dirty_ = true; return true; }
    if (unicodeChar == 'c' || unicodeChar == 'C') { showControlCenter_ = !showControlCenter_; dirty_ = true; return true; }
    if (unicodeChar == 'w' || unicodeChar == 'W') { showWidgets_ = !showWidgets_; dirty_ = true; return true; }
    if (unicodeChar == 'f' || unicodeChar == 'F') { showFinder_ = !showFinder_; dirty_ = true; return true; }
    if (unicodeChar == 'a' || unicodeChar == 'A') { showAlert_ = !showAlert_; dirty_ = true; return true; }
    if (unicodeChar == 's' || unicodeChar == 'S') { showSpotlight_ = !showSpotlight_; spotlightFocused_ = showSpotlight_; dirty_ = true; return true; }
    if (unicodeChar == 'q' || unicodeChar == 'Q') { pendingActions_.requestExit = true; return true; }
    return false;
}

UiActions LiquidGlassUi::consumeActions() {
    UiActions out = pendingActions_;
    pendingActions_ = {};
    return out;
}

std::vector<GlassPanelSpec> LiquidGlassUi::collectGlassPanels() const {
    std::vector<GlassPanelSpec> panels;

    // 1. Ultra-clean Menu Bar
    Rect mb = menuBarRect();
    GlassMaterial barMat = material_;
    barMat.frost = 0.25f;
    barMat.tintR = 0.20f;
    barMat.tintG = 0.28f;
    barMat.tintB = 0.44f;
    barMat.tintAlpha = 0.04f;
    barMat.shadowAlpha = 0.0f;
    barMat.refraction = 0.15f;
    barMat.specular = 0.10f;
    panels.push_back({mb.x, mb.y, mb.w, mb.h, 0.0f, false, true, barMat});

    // 2. Slim Top-Left Widgets
    if (showWidgets_) {
        for (const Rect& r : {widCalendarRect(), widPhotoRect(), widWorldRect()}) {
            panels.push_back({r.x, r.y, r.w, r.h, 20.0f, false, false, {}});
        }
    }

    // 3. Light Quartz Frosted Applications/Finder Window (matching liquid-glass-macos.jpg)
    if (showFinder_) {
        Rect f = finderRect();
        GlassMaterial winBody = material_;
        winBody.frost = 0.85f;
        winBody.tintR = 0.96f;
        winBody.tintG = 0.97f;
        winBody.tintB = 0.99f;
        winBody.tintAlpha = 0.78f;
        winBody.shadowAlpha = 0.36f;
        winBody.refraction = 0.35f;
        panels.push_back({f.x, f.y, f.w, f.h, 20.0f, false, true, winBody});
    }

    // 4. Slim Spotlight Pill
    if (showSpotlight_) {
        Rect sp = spotlightRect();
        panels.push_back({sp.x, sp.y, sp.w, sp.h, 21.0f, true, false, {}});
        if (spotlightFocused_ || !spotlightQuery_.empty()) {
            Rect sr = spotlightResultsRect();
            panels.push_back({sr.x, sr.y, sr.w, sr.h, 18.0f, false, false, {}});
        }
    }

    // 5. Slim Control Center Pills & Circular Droplet Buttons
    if (showControlCenter_) {
        const Rect pills[] = {
            ccWifiRect(), ccBtCircleRect(), ccAirCircleRect(), ccMediaRect(),
            ccFocusPillRect(), ccStageCircleRect(), ccMirrorCircleRect(),
            ccSoundRect(), ccDisplayRect(), ccEditBtnRect(),
        };
        for (const Rect& r : pills) {
            panels.push_back({r.x, r.y, r.w, r.h, std::min(22.0f, r.h * 0.5f), true, false, {}});
        }
        for (int i = 0; i < 8; ++i) {
            Rect qr = ccQuickCircleRect(i);
            panels.push_back({qr.x, qr.y, qr.w, qr.h, qr.h * 0.5f, true, false, {}});
        }
    }

    if (showTuner_) {
        Rect tn = tunerRect();
        panels.push_back({tn.x, tn.y, tn.w, tn.h, 20.0f, false, false, {}});
    }

    if (showAlert_) {
        Rect al = alertRect();
        GlassMaterial alMat = material_;
        alMat.frost = 0.82f;
        alMat.tintR = 0.16f;
        alMat.tintG = 0.20f;
        alMat.tintB = 0.28f;
        alMat.tintAlpha = 0.56f;
        alMat.shadowAlpha = 0.42f;
        panels.push_back({al.x, al.y, al.w, al.h, 24.0f, false, true, alMat});
    }

    // 6. Slim Bottom Dock
    Rect dk = dockRect();
    panels.push_back({dk.x, dk.y, dk.w, dk.h, 19.0f, true, false, {}});

    if (dockStyle_ == DockIconStyle::ClearGlass || dockStyle_ == DockIconStyle::TintedGlass) {
        GlassMaterial iconMat = material_;
        iconMat.refraction = 1.25f;
        iconMat.dispersion = 1.25f;
        iconMat.frost = 0.20f;
        iconMat.specular = 1.05f;
        iconMat.shadowAlpha = 0.14f;
        if (dockStyle_ == DockIconStyle::TintedGlass) {
            iconMat.tintR = 0.18f;
            iconMat.tintG = 0.44f;
            iconMat.tintB = 0.90f;
            iconMat.tintAlpha = 0.20f;
        }
        for (int i = 0; i < kDockItemCount; ++i) {
            Rect ir = dockIconRect(i);
            panels.push_back({ir.x, ir.y, ir.w, ir.h, ir.w * 0.23f, true, true, iconMat});
        }
    }

    return panels;
}

void LiquidGlassUi::destroySurface() {
    if (surface_) {
        cairo_surface_destroy(surface_);
        surface_ = nullptr;
    }
    surfW_ = 0;
    surfH_ = 0;
}

void LiquidGlassUi::ensureSurface() {
    int targetW = width_ * scale_;
    int targetH = height_ * scale_;
    if (surface_ && surfW_ == targetW && surfH_ == targetH) return;
    destroySurface();
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, targetW, targetH);
    surfW_ = targetW;
    surfH_ = targetH;
    dirty_ = true;
}

bool LiquidGlassUi::renderUiIfDirty(int& outWidth, int& outHeight, const uint8_t*& outBgra) {
    ensureSurface();
    if (!dirty_) return false;
    redrawCairo();
    cairo_surface_flush(surface_);
    outWidth = surfW_;
    outHeight = surfH_;
    outBgra = cairo_image_surface_get_data(surface_);
    dirty_ = false;
    return true;
}

void LiquidGlassUi::redrawCairo() {
    cairo_t* cr = cairo_create(surface_);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_scale(cr, static_cast<double>(scale_), static_cast<double>(scale_));

    drawMenuBar(cr);
    if (showWidgets_) drawWidgets(cr);
    if (showFinder_) drawFinderWindow(cr);
    if (showSpotlight_) drawSpotlight(cr);
    if (showControlCenter_) drawControlCenter(cr);
    if (showTuner_) drawTuner(cr);
    if (showAlert_) drawAlertDialog(cr);
    drawDock(cr);

    cairo_destroy(cr);
}

void LiquidGlassUi::drawMenuBar(cairo_t* cr) const {
    Rect mb = menuBarRect();
    Rgba ink{0.98, 0.99, 1.0, 0.94};
    Rgba sub{0.92, 0.95, 1.0, 0.84};

    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, ink.a);
    float lx = 18.0f;
    float ly = 13.0f;
    cairo_move_to(cr, lx, ly - 6.0f);
    cairo_line_to(cr, lx + 6.0f, ly + 4.5f);
    cairo_line_to(cr, lx - 6.0f, ly + 4.5f);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_restore(cr);

    drawText(cr, "Finder", 34.0f, 0.0f, 64.0f, "Sans Bold 9", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, mb.h);
    drawText(cr, "File   Edit   View   Go   Window   Help", 84.0f, 0.0f, 300.0f, "Sans 9", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_LEFT, true, mb.h);

    float rx = mb.w - 14.0f;
    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);
    char clockBuf[32];
    std::strftime(clockBuf, sizeof(clockBuf), "%a %d %b  %H:%M", &tmNow);

    drawText(cr, clockBuf, rx - 108.0f, 0.0f, 108.0f, "Sans 9", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_RIGHT, true, mb.h);
    drawControlCenterGlyph(cr, rx - 122.0f, 13.0f, 13.0f, ink);
    drawSearchGlyph(cr, rx - 148.0f, 13.0f, 13.0f, ink);

    const char* modeStr = (backdropMode_ == 0 && hasDesktopCapture_)
        ? "Live BG"
        : (backdropMode_ == 2 ? "Optics Lab" : "Lake Tahoe");
    pathRoundedRect(cr, rx - 252.0f, 3.0f, 82.0f, 20.0f, 10.0f);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.14);
    cairo_fill(cr);
    drawText(cr, modeStr, rx - 252.0f, 3.0f, 82.0f, "Sans Bold 7.5", 1.0, 1.0, 1.0, 0.94, PANGO_ALIGN_CENTER, true, 20.0f);
}

void LiquidGlassUi::drawSpotlight(cairo_t* cr) const {
    Rect sp = spotlightRect();
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    drawSearchGlyph(cr, sp.x + 22.0f, sp.y + sp.h * 0.5f, 16.0f, sub);
    std::string display = spotlightQuery_.empty()
        ? (spotlightFocused_ ? "|" : "Spotlight Search")
        : (spotlightFocused_ ? (spotlightQuery_ + "|") : spotlightQuery_);
    Rgba txtCol = spotlightQuery_.empty() ? sub : ink;
    drawText(cr, display, sp.x + 42.0f, sp.y, sp.w - 60.0f, "Sans 11", txtCol.r, txtCol.g, txtCol.b, txtCol.a, PANGO_ALIGN_LEFT, true, sp.h);

    if (spotlightFocused_ || !spotlightQuery_.empty()) {
        Rect sr = spotlightResultsRect();
        std::string q = toLower(spotlightQuery_);
        float rowY = sr.y + 10.0f;
        int shown = 0;
        for (const auto& entry : kSpotlightEntries) {
            if (!q.empty() && toLower(entry.name).find(q) == std::string::npos &&
                toLower(entry.category).find(q) == std::string::npos) {
                continue;
            }
            if (shown >= 5) break;
            if (shown == 0) {
                pathRoundedRect(cr, sr.x + 10.0f, rowY, sr.w - 20.0f, 36.0f, 11.0f);
                cairo_set_source_rgba(cr, 0.24, 0.58, 1.0, 0.26);
                cairo_fill(cr);
            }
            pathRoundedRect(cr, sr.x + 16.0f, rowY + 6.0f, 24.0f, 24.0f, 7.0f);
            cairo_set_source_rgba(cr, entry.r, entry.g, entry.b, 0.94);
            cairo_fill(cr);
            drawText(cr, entry.name, sr.x + 48.0f, rowY, 200.0f, "Sans Bold 9.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, 36.0f);
            drawText(cr, entry.category, sr.x + sr.w - 190.0f, rowY, 172.0f, "Sans 8", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_RIGHT, true, 36.0f);
            rowY += 38.0f;
            ++shown;
        }
    }
}

void LiquidGlassUi::drawControlCenter(cairo_t* cr) const {
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    // Wi-Fi slim pill
    Rect wf = ccWifiRect();
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_arc(cr, wf.x + 22.0f, wf.y + wf.h * 0.5f, 14.0f, 0.0, 2.0 * kPi);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, wifiOn_ ? 0.26 : 0.10);
    cairo_fill(cr);
    cairo_restore(cr);
    drawWifiGlyph(cr, wf.x + 22.0f, wf.y + wf.h * 0.5f, 14.0f, wifiOn_ ? Rgba{0.35, 0.72, 1.0, 1.0} : ink);
    drawText(cr, "Wi-Fi", wf.x + 42.0f, wf.y + 8.0f, 70.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a);
    drawText(cr, wifiOn_ ? "Connected" : "Off", wf.x + 42.0f, wf.y + 22.0f, 70.0f, "Sans 7", sub.r, sub.g, sub.b, sub.a);

    // Bluetooth + AirDrop circle droplets
    Rect bt = ccBtCircleRect();
    drawBluetoothGlyph(cr, bt.x + bt.w * 0.5f, bt.y + bt.h * 0.5f, 16.0f, btOn_ ? Rgba{0.38, 0.74, 1.0, 1.0} : ink);
    Rect ad = ccAirCircleRect();
    drawAirDropGlyph(cr, ad.x + ad.w * 0.5f, ad.y + ad.h * 0.5f, 16.0f, airdropOn_ ? Rgba{0.38, 0.74, 1.0, 1.0} : ink);

    // Media card
    Rect mp = ccMediaRect();
    pathRoundedRect(cr, mp.x + 12.0f, mp.y + 10.0f, 34.0f, 34.0f, 8.0f);
    if (musicPlaying_) {
        cairo_pattern_t* pat = cairo_pattern_create_linear(mp.x + 12.0f, mp.y + 10.0f, mp.x + 46.0f, mp.y + 44.0f);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.25, 0.65, 1.0, 0.95);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.68, 0.28, 0.95, 0.95);
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    } else {
        cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.14);
        cairo_fill(cr);
    }
    drawText(cr, kTracks[trackIndex_], mp.x + 12.0f, mp.y + 49.0f, mp.w - 24.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a);

    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.88);
    float ty = mp.y + 78.0f;
    cairo_move_to(cr, mp.x + 30.0f, ty - 5.0f); cairo_line_to(cr, mp.x + 22.0f, ty); cairo_line_to(cr, mp.x + 30.0f, ty + 5.0f); cairo_close_path(cr); cairo_fill(cr);
    if (musicPlaying_) {
        cairo_rectangle(cr, mp.x + 54.0f, ty - 5.5f, 3.5f, 11.0f);
        cairo_rectangle(cr, mp.x + 61.0f, ty - 5.5f, 3.5f, 11.0f);
        cairo_fill(cr);
    } else {
        cairo_move_to(cr, mp.x + 55.0f, ty - 6.0f); cairo_line_to(cr, mp.x + 66.0f, ty); cairo_line_to(cr, mp.x + 55.0f, ty + 6.0f); cairo_close_path(cr); cairo_fill(cr);
    }
    cairo_move_to(cr, mp.x + 88.0f, ty - 5.0f); cairo_line_to(cr, mp.x + 96.0f, ty); cairo_line_to(cr, mp.x + 88.0f, ty + 5.0f); cairo_close_path(cr); cairo_fill(cr);
    cairo_restore(cr);

    // Focus pill + Stage / Mirror buttons
    Rect fp = ccFocusPillRect();
    drawMoonGlyph(cr, fp.x + 22.0f, fp.y + fp.h * 0.5f, 14.0f, focusOn_ ? Rgba{0.62, 0.50, 1.0, 1.0} : ink);
    drawText(cr, "Focus", fp.x + 42.0f, fp.y, 70.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, fp.h);

    auto drawBoxIcon = [&](Rect r, bool active, bool doubleBox) {
        float cx = r.x + r.w * 0.5f;
        float cy = r.y + r.h * 0.5f;
        cairo_save(cr);
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, active ? 0.98 : 0.85);
        cairo_set_line_width(cr, 1.3);
        pathRoundedRect(cr, cx - 8.0f, cy - 5.5f, 11.0f, 8.5f, 2.0f);
        cairo_stroke(cr);
        if (doubleBox) {
            pathRoundedRect(cr, cx - 2.0f, cy - 2.0f, 11.0f, 8.5f, 2.0f);
            cairo_stroke(cr);
        } else {
            cairo_rectangle(cr, cx - 5.0f, cy - 3.0f, 2.5f, 4.0f);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    };
    drawBoxIcon(ccStageCircleRect(), showFinder_, false);
    drawBoxIcon(ccMirrorCircleRect(), dockStyle_ != DockIconStyle::Default, true);

    // Slim Sound & Display Slider Pills
    auto drawCcSlider = [&](Rect r, const char* title, float val, bool isSound) {
        drawText(cr, title, r.x + 14.0f, r.y + 6.0f, 100.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a);
        float trackX = r.x + 30.0f;
        float trackY = r.y + 31.0f;
        float trackW = r.w - (isSound ? 82.0f : 58.0f);

        if (isSound) {
            drawSpeakerGlyph(cr, r.x + 16.0f, trackY + 1.5f, 11.0f, ink, false);
            drawSpeakerGlyph(cr, r.x + r.w - 38.0f, trackY + 1.5f, 11.0f, ink, true);
            cairo_save(cr);
            cairo_new_path(cr);
            cairo_arc(cr, r.x + r.w - 17.0f, trackY + 1.5f, 9.5f, 0.0, 2.0 * kPi);
            cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.14);
            cairo_fill(cr);
            cairo_restore(cr);
            drawAirDropGlyph(cr, r.x + r.w - 17.0f, trackY + 1.5f, 10.0f, ink);
        } else {
            drawSunGlyph(cr, r.x + 16.0f, trackY + 1.5f, 11.0f, sub);
            drawSunGlyph(cr, r.x + r.w - 16.0f, trackY + 1.5f, 13.0f, ink);
        }

        pathRoundedRect(cr, trackX, trackY, trackW, 3.0f, 1.5f);
        cairo_set_source_rgba(cr, 0.04, 0.06, 0.10, 0.45);
        cairo_fill(cr);

        float fillW = std::max(3.0f, trackW * val);
        pathRoundedRect(cr, trackX, trackY, fillW, 3.0f, 1.5f);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.94);
        cairo_fill(cr);

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_arc(cr, trackX + fillW, trackY + 1.5f, 6.5f, 0.0, 2.0 * kPi);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
        cairo_fill(cr);
        cairo_restore(cr);
    };

    drawCcSlider(ccSoundRect(), "Sound", volume_, true);
    drawCcSlider(ccDisplayRect(), "Display", brightness_, false);

    // 8 circular quick-action droplets (2 rows of 4, matching liquid-glass-macos.jpg)
    const char* quickLabels[8] = {"◐", "▦", "◷", "⌑", "⊞", "◫", "◎", "⚙"};
    for (int i = 0; i < 8; ++i) {
        Rect qr = ccQuickCircleRect(i);
        if (i == 0) {
            float cx = qr.x + qr.w * 0.5f;
            float cy = qr.y + qr.h * 0.5f;
            cairo_save(cr);
            cairo_new_path(cr);
            cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.92);
            cairo_arc(cr, cx, cy, 7.5f, 0.0, 2.0 * kPi);
            cairo_set_line_width(cr, 1.4);
            cairo_stroke(cr);
            cairo_arc(cr, cx, cy, 7.0f, -kPi * 0.5, kPi * 0.5);
            cairo_fill(cr);
            cairo_restore(cr);
        } else {
            drawText(cr, quickLabels[i], qr.x, qr.y, qr.w, "Sans 11", ink.r, ink.g, ink.b,
                     quickCircleState_[i] ? 0.98 : 0.78, PANGO_ALIGN_CENTER, true, qr.h);
        }
    }

    Rect eb = ccEditBtnRect();
    drawText(cr, "Edit Controls", eb.x, eb.y, eb.w, "Sans Bold 7.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER, true, eb.h);
}

void LiquidGlassUi::drawWidgets(cairo_t* cr) const {
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    // 1. Slim Calendar Card
    Rect ca = widCalendarRect();
    drawText(cr, "NOVEMBER", ca.x + 10.0f, ca.y + 8.0f, ca.w - 20.0f, "Sans Bold 6", ink.r, ink.g, ink.b, 0.90);
    const char* wd[7] = {"M", "T", "W", "T", "F", "S", "S"};
    float colW = (ca.w - 16.0f) / 7.0f;
    for (int c = 0; c < 7; ++c) {
        drawText(cr, wd[c], ca.x + 8.0f + c * colW, ca.y + 19.0f, colW, "Sans Bold 5.5", sub.r, sub.g, sub.b, 0.72, PANGO_ALIGN_CENTER);
    }
    for (int day = 1; day <= 30; ++day) {
        int idx = day - 1;
        int row = idx / 7;
        int col = idx % 7;
        float dx = ca.x + 8.0f + col * colW;
        float dy = ca.y + 30.0f + row * 12.0f;
        if (day == 26) {
            cairo_save(cr);
            cairo_new_path(cr);
            cairo_arc(cr, dx + colW * 0.5f, dy + 4.8f, 5.2f, 0.0, 2.0 * kPi);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.88);
            cairo_fill(cr);
            cairo_restore(cr);
        }
        char dBuf[4];
        std::snprintf(dBuf, sizeof(dBuf), "%d", day);
        drawText(cr, dBuf, dx, dy, colW, (day == 26) ? "Sans Bold 5.5" : "Sans 5.5",
                 (day == 26) ? 0.12 : ink.r, (day == 26) ? 0.14 : ink.g, (day == 26) ? 0.18 : ink.b, 0.92, PANGO_ALIGN_CENTER);
    }

    // 2. Architectural Photo Card (clean monochrome illustration inside glass card)
    Rect ph = widPhotoRect();
    cairo_save(cr);
    pathRoundedRect(cr, ph.x + 3.0f, ph.y + 3.0f, ph.w - 6.0f, ph.h - 6.0f, 17.0f);
    cairo_clip(cr);
    cairo_pattern_t* skyP = cairo_pattern_create_linear(ph.x, ph.y, ph.x, ph.y + ph.h);
    cairo_pattern_add_color_stop_rgba(skyP, 0.0, 0.64, 0.70, 0.78, 0.48);
    cairo_pattern_add_color_stop_rgba(skyP, 1.0, 0.42, 0.48, 0.56, 0.48);
    cairo_set_source(cr, skyP);
    cairo_paint(cr);
    cairo_pattern_destroy(skyP);

    // Pediment + columns silhouette
    cairo_set_source_rgba(cr, 0.92, 0.95, 0.98, 0.55);
    cairo_move_to(cr, ph.x + 14.0f, ph.y + 36.0f);
    cairo_line_to(cr, ph.x + ph.w * 0.5f, ph.y + 18.0f);
    cairo_line_to(cr, ph.x + ph.w - 14.0f, ph.y + 36.0f);
    cairo_close_path(cr);
    cairo_fill(cr);
    for (int c = 0; c < 4; ++c) {
        cairo_rectangle(cr, ph.x + 21.0f + c * 15.0f, ph.y + 38.0f, 6.0f, 44.0f);
    }
    cairo_fill(cr);
    cairo_restore(cr);

    // 3. Slim Translucent World Clocks Pill
    Rect wc = widWorldRect();
    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);
    const char* cities[] = {"Cupertino", "Tokyo", "Sydney", "Paris"};
    const int hrsOff[] = {-8, 9, 11, 1};
    for (int i = 0; i < 4; ++i) {
        float cx = wc.x + 27.0f + i * 47.0f;
        int hh = (tmNow.tm_hour + hrsOff[i] + 24) % 24;
        drawTranslucentClock(cr, cx, wc.y + 27.0f, 17.0f, hh, tmNow.tm_min);
        drawText(cr, cities[i], cx - 22.0f, wc.y + 49.0f, 44.0f, "Sans Bold 6", ink.r, ink.g, ink.b, 0.92, PANGO_ALIGN_CENTER);
        drawText(cr, "Today", cx - 22.0f, wc.y + 59.0f, 44.0f, "Sans 5.5", sub.r, sub.g, sub.b, 0.80, PANGO_ALIGN_CENTER);
    }
}

void LiquidGlassUi::drawFinderWindow(cairo_t* cr) const {
    Rect f = finderRect();
    Rgba darkInk{0.10, 0.12, 0.16, 0.94};
    Rgba darkSub{0.34, 0.38, 0.45, 0.80};

    // Traffic lights
    const double tl[3][3] = {{1.0, 0.37, 0.34}, {1.0, 0.74, 0.18}, {0.16, 0.78, 0.25}};
    for (int i = 0; i < 3; ++i) {
        cairo_save(cr);
        cairo_new_path(cr);
        cairo_arc(cr, f.x + 20.0f + i * 18.0f, f.y + 20.0f, 5.5f, 0.0, 2.0 * kPi);
        cairo_set_source_rgba(cr, tl[i][0], tl[i][1], tl[i][2], 0.98);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    drawText(cr, "<   >", f.x + 80.0f, f.y + 11.0f, 44.0f, "Sans Bold 9", darkSub.r, darkSub.g, darkSub.b, 0.85);
    drawText(cr, "Applications", f.x + 126.0f, f.y + 11.0f, 160.0f, "Sans Bold 10", darkInk.r, darkInk.g, darkInk.b, 0.95);

    const char* appNames[12] = {
        "Finder", "Launchpad", "Safari", "Messages",
        "Mail", "Photos", "Calendar", "Notes",
        "Music", "Terminal", "Settings", "Trash",
    };
    float startX = f.x + 48.0f;
    float startY = f.y + 60.0f;
    float colStep = (f.w - 96.0f) / 4.0f;
    float rowStep = 98.0f;
    for (int i = 0; i < 12; ++i) {
        int col = i % 4;
        int row = i / 4;
        float ix = startX + col * colStep + (colStep - 52.0f) * 0.5f;
        float iy = startY + row * rowStep;
        drawVectorAppIcon(cr, i, ix, iy, 52.0f, DockIconStyle::Default);
        drawText(cr, appNames[i], ix - 16.0f, iy + 58.0f, 84.0f, "Sans 8", darkInk.r, darkInk.g, darkInk.b, 0.90, PANGO_ALIGN_CENTER);
    }

    drawText(cr, "12 items, Liquid Glass Preview", f.x, f.y + f.h - 24.0f, f.w, "Sans 7.5", darkSub.r, darkSub.g, darkSub.b, 0.82, PANGO_ALIGN_CENTER);
}

void LiquidGlassUi::drawAlertDialog(cairo_t* cr) const {
    Rect al = alertRect();
    Rgba ink{0.98, 0.99, 1.0, 0.96};
    Rgba sub{0.84, 0.89, 0.97, 0.80};

    pathRoundedRect(cr, al.x + al.w * 0.5f - 20.0f, al.y + 18.0f, 40.0f, 40.0f, 11.0f);
    cairo_set_source_rgba(cr, 0.16, 0.48, 0.96, 0.96);
    cairo_fill(cr);
    drawText(cr, ">_", al.x + al.w * 0.5f - 20.0f, al.y + 18.0f, 40.0f, "Sans Bold 11", 1.0, 1.0, 1.0, 0.98, PANGO_ALIGN_CENTER, true, 40.0f);

    drawText(cr, "\"Wallpaper.png\" already exists.\nDo you want to replace it?",
             al.x + 16.0f, al.y + 68.0f, al.w - 32.0f, "Sans Bold 9", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER);
    drawText(cr, "Replacing it will overwrite its current\nLiquid Glass backdrop.",
             al.x + 16.0f, al.y + 104.0f, al.w - 32.0f, "Sans 7.5", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_CENTER);

    pathRoundedRect(cr, al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, 30.0f, 15.0f);
    cairo_set_source_rgba(cr, 0.08, 0.52, 1.0, 0.96);
    cairo_fill(cr);
    drawText(cr, "Replace", al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, "Sans Bold 9", 1.0, 1.0, 1.0, 0.98, PANGO_ALIGN_CENTER, true, 30.0f);

    pathRoundedRect(cr, al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, 30.0f, 15.0f);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.16);
    cairo_fill(cr);
    drawText(cr, "Cancel", al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, "Sans Bold 9", 1.0, 1.0, 1.0, 0.95, PANGO_ALIGN_CENTER, true, 30.0f);
}

void LiquidGlassUi::drawTuner(cairo_t* cr) const {
    Rect tn = tunerRect();
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    drawText(cr, "Liquid Glass Shader Lab", tn.x + 16.0f, tn.y + 14.0f, tn.w - 32.0f, "Sans Bold 9.5", ink.r, ink.g, ink.b, ink.a);
    drawText(cr, "1-4 Presets • I Icons • F Window • B Backdrop", tn.x + 16.0f, tn.y + 30.0f, tn.w - 32.0f, "Sans 6.5", sub.r, sub.g, sub.b, sub.a);

    struct Row { const char* name; float norm; float val; };
    const Row rows[] = {
        {"Refraction", material_.refraction / 2.0f, material_.refraction},
        {"Dispersion", material_.dispersion / 2.5f, material_.dispersion},
        {"Frost / Blur", material_.frost, material_.frost},
        {"Rim Shine", material_.specular / 2.0f, material_.specular},
        {"Tint Opacity", material_.tintAlpha / 0.45f, material_.tintAlpha},
    };
    for (int i = 0; i < 5; ++i) {
        Rect sr = sliderRectInTuner(i);
        char valBuf[16];
        std::snprintf(valBuf, sizeof(valBuf), "%.2f", rows[i].val);
        drawText(cr, rows[i].name, sr.x, sr.y - 3.0f, 150.0f, "Sans 8", ink.r, ink.g, ink.b, ink.a);
        drawText(cr, valBuf, sr.x + sr.w - 40.0f, sr.y - 3.0f, 40.0f, "Sans Bold 7.5", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_RIGHT);

        float ty = sr.y + 14.0f;
        pathRoundedRect(cr, sr.x, ty, sr.w, 4.0f, 2.0f);
        cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.18);
        cairo_fill(cr);

        float n = std::clamp(rows[i].norm, 0.0f, 1.0f);
        pathRoundedRect(cr, sr.x, ty, std::max(4.0f, sr.w * n), 4.0f, 2.0f);
        cairo_set_source_rgba(cr, 0.28, 0.68, 1.0, 0.94);
        cairo_fill(cr);

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_arc(cr, sr.x + n * sr.w, ty + 2.0f, 5.5f, 0.0, 2.0 * kPi);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    const char* presetNames[] = {"Preset: Clear", "Preset: Frost", "Preset: Smoked", "Preset: Aqua"};
    const char* dockNames[] = {"Icons: Default", "Icons: Dark", "Icons: Clear", "Icons: Tinted"};
    auto drawBtn = [&](float x, float y, float w, const char* label) {
        pathRoundedRect(cr, x, y, w, 22.0f, 11.0f);
        cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.14);
        cairo_fill(cr);
        drawText(cr, label, x, y, w, "Sans Bold 7.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER, true, 22.0f);
    };
    drawBtn(tn.x + 16.0f, tn.y + tn.h - 68.0f, 122.0f, presetNames[static_cast<int>(preset_)]);
    drawBtn(tn.x + 148.0f, tn.y + tn.h - 68.0f, 122.0f, dockNames[static_cast<int>(dockStyle_)]);
    drawBtn(tn.x + 16.0f, tn.y + tn.h - 36.0f, 122.0f, "Recapture (R)");
    drawBtn(tn.x + 148.0f, tn.y + tn.h - 36.0f, 122.0f, "Toggle BG (B)");
}

void LiquidGlassUi::drawDock(cairo_t* cr) const {
    Rect dk = dockRect();
    Rgba ink = inkPrimary(darkAppearance_);

    float sepX = dk.x + 12.0f + 11 * 47.0f - 0.5f;
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.26);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, sepX, dk.y + 12.0f);
    cairo_line_to(cr, sepX, dk.y + dk.h - 12.0f);
    cairo_stroke(cr);
    cairo_restore(cr);

    const bool running[kDockItemCount] = {
        showFinder_, showWidgets_, backdropMode_ != 0, showAlert_, false, false,
        false, false, musicPlaying_, true, showTuner_, false,
    };

    for (int i = 0; i < kDockItemCount; ++i) {
        Rect ir = dockIconRect(i);
        drawVectorAppIcon(cr, i, ir.x, ir.y, ir.w, dockStyle_);

        float cx = ir.x + ir.w * 0.5f;
        if (running[i]) {
            cairo_save(cr);
            cairo_new_path(cr);
            cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.88);
            cairo_arc(cr, cx, dk.y + dk.h - 4.5f, 1.8f, 0.0, 2.0 * kPi);
            cairo_fill(cr);
            cairo_restore(cr);
        }
    }
}

} // namespace lg
