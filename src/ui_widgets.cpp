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
              float boxHeight = 0.0f,
              bool dropShadow = false) {
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
    if (dropShadow) {
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, a * 0.42);
        cairo_move_to(cr, x, drawY + 0.9);
        pango_cairo_show_layout(cr, layout);
    }
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
    return darkInk ? Rgba{0.20, 0.24, 0.30, 0.72} : Rgba{0.84, 0.89, 0.96, 0.76};
}

void drawAppleDropGlyph(cairo_t* cr, float cx, float cy, float size, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    // Clean sculpted apple-inspired silhouette + leaf
    float s = size * 0.45f;
    cairo_move_to(cr, cx, cy - s * 0.48f);
    cairo_curve_to(cr, cx + s * 0.52f, cy - s * 0.88f, cx + s * 1.05f, cy - s * 0.30f, cx + s * 0.82f, cy + s * 0.35f);
    cairo_curve_to(cr, cx + s * 0.65f, cy + s * 0.85f, cx + s * 0.26f, cy + s * 0.98f, cx, cy + s * 0.76f);
    cairo_curve_to(cr, cx - s * 0.26f, cy + s * 0.98f, cx - s * 0.65f, cy + s * 0.85f, cx - s * 0.82f, cy + s * 0.35f);
    cairo_curve_to(cr, cx - s * 1.05f, cy - s * 0.30f, cx - s * 0.52f, cy - s * 0.88f, cx, cy - s * 0.48f);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Leaf
    cairo_move_to(cr, cx + s * 0.04f, cy - s * 0.62f);
    cairo_curve_to(cr, cx + s * 0.08f, cy - s * 1.02f, cx + s * 0.50f, cy - s * 1.12f, cx + s * 0.46f, cy - s * 0.74f);
    cairo_curve_to(cr, cx + s * 0.34f, cy - s * 0.58f, cx + s * 0.14f, cy - s * 0.56f, cx + s * 0.04f, cy - s * 0.62f);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_restore(cr);
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

void drawBatteryGlyph(cairo_t* cr, float x, float y, float pct, bool charging, Rgba col) {
    cairo_save(cr);
    cairo_new_path(cr);
    float w = 21.0f;
    float h = 10.5f;
    pathRoundedRect(cr, x, y - h * 0.5f, w, h, 2.8f);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, 0.42);
    cairo_set_line_width(cr, 1.1);
    cairo_stroke(cr);

    // Battery cap
    cairo_rectangle(cr, x + w + 1.0f, y - 2.2f, 1.8f, 4.4f);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, 0.48);
    cairo_fill(cr);

    float fillFrac = std::clamp(pct / 100.0f, 0.08f, 1.0f);
    float innerW = (w - 3.6f) * fillFrac;
    pathRoundedRect(cr, x + 1.8f, y - h * 0.5f + 1.8f, innerW, h - 3.6f, 1.4f);
    if (charging) {
        cairo_set_source_rgba(cr, 0.28, 0.90, 0.46, 0.96);
    } else if (pct <= 20.0f) {
        cairo_set_source_rgba(cr, 0.98, 0.32, 0.28, 0.96);
    } else {
        cairo_set_source_rgba(cr, col.r, col.g, col.b, 0.92);
    }
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

void drawTranslucentClock(cairo_t* cr, float cx, float cy, float radius, int hour, int minute, int second = -1) {
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, radius, 0.0, 2.0 * kPi);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.13);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.34);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    for (int i = 0; i < 12; ++i) {
        double a = (i * 30.0 - 90.0) * kPi / 180.0;
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%d", i == 0 ? 12 : i);
        float tx = cx + static_cast<float>(std::cos(a)) * radius * 0.72f - 6.0f;
        float ty = cy + static_cast<float>(std::sin(a)) * radius * 0.72f - 4.0f;
        drawText(cr, buf, tx, ty, 12.0f, radius > 22.0f ? "Sans 5.5" : "Sans 4.5", 0.96, 0.98, 1.0, 0.82, PANGO_ALIGN_CENTER);
    }

    double hAng = (((hour % 12) + minute / 60.0) * 30.0 - 90.0) * kPi / 180.0;
    double mAng = (minute * 6.0 - 90.0) * kPi / 180.0;

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr, 0.98, 0.99, 1.0, 0.96);
    cairo_set_line_width(cr, 1.65);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, cx + std::cos(hAng) * radius * 0.44, cy + std::sin(hAng) * radius * 0.44);
    cairo_stroke(cr);

    cairo_set_line_width(cr, 1.2);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, cx + std::cos(mAng) * radius * 0.68, cy + std::sin(mAng) * radius * 0.68);
    cairo_stroke(cr);

    if (second >= 0) {
        double sAng = (second * 6.0 - 90.0) * kPi / 180.0;
        cairo_set_source_rgba(cr, 1.0, 0.62, 0.18, 0.96);
        cairo_set_line_width(cr, 0.8);
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, cx + std::cos(sAng) * radius * 0.76, cy + std::sin(sAng) * radius * 0.76);
        cairo_stroke(cr);
    }

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

void drawVectorAppIcon(cairo_t* cr, int idx, float x, float y, float size, DockIconStyle style, int todayDay = 27) {
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
                case 3: // Widgets / Messages
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
        // Widgets / Dashboard grid glyph
        float cw = size * 0.26f;
        pathRoundedRect(cr, cx - cw - 2.0f, cy - cw - 2.0f, cw, cw, cw * 0.28f);
        pathRoundedRect(cr, cx + 2.0f, cy - cw - 2.0f, cw, cw, cw * 0.28f);
        pathRoundedRect(cr, cx - cw - 2.0f, cy + 2.0f, cw * 2.0f + 4.0f, cw, cw * 0.28f);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_fill(cr);
    } else if (idx == 4) {
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
        char dayStr[8];
        std::snprintf(dayStr, sizeof(dayStr), "%d", todayDay);
        drawText(cr, dayStr, x, y + size * 0.26f, size, size > 44.0f ? "Sans Bold 14" : "Sans Bold 12", numCol.r, numCol.g, numCol.b, numCol.a, PANGO_ALIGN_CENTER);
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
        drawText(cr, ">_", x + size * 0.14f, y + size * 0.14f, size * 0.76f, size > 44.0f ? "Sans Bold 12" : "Sans Bold 10", white.r, white.g, white.b, 0.96);
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

const char* kDockLabels[12] = {
    "Finder", "Applications", "Browser", "Widgets",
    "Mail", "Photos", "Calendar", "Notes",
    "Music", "Terminal", "Shader Lab", "Trash",
};

const char* kTracks[] = {
    "Not Playing",
    "Alpine Horizon",
    "Quartz Ripple",
};

const char* kMonthNamesUpper[12] = {
    "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
    "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

const char* kWeekdayNames[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

int daysInMonth(int year, int month0) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int d = kDays[std::clamp(month0, 0, 11)];
    if (month0 == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        d = 29;
    }
    return d;
}

// Returns 0=Monday .. 6=Sunday for the 1st of the given month
int firstWeekdayMonday0(int year, int month0, int mday, int wdaySun0) {
    // Convert current wday (0=Sun..6=Sat) to 0=Mon..6=Sun
    int todayMon0 = (wdaySun0 + 6) % 7;
    int firstMon0 = (todayMon0 - ((mday - 1) % 7) + 7) % 7;
    return firstMon0;
}

std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

struct MenuEntry {
    const char* label;
    const char* shortcut;
    bool separator = false;
};

std::vector<MenuEntry> getMenuEntries(int menuIdx) {
    switch (menuIdx) {
        case 0: // Apple / Mizzle menu
            return {
                {"About Mizzle Desktop", "", false},
                {"", "", true},
                {"Shader Lab & Settings...", "T", false},
                {"Edit Desktop Widgets", "E", false},
                {"Toggle Dock Auto-Hide", "", false},
                {"Recapture Desktop", "R", false},
                {"", "", true},
                {"Lock Screen", "", false},
                {"Restart Mizzle", "", false},
                {"Quit Mizzle", "Q", false},
            };
        case 1: // File
            return {
                {"New Terminal", "", false},
                {"Open File Manager", "", false},
                {"Applications Grid...", "F", false},
                {"Spotlight Search...", "S", false},
            };
        case 2: // Edit
            return {
                {"Edit Desktop Widgets", "E", false},
                {"Reset Widget Layout", "", false},
                {"", "", true},
                {"Cycle Glass Preset", "1-4", false},
                {"Cycle Dock Style", "I", false},
            };
        case 3: // View
            return {
                {"Show / Hide Widgets", "W", false},
                {"Show / Hide Control Center", "C", false},
                {"Cycle Backdrop Mode", "B", false},
                {"Toggle Dock Auto-Hide", "", false},
            };
        case 4: // Window
            return {
                {"Compositor Overview", "", false},
                {"Focus Next Window", "", false},
                {"Open Shader Lab", "T", false},
            };
        default: // Help
            return {
                {"Mizzle Shortcuts & Alert", "A", false},
                {"About Liquid Glass UI", "", false},
            };
    }
}

} // namespace

LiquidGlassUi::LiquidGlassUi() {
    quickCircleState_[0] = true;
    dockIconScale_.fill(1.0f);
    applyPreset(GlassPreset::Clear);
    refreshDesktopApps();
}

LiquidGlassUi::~LiquidGlassUi() {
    destroySurfaces();
}

void LiquidGlassUi::setViewport(int width, int height, int scaleFactor) {
    setSurfaceViewport(SurfaceRole::All, width, height, scaleFactor);
}

void LiquidGlassUi::setSurfaceViewport(SurfaceRole role, int width, int height, int scaleFactor) {
    int idx = static_cast<int>(role);
    int minW = (role == SurfaceRole::Dock) ? 300 : 640;
    int minH = (role == SurfaceRole::MenuBar) ? 24 : ((role == SurfaceRole::Dock) ? 80 : 480);
    int w = std::max(width, minW);
    int h = std::max(height, minH);
    int sc = std::clamp(scaleFactor, 1, 4);

    if (role != SurfaceRole::All) {
        envMode_ = true;
    }

    SurfaceBuffer& buf = surfaces_[idx];
    if (w != buf.viewW || h != buf.viewH || sc != buf.scale || !buf.initialized) {
        buf.viewW = w;
        buf.viewH = h;
        buf.scale = sc;
        buf.initialized = true;
        buf.dirty = true;
    }

    if (role == SurfaceRole::All || role == SurfaceRole::Widgets || role == SurfaceRole::Popups) {
        width_ = w;
        height_ = h;
        scale_ = sc;
    } else if (role == SurfaceRole::MenuBar) {
        width_ = w;
        scale_ = sc;
    }
}

void LiquidGlassUi::markDirty(SurfaceRole role) {
    if (role == SurfaceRole::All) {
        markAllDirty();
    } else {
        surfaces_[static_cast<int>(SurfaceRole::All)].dirty = true;
        surfaces_[static_cast<int>(role)].dirty = true;
    }
}

void LiquidGlassUi::markAllDirty() {
    for (auto& s : surfaces_) s.dirty = true;
}

void LiquidGlassUi::applyConfig(const MizzleConfig& cfg) {
    applyPreset(static_cast<GlassPreset>(std::clamp(cfg.preset, 0, 3)));
    dockStyle_ = static_cast<DockIconStyle>(std::clamp(cfg.dockStyle, 0, 3));
    dockAutoHide_ = cfg.dockAutoHide;
    backdropMode_ = std::clamp(cfg.backdropMode, 0, 2);
    showWidgets_ = cfg.showWidgets;

    widgets_[0] = {std::clamp(cfg.calSizeTier, 0, 2), std::clamp(cfg.calScale, 0.72f, 1.80f), cfg.calDx, cfg.calDy};
    widgets_[1] = {std::clamp(cfg.sysSizeTier, 0, 2), std::clamp(cfg.sysScale, 0.72f, 1.80f), cfg.sysDx, cfg.sysDy};
    widgets_[2] = {std::clamp(cfg.clkSizeTier, 0, 2), std::clamp(cfg.clkScale, 0.72f, 1.80f), cfg.clkDx, cfg.clkDy};
    markAllDirty();
}

MizzleConfig LiquidGlassUi::exportConfig() const {
    MizzleConfig cfg;
    cfg.preset = static_cast<int>(preset_);
    cfg.dockStyle = static_cast<int>(dockStyle_);
    cfg.dockAutoHide = dockAutoHide_;
    cfg.backdropMode = backdropMode_;
    cfg.showWidgets = showWidgets_;
    cfg.calSizeTier = widgets_[0].sizeTier;
    cfg.calScale = widgets_[0].scale;
    cfg.calDx = widgets_[0].dx;
    cfg.calDy = widgets_[0].dy;
    cfg.sysSizeTier = widgets_[1].sizeTier;
    cfg.sysScale = widgets_[1].scale;
    cfg.sysDx = widgets_[1].dx;
    cfg.sysDy = widgets_[1].dy;
    cfg.clkSizeTier = widgets_[2].sizeTier;
    cfg.clkScale = widgets_[2].scale;
    cfg.clkDx = widgets_[2].dx;
    cfg.clkDy = widgets_[2].dy;
    return cfg;
}

void LiquidGlassUi::setTelemetry(const SystemTelemetry& telemetry) {
    bool changed = (static_cast<int>(telemetry.cpuPercent) != static_cast<int>(telemetry_.cpuPercent)) ||
                   (static_cast<int>(telemetry.ramPercent) != static_cast<int>(telemetry_.ramPercent)) ||
                   (telemetry.batteryPercent != telemetry_.batteryPercent) ||
                   (telemetry.batteryCharging != telemetry_.batteryCharging) ||
                   (telemetry.focusedAppName != telemetry_.focusedAppName) ||
                   (telemetry.wifiOn != telemetry_.wifiOn) ||
                   (telemetry.wifiSsid != telemetry_.wifiSsid) ||
                   (telemetry.btOn != telemetry_.btOn) ||
                   (telemetry.musicPlaying != telemetry_.musicPlaying) ||
                   (telemetry.mediaTitle != telemetry_.mediaTitle) ||
                   (telemetry.filesRunning != telemetry_.filesRunning) ||
                   (telemetry.browserRunning != telemetry_.browserRunning) ||
                   (telemetry.terminalRunning != telemetry_.terminalRunning);
    telemetry_ = telemetry;
    if (dragTarget_ != DragTarget::VolumeSlider) volume_ = telemetry.volume;
    if (dragTarget_ != DragTarget::BrightnessSlider && telemetry.hasBacklight) brightness_ = telemetry.brightness;
    wifiOn_ = telemetry.wifiOn;
    btOn_ = telemetry.btOn;
    if (!telemetry.mediaTitle.empty()) musicPlaying_ = telemetry.musicPlaying;
    if (changed) {
        markAllDirty();
    }
}

void LiquidGlassUi::refreshDesktopApps() {
    desktopApps_ = loadDesktopApps();
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
    markAllDirty();
}

void LiquidGlassUi::setBackdropModeLabel(int modeIndex, bool hasCapture) {
    if (backdropMode_ != modeIndex || hasDesktopCapture_ != hasCapture) {
        backdropMode_ = modeIndex;
        hasDesktopCapture_ = hasCapture;
        markDirty(SurfaceRole::MenuBar);
        markDirty(SurfaceRole::Popups);
        markDirty(SurfaceRole::Dock);
    }
}

bool LiquidGlassUi::hasAnyPopupVisible() const {
    return showControlCenter_ || showSpotlight_ || showFinder_ || showAlert_ || showTuner_ || activeMenu_ >= 0;
}

UiRect LiquidGlassUi::menuBarRect(int viewW) const {
    return {0.0f, 0.0f, static_cast<float>(viewW), kMenuBarHeight};
}

UiRect LiquidGlassUi::menuTitleRect(int menuIdx) const {
    // 0: Apple/Mizzle logo, 1: ActiveApp, 2: File, 3: Edit, 4: View, 5: Window, 6: Help
    switch (menuIdx) {
        case 0: return {8.0f, 0.0f, 28.0f, kMenuBarHeight};
        case 1: return {38.0f, 0.0f, 92.0f, kMenuBarHeight};
        case 2: return {134.0f, 0.0f, 38.0f, kMenuBarHeight};
        case 3: return {174.0f, 0.0f, 40.0f, kMenuBarHeight};
        case 4: return {216.0f, 0.0f, 44.0f, kMenuBarHeight};
        case 5: return {262.0f, 0.0f, 58.0f, kMenuBarHeight};
        case 6: return {322.0f, 0.0f, 42.0f, kMenuBarHeight};
        default: return {};
    }
}

UiRect LiquidGlassUi::menuDropdownRect() const {
    if (activeMenu_ < 0) return {};
    int btnIdx = (activeMenu_ == 0) ? 0 : (activeMenu_ + 1);
    UiRect tr = menuTitleRect(btnIdx);
    auto entries = getMenuEntries(activeMenu_);
    float h = 12.0f;
    for (const auto& e : entries) h += e.separator ? 9.0f : 26.0f;
    return {std::max(8.0f, tr.x), kMenuBarHeight + 4.0f, 236.0f, h};
}

UiRect LiquidGlassUi::spotlightRect() const {
    float w = std::min(500.0f, static_cast<float>(width_) - 120.0f);
    float x = (static_cast<float>(width_) - w) * 0.5f + spotDx_;
    float y = 78.0f + spotDy_;
    return {x, y, w, 44.0f};
}

UiRect LiquidGlassUi::spotlightResultsRect() const {
    UiRect s = spotlightRect();
    auto items = buildSpotlightResults();
    int count = std::min<int>(static_cast<int>(items.size()), 6);
    float h = count > 0 ? (18.0f + count * 38.0f) : 54.0f;
    return {s.x, s.y + s.h + 8.0f, s.w, h};
}

UiRect LiquidGlassUi::ccOrigin() const {
    return {static_cast<float>(width_) - 266.0f + ccDx_, 38.0f + ccDy_, 248.0f, 444.0f};
}
UiRect LiquidGlassUi::ccWifiRect() const { UiRect o = ccOrigin(); return {o.x, o.y, 120.0f, 44.0f}; }
UiRect LiquidGlassUi::ccBtCircleRect() const { UiRect o = ccOrigin(); return {o.x, o.y + 52.0f, 56.0f, 44.0f}; }
UiRect LiquidGlassUi::ccAirCircleRect() const { UiRect o = ccOrigin(); return {o.x + 64.0f, o.y + 52.0f, 56.0f, 44.0f}; }
UiRect LiquidGlassUi::ccMediaRect() const { UiRect o = ccOrigin(); return {o.x + 128.0f, o.y, 120.0f, 96.0f}; }
UiRect LiquidGlassUi::ccFocusPillRect() const { UiRect o = ccOrigin(); return {o.x, o.y + 104.0f, 120.0f, 44.0f}; }
UiRect LiquidGlassUi::ccStageCircleRect() const { UiRect o = ccOrigin(); return {o.x + 128.0f, o.y + 104.0f, 56.0f, 44.0f}; }
UiRect LiquidGlassUi::ccMirrorCircleRect() const { UiRect o = ccOrigin(); return {o.x + 192.0f, o.y + 104.0f, 56.0f, 44.0f}; }
UiRect LiquidGlassUi::ccSoundRect() const { UiRect o = ccOrigin(); return {o.x, o.y + 156.0f, 248.0f, 50.0f}; }
UiRect LiquidGlassUi::ccDisplayRect() const { UiRect o = ccOrigin(); return {o.x, o.y + 214.0f, 248.0f, 50.0f}; }
UiRect LiquidGlassUi::ccQuickCircleRect(int index) const {
    UiRect o = ccOrigin();
    int col = index % 4;
    int row = index / 4;
    return {o.x + col * 64.0f, o.y + 272.0f + row * 52.0f, 56.0f, 44.0f};
}
UiRect LiquidGlassUi::ccEditBtnRect() const { UiRect o = ccOrigin(); return {o.x + 66.0f, o.y + 380.0f, 116.0f, 24.0f}; }
UiRect LiquidGlassUi::ccBoundsRect() const { UiRect o = ccOrigin(); return {o.x, o.y, 248.0f, 408.0f}; }

UiRect LiquidGlassUi::widgetBaseSize(int index) const {
    int tier = widgets_[index].sizeTier;
    if (index == 0) { // Calendar
        if (tier == 0) return {0, 0, 110.0f, 110.0f};
        if (tier == 2) return {0, 0, 310.0f, 198.0f};
        return {0, 0, 196.0f, 184.0f};
    }
    if (index == 1) { // System & Gallery
        if (tier == 0) return {0, 0, 110.0f, 110.0f};
        if (tier == 2) return {0, 0, 290.0f, 166.0f};
        return {0, 0, 176.0f, 120.0f};
    }
    // World Clocks
    if (tier == 0) return {0, 0, 110.0f, 110.0f};
    if (tier == 2) return {0, 0, 316.0f, 128.0f};
    return {0, 0, 224.0f, 94.0f};
}

UiRect LiquidGlassUi::widgetRect(int index) const {
    UiRect b = widgetBaseSize(index);
    float s = widgets_[index].scale;
    float w = b.w * s;
    float h = b.h * s;

    float defaultX = 18.0f;
    float defaultY = 42.0f;
    if (index == 1) {
        UiRect cal = widgetRect(0);
        defaultX = cal.x + cal.w + 12.0f;
        defaultY = 42.0f;
    } else if (index == 2) {
        UiRect cal = widgetRect(0);
        defaultX = 18.0f;
        defaultY = cal.y + cal.h + 12.0f;
    }
    return {defaultX + widgets_[index].dx, defaultY + widgets_[index].dy, w, h};
}

UiRect LiquidGlassUi::widgetSizeBtnRect(int index, int tier) const {
    UiRect r = widgetRect(index);
    float bw = 18.0f;
    float bh = 16.0f;
    float x = r.x + r.w - 64.0f + tier * 19.0f;
    float y = r.y + 6.0f;
    return {x, y, bw, bh};
}

UiRect LiquidGlassUi::widgetResizeHandleRect(int index) const {
    UiRect r = widgetRect(index);
    return {r.x + r.w - 20.0f, r.y + r.h - 20.0f, 20.0f, 20.0f};
}

void LiquidGlassUi::setWidgetSizeTier(int index, int tier) {
    if (index < 0 || index >= 3) return;
    widgets_[index].sizeTier = std::clamp(tier, 0, 2);
    widgets_[index].scale = 1.0f;
    markDirty(SurfaceRole::Widgets);
    markDirty(SurfaceRole::Popups);
    pendingActions_.configChanged = true;
}

UiRect LiquidGlassUi::finderRect() const {
    float w = std::min(640.0f, static_cast<float>(width_) - 120.0f);
    float h = 420.0f;
    float x = (static_cast<float>(width_) - w) * 0.46f + findDx_;
    float y = 136.0f + findDy_;
    return {x, y, w, h};
}

UiRect LiquidGlassUi::alertRect() const {
    float w = 286.0f;
    float h = 230.0f;
    float x = (static_cast<float>(width_) - w) * 0.5f + alertDx_;
    float y = (static_cast<float>(height_) - h) * 0.50f + alertDy_;
    return {x, y, w, h};
}

UiRect LiquidGlassUi::tunerRect() const {
    float w = 304.0f;
    float h = 412.0f;
    float x = 22.0f + tunerDx_;
    float y = static_cast<float>(height_) - h - 92.0f + tunerDy_;
    return {x, y, w, h};
}

UiRect LiquidGlassUi::sliderRectInTuner(int index) const {
    UiRect t = tunerRect();
    return {t.x + 16.0f, t.y + 54.0f + index * 35.0f, t.w - 32.0f, 24.0f};
}

UiRect LiquidGlassUi::dockRectFor(int viewW, int viewH) const {
    float baseIcon = 40.0f;
    float gap = 9.0f;
    float padX = 12.0f;
    float extraWidth = 0.0f;
    for (int i = 0; i < kDockItemCount; ++i) {
        extraWidth += (dockIconScale_[i] - 1.0f) * baseIcon;
    }
    float totalW = padX * 2.0f + kDockItemCount * baseIcon + (kDockItemCount - 1) * gap + 12.0f + extraWidth;
    float h = 56.0f;
    float x = (static_cast<float>(viewW) - totalW) * 0.5f;
    float hideSlide = dockAutoHide_ ? (dockHideProgress_ * 62.0f) : 0.0f;
    float y = static_cast<float>(viewH) - h - 10.0f + hideSlide;
    return {x, y, totalW, h};
}

UiRect LiquidGlassUi::dockIconRectFor(int viewW, int viewH, int index) const {
    UiRect dk = dockRectFor(viewW, viewH);
    float baseIcon = 40.0f;
    float gap = 9.0f;
    float cursorX = dk.x + 12.0f;
    for (int i = 0; i < index; ++i) {
        cursorX += baseIcon * dockIconScale_[i] + gap;
        if (i == kDockItemCount - 2) cursorX += 12.0f;
    }
    float pressScale = (pressedDockIdx_ == index) ? 0.88f : 1.0f;
    float size = baseIcon * dockIconScale_[index] * pressScale;
    float bounceLift = 0.0f;
    if (bouncingDockIdx_ == index && dockBounceAge_ < 0.55f) {
        float t = dockBounceAge_ / 0.55f;
        bounceLift = std::abs(std::sin(t * 2.0f * static_cast<float>(kPi))) * std::exp(-t * 3.0f) * 14.0f;
    }
    // Magnified icons anchor cleanly near bottom edge of dock and grow upward
    float growLift = (dockIconScale_[index] - 1.0f) * baseIcon * 0.42f;
    float cy = dk.y + dk.h * 0.5f - 2.0f - growLift - bounceLift;
    return {cursorX, cy - size * 0.5f, size, size};
}

UiRect LiquidGlassUi::dockTooltipRectFor(int viewW, int viewH) const {
    if (hoveredDockIdx_ < 0 || hoveredDockIdx_ >= kDockItemCount) return {};
    if (dockAutoHide_ && dockHideProgress_ > 0.45f) return {};
    UiRect ir = dockIconRectFor(viewW, viewH, hoveredDockIdx_);
    std::string label = kDockLabels[hoveredDockIdx_];
    float tw = std::max(64.0f, static_cast<float>(label.size()) * 7.5f + 22.0f);
    float th = 22.0f;
    return {ir.x + ir.w * 0.5f - tw * 0.5f, ir.y - th - 10.0f, tw, th};
}

bool LiquidGlassUi::tick(float dtSec, float timeSec) {
    bool changed = false;
    bool dockChanged = false;
    bool barChanged = false;
    bool widChanged = false;

    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);
    if (tmNow.tm_sec != lastClockSecond_) {
        lastClockSecond_ = tmNow.tm_sec;
        barChanged = true;
        widChanged = true;
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
        dockChanged = true;
        changed = true;
    }

    // Smooth macOS Dock hover magnification spring
    int dockW = envMode_ ? kDockWindowWidth : width_;
    int dockH = envMode_ ? kDockWindowHeight : height_;
    UiRect dk = dockRectFor(dockW, dockH);
    bool hoveringDockArea = dockPointerInside_ &&
                            dockPointerY_ >= dk.y - 16.0f &&
                            dockPointerY_ <= static_cast<float>(dockH) &&
                            dockPointerX_ >= dk.x - 16.0f &&
                            dockPointerX_ <= dk.x + dk.w + 16.0f;

    int newHovered = -1;
    if (hoveringDockArea && (!dockAutoHide_ || dockHideProgress_ < 0.35f)) {
        float bestDist = 9999.0f;
        for (int i = 0; i < kDockItemCount; ++i) {
            UiRect ir = dockIconRectFor(dockW, dockH, i);
            float cx = ir.x + ir.w * 0.5f;
            float d = std::abs(dockPointerX_ - cx);
            if (d < bestDist && d < 28.0f) {
                bestDist = d;
                newHovered = i;
            }
        }
    }
    if (newHovered != hoveredDockIdx_) {
        hoveredDockIdx_ = newHovered;
        dockChanged = true;
        changed = true;
    }

    for (int i = 0; i < kDockItemCount; ++i) {
        float target = 1.0f;
        if (hoveringDockArea && (!dockAutoHide_ || dockHideProgress_ < 0.35f)) {
            UiRect ir = dockIconRectFor(dockW, dockH, i);
            float cx = ir.x + ir.w * 0.5f;
            float dist = std::abs(dockPointerX_ - cx) / 110.0f;
            if (dist < 1.0f) {
                target = 1.0f + 0.38f * 0.5f * (1.0f + std::cos(dist * static_cast<float>(kPi)));
            }
        }
        float diff = target - dockIconScale_[i];
        if (std::abs(diff) > 0.003f) {
            dockIconScale_[i] += diff * std::min(1.0f, dtSec * 16.0f);
            dockChanged = true;
            changed = true;
        } else if (dockIconScale_[i] != target) {
            dockIconScale_[i] = target;
            dockChanged = true;
            changed = true;
        }
    }

    // Smooth Dock Auto-Hide slide
    float targetHide = (dockAutoHide_ && !dockPointerInside_) ? 1.0f : 0.0f;
    if (std::abs(targetHide - dockHideProgress_) > 0.004f) {
        dockHideProgress_ += (targetHide - dockHideProgress_) * std::min(1.0f, dtSec * 12.0f);
        dockChanged = true;
        changed = true;
    } else if (dockHideProgress_ != targetHide) {
        dockHideProgress_ = targetHide;
        dockChanged = true;
        changed = true;
    }

    if (barChanged) markDirty(SurfaceRole::MenuBar);
    if (dockChanged) markDirty(SurfaceRole::Dock);
    if (widChanged) markDirty(SurfaceRole::Widgets);
    return changed;
}

void LiquidGlassUi::updateSliderFromPointer(DragTarget target, float px) {
    auto norm = [px](const UiRect& r, float padLeft, float padRight) {
        return std::clamp((px - (r.x + padLeft)) / std::max(r.w - padLeft - padRight, 1.0f), 0.0f, 1.0f);
    };

    switch (target) {
        case DragTarget::VolumeSlider:
            volume_ = norm(ccSoundRect(), 26.0f, 52.0f);
            setSystemVolume(volume_);
            markDirty(SurfaceRole::Popups);
            break;
        case DragTarget::BrightnessSlider:
            brightness_ = norm(ccDisplayRect(), 26.0f, 26.0f);
            setSystemBrightness(brightness_);
            markDirty(SurfaceRole::Popups);
            break;
        case DragTarget::TunerRefraction:
            material_.refraction = norm(sliderRectInTuner(0), 6.0f, 6.0f) * 2.0f;
            markAllDirty();
            break;
        case DragTarget::TunerDispersion:
            material_.dispersion = norm(sliderRectInTuner(1), 6.0f, 6.0f) * 2.5f;
            markAllDirty();
            break;
        case DragTarget::TunerFrost:
            material_.frost = norm(sliderRectInTuner(2), 6.0f, 6.0f);
            markAllDirty();
            break;
        case DragTarget::TunerSpecular:
            material_.specular = norm(sliderRectInTuner(3), 6.0f, 6.0f) * 2.0f;
            markAllDirty();
            break;
        case DragTarget::TunerTint:
            material_.tintAlpha = norm(sliderRectInTuner(4), 6.0f, 6.0f) * 0.45f;
            markAllDirty();
            break;
        default:
            return;
    }
}

void LiquidGlassUi::onPointerMove(float x, float y, SurfaceRole role) {
    if (role == SurfaceRole::Dock) {
        dockPointerInside_ = true;
        dockPointerX_ = x;
        dockPointerY_ = y;
        return;
    }

    pointerX_ = x;
    pointerY_ = y;

    if (role == SurfaceRole::All) {
        UiRect dk = dockRectFor(width_, height_);
        if (y >= dk.y - 40.0f && y <= static_cast<float>(height_)) {
            dockPointerInside_ = true;
            dockPointerX_ = x;
            dockPointerY_ = y;
        } else {
            dockPointerInside_ = false;
        }
    }

    if (role == SurfaceRole::Widgets || role == SurfaceRole::All) {
        int hov = -1;
        if (showWidgets_) {
            for (int i = 2; i >= 0; --i) {
                if (widgetRect(i).contains(x, y)) {
                    hov = i;
                    break;
                }
            }
        }
        if (hov != hoveredWidgetIdx_) {
            hoveredWidgetIdx_ = hov;
            markDirty(SurfaceRole::Widgets);
        }
    }

    if (dragTarget_ == DragTarget::None) return;

    float dx = x - dragStartX_;
    float dy = y - dragStartY_;
    switch (dragTarget_) {
        case DragTarget::Spotlight: spotDx_ = dragBaseDx_ + dx; spotDy_ = dragBaseDy_ + dy; markDirty(SurfaceRole::Popups); break;
        case DragTarget::ControlCenter: ccDx_ = dragBaseDx_ + dx; ccDy_ = dragBaseDy_ + dy; markDirty(SurfaceRole::Popups); break;
        case DragTarget::WidgetMove0: widgets_[0].dx = dragBaseDx_ + dx; widgets_[0].dy = dragBaseDy_ + dy; markDirty(SurfaceRole::Widgets); break;
        case DragTarget::WidgetMove1: widgets_[1].dx = dragBaseDx_ + dx; widgets_[1].dy = dragBaseDy_ + dy; markDirty(SurfaceRole::Widgets); break;
        case DragTarget::WidgetMove2: widgets_[2].dx = dragBaseDx_ + dx; widgets_[2].dy = dragBaseDy_ + dy; markDirty(SurfaceRole::Widgets); break;
        case DragTarget::WidgetResize0:
        case DragTarget::WidgetResize1:
        case DragTarget::WidgetResize2: {
            int idx = (dragTarget_ == DragTarget::WidgetResize0) ? 0 : ((dragTarget_ == DragTarget::WidgetResize1) ? 1 : 2);
            UiRect base = widgetBaseSize(idx);
            float deltaScale = (dx / std::max(base.w, 80.0f) + dy / std::max(base.h, 80.0f)) * 0.5f;
            widgets_[idx].scale = std::clamp(dragBaseScale_ + deltaScale, 0.72f, 1.78f);
            markDirty(SurfaceRole::Widgets);
            break;
        }
        case DragTarget::FinderWindow: findDx_ = dragBaseDx_ + dx; findDy_ = dragBaseDy_ + dy; markDirty(SurfaceRole::Popups); break;
        case DragTarget::Tuner: tunerDx_ = dragBaseDx_ + dx; tunerDy_ = dragBaseDy_ + dy; markDirty(SurfaceRole::Popups); break;
        case DragTarget::AlertDialog: alertDx_ = dragBaseDx_ + dx; alertDy_ = dragBaseDy_ + dy; markDirty(SurfaceRole::Popups); break;
        default: updateSliderFromPointer(dragTarget_, x); break;
    }
}

void LiquidGlassUi::onPointerLeave(SurfaceRole role) {
    if (role == SurfaceRole::Dock || role == SurfaceRole::All) {
        dockPointerInside_ = false;
        hoveredDockIdx_ = -1;
        markDirty(SurfaceRole::Dock);
    }
    if (role == SurfaceRole::Widgets || role == SurfaceRole::All) {
        if (hoveredWidgetIdx_ != -1) {
            hoveredWidgetIdx_ = -1;
            markDirty(SurfaceRole::Widgets);
        }
    }
}

void LiquidGlassUi::onPointerDown(float x, float y, int button, SurfaceRole role) {
    pointerX_ = x;
    pointerY_ = y;
    clickX_ = x;
    clickY_ = y;
    clickAgeSec_ = 0.0f;
    clickActive_ = true;
    clickSurface_ = role;
    lastClickScale_ = surfaces_[static_cast<int>(role)].scale;

    // 1. Dock surface clicks
    if (role == SurfaceRole::Dock || role == SurfaceRole::All) {
        int dw = (role == SurfaceRole::Dock) ? kDockWindowWidth : width_;
        int dh = (role == SurfaceRole::Dock) ? kDockWindowHeight : height_;
        if (dockRectFor(dw, dh).contains(x, y)) {
            for (int i = 0; i < kDockItemCount; ++i) {
                if (dockIconRectFor(dw, dh, i).contains(x, y)) {
                    pressedDockIdx_ = i;
                    bouncingDockIdx_ = i;
                    dockBounceAge_ = 0.0f;
                    if (button == 3 && i == 11) {
                        dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4);
                        pendingActions_.configChanged = true;
                        markAllDirty();
                    } else if (i == 0) {
                        if (button == 3) showFinder_ = !showFinder_;
                        else launchDockApp(0);
                    } else if (i == 1) {
                        showFinder_ = !showFinder_;
                    } else if (i == 3) {
                        showWidgets_ = !showWidgets_;
                        pendingActions_.configChanged = true;
                        markDirty(SurfaceRole::Widgets);
                    } else if (i == 10) {
                        showTuner_ = !showTuner_;
                    } else {
                        launchDockApp(i);
                        if (i == 8) {
                            musicPlaying_ = !musicPlaying_;
                            if (musicPlaying_ && trackIndex_ == 0) trackIndex_ = 1;
                        }
                    }
                    markDirty(SurfaceRole::Dock);
                    markDirty(SurfaceRole::Popups);
                    return;
                }
            }
            if (role == SurfaceRole::Dock) return;
        }
        if (role == SurfaceRole::Dock) return;
    }

    // 2. Menu Bar clicks
    if (role == SurfaceRole::MenuBar || (role == SurfaceRole::All && menuBarRect(width_).contains(x, y))) {
        int vw = (role == SurfaceRole::MenuBar) ? surfaces_[static_cast<int>(SurfaceRole::MenuBar)].viewW : width_;
        UiRect mb = menuBarRect(vw);
        if (mb.contains(x, y)) {
            // Left menu items
            for (int m = 0; m < 7; ++m) {
                if (menuTitleRect(m).contains(x, y)) {
                    int mapped = (m == 0) ? 0 : (m == 1 ? 0 : m - 1);
                    activeMenu_ = (activeMenu_ == mapped) ? -1 : mapped;
                    markDirty(SurfaceRole::MenuBar);
                    markDirty(SurfaceRole::Popups);
                    return;
                }
            }

            float rx = mb.w - 14.0f;
            if (UiRect{rx - 122.0f, 0.0f, 122.0f, mb.h}.contains(x, y)) {
                showWidgets_ = !showWidgets_;
                pendingActions_.configChanged = true;
                markDirty(SurfaceRole::Widgets);
                markDirty(SurfaceRole::MenuBar);
                return;
            }
            if (UiRect{rx - 150.0f, 0.0f, 26.0f, mb.h}.contains(x, y)) {
                showControlCenter_ = !showControlCenter_;
                activeMenu_ = -1;
                markDirty(SurfaceRole::Popups);
                return;
            }
            if (UiRect{rx - 176.0f, 0.0f, 24.0f, mb.h}.contains(x, y)) {
                showSpotlight_ = !showSpotlight_;
                spotlightFocused_ = showSpotlight_;
                activeMenu_ = -1;
                markDirty(SurfaceRole::Popups);
                return;
            }
            if (UiRect{rx - 204.0f, 0.0f, 24.0f, mb.h}.contains(x, y)) {
                wifiOn_ = !wifiOn_;
                toggleSystemWifi(wifiOn_);
                markDirty(SurfaceRole::MenuBar);
                markDirty(SurfaceRole::Popups);
                return;
            }
            float pillLeft = telemetry_.hasBattery ? (rx - 332.0f) : (rx - 282.0f);
            if (UiRect{pillLeft, 4.0f, 68.0f, 20.0f}.contains(x, y)) {
                pendingActions_.requestToggleBackdropMode = true;
                return;
            }
            activeMenu_ = -1;
            markDirty(SurfaceRole::Popups);
            return;
        }
    }

    // 3. Menu Bar Dropdown clicks (in Popups)
    if (activeMenu_ >= 0) {
        UiRect dr = menuDropdownRect();
        if (dr.contains(x, y)) {
            auto entries = getMenuEntries(activeMenu_);
            float ey = dr.y + 6.0f;
            int itemIdx = 0;
            for (const auto& e : entries) {
                float eh = e.separator ? 9.0f : 26.0f;
                if (!e.separator && UiRect{dr.x, ey, dr.w, eh}.contains(x, y)) {
                    int menu = activeMenu_;
                    activeMenu_ = -1;
                    if (menu == 0) {
                        if (itemIdx == 0) showAlert_ = true;
                        else if (itemIdx == 1) showTuner_ = true;
                        else if (itemIdx == 2) toggleWidgetEditMode();
                        else if (itemIdx == 3) { dockAutoHide_ = !dockAutoHide_; pendingActions_.configChanged = true; markDirty(SurfaceRole::Dock); }
                        else if (itemIdx == 4) pendingActions_.requestRecapture = true;
                        else if (itemIdx == 5) lockScreenOrSleep();
                        else if (itemIdx == 6) pendingActions_.requestRestart = true;
                        else if (itemIdx == 7) pendingActions_.requestExit = true;
                    } else if (menu == 1) {
                        if (itemIdx == 0) launchDockApp(9);
                        else if (itemIdx == 1) launchDockApp(0);
                        else if (itemIdx == 2) showFinder_ = true;
                        else if (itemIdx == 3) { showSpotlight_ = true; spotlightFocused_ = true; }
                    } else if (menu == 2) {
                        if (itemIdx == 0) toggleWidgetEditMode();
                        else if (itemIdx == 1) resetWidgetLayout();
                        else if (itemIdx == 2) applyPreset(static_cast<GlassPreset>((static_cast<int>(preset_) + 1) % 4));
                        else if (itemIdx == 3) { dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4); pendingActions_.configChanged = true; markAllDirty(); }
                    } else if (menu == 3) {
                        if (itemIdx == 0) toggleWidgets();
                        else if (itemIdx == 1) toggleControlCenter();
                        else if (itemIdx == 2) pendingActions_.requestToggleBackdropMode = true;
                        else if (itemIdx == 3) { dockAutoHide_ = !dockAutoHide_; pendingActions_.configChanged = true; markDirty(SurfaceRole::Dock); }
                    } else if (menu == 4) {
                        if (itemIdx == 0) spawnDetached("niri msg action toggle-overview >/dev/null 2>&1");
                        else if (itemIdx == 1) spawnDetached("niri msg action focus-window-or-column-right >/dev/null 2>&1");
                        else if (itemIdx == 2) showTuner_ = true;
                    } else if (menu == 5) {
                        if (itemIdx == 0) showAlert_ = true;
                        else if (itemIdx == 1) showTuner_ = true;
                    }
                    markDirty(SurfaceRole::MenuBar);
                    markDirty(SurfaceRole::Popups);
                    return;
                }
                if (!e.separator) ++itemIdx;
                ey += eh;
            }
            return;
        } else {
            activeMenu_ = -1;
            markDirty(SurfaceRole::MenuBar);
            markDirty(SurfaceRole::Popups);
        }
    }

    // 4. Popups: Alert, Control Center, Spotlight, Shader Lab, Applications/Finder
    if (role == SurfaceRole::Popups || role == SurfaceRole::All) {
        if (showAlert_ && alertRect().contains(x, y)) {
            UiRect al = alertRect();
            if (UiRect{al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, 30.0f}.contains(x, y) ||
                UiRect{al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, 30.0f}.contains(x, y)) {
                showAlert_ = false;
                markDirty(SurfaceRole::Popups);
                return;
            }
            dragTarget_ = DragTarget::AlertDialog;
            dragStartX_ = x; dragStartY_ = y;
            dragBaseDx_ = alertDx_; dragBaseDy_ = alertDy_;
            return;
        }

        if (showControlCenter_) {
            if (ccWifiRect().contains(x, y)) { wifiOn_ = !wifiOn_; toggleSystemWifi(wifiOn_); markDirty(SurfaceRole::Popups); markDirty(SurfaceRole::MenuBar); return; }
            if (ccBtCircleRect().contains(x, y)) { btOn_ = !btOn_; toggleSystemBluetooth(btOn_); markDirty(SurfaceRole::Popups); return; }
            if (ccAirCircleRect().contains(x, y)) { airdropOn_ = !airdropOn_; markDirty(SurfaceRole::Popups); return; }
            if (ccFocusPillRect().contains(x, y)) { focusOn_ = !focusOn_; markDirty(SurfaceRole::Popups); return; }
            if (ccStageCircleRect().contains(x, y)) { showFinder_ = !showFinder_; markDirty(SurfaceRole::Popups); return; }
            if (ccMirrorCircleRect().contains(x, y)) {
                dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4);
                pendingActions_.configChanged = true;
                markAllDirty();
                return;
            }
            if (ccEditBtnRect().contains(x, y)) {
                toggleWidgetEditMode();
                return;
            }

            for (int i = 0; i < 8; ++i) {
                if (ccQuickCircleRect(i).contains(x, y)) {
                    quickCircleState_[i] = !quickCircleState_[i];
                    if (i == 0) {
                        darkAppearance_ = !darkAppearance_;
                        applyPreset(darkAppearance_ ? GlassPreset::Frosted : GlassPreset::Clear);
                        pendingActions_.configChanged = true;
                    } else if (i == 1) {
                        toggleWidgets();
                    } else if (i == 2) {
                        dockAutoHide_ = !dockAutoHide_;
                        pendingActions_.configChanged = true;
                        markDirty(SurfaceRole::Dock);
                    } else if (i == 3) {
                        showAlert_ = !showAlert_;
                    } else if (i == 7) {
                        showTuner_ = !showTuner_;
                    }
                    markDirty(SurfaceRole::Popups);
                    return;
                }
            }

            UiRect mp = ccMediaRect();
            if (mp.contains(x, y)) {
                if (UiRect{mp.x + 8.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) {
                    trackIndex_ = (trackIndex_ + 2) % 3;
                    sendMediaCommand("previous");
                    markDirty(SurfaceRole::Popups);
                    return;
                }
                if (UiRect{mp.x + 44.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) {
                    musicPlaying_ = !musicPlaying_;
                    if (musicPlaying_ && trackIndex_ == 0) trackIndex_ = 1;
                    sendMediaCommand("play-pause");
                    markDirty(SurfaceRole::Popups);
                    markDirty(SurfaceRole::Dock);
                    return;
                }
                if (UiRect{mp.x + 80.0f, mp.y + 66.0f, 32.0f, 26.0f}.contains(x, y)) {
                    trackIndex_ = (trackIndex_ + 1) % 3;
                    sendMediaCommand("next");
                    markDirty(SurfaceRole::Popups);
                    return;
                }
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

        if (showSpotlight_) {
            if (spotlightRect().contains(x, y)) {
                spotlightFocused_ = true;
                dragTarget_ = DragTarget::Spotlight;
                dragStartX_ = x; dragStartY_ = y;
                dragBaseDx_ = spotDx_; dragBaseDy_ = spotDy_;
                markDirty(SurfaceRole::Popups);
                return;
            }
            if (spotlightResultsRect().contains(x, y)) {
                auto results = buildSpotlightResults();
                UiRect sr = spotlightResultsRect();
                float rowY = sr.y + 9.0f;
                int count = std::min<int>(static_cast<int>(results.size()), 6);
                for (int i = 0; i < count; ++i) {
                    if (UiRect{sr.x + 8.0f, rowY, sr.w - 16.0f, 36.0f}.contains(x, y)) {
                        executeSpotlightItem(results[i]);
                        return;
                    }
                    rowY += 38.0f;
                }
                return;
            }
        }

        if (showTuner_ && tunerRect().contains(x, y)) {
            UiRect tn = tunerRect();
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
            // Widget Size Controls in Shader Lab
            for (int wIdx = 0; wIdx < 3; ++wIdx) {
                for (int tier = 0; tier < 3; ++tier) {
                    float bx = tn.x + 114.0f + tier * 36.0f;
                    float by = tn.y + 234.0f + wIdx * 26.0f;
                    if (UiRect{bx, by, 32.0f, 20.0f}.contains(x, y)) {
                        setWidgetSizeTier(wIdx, tier);
                        return;
                    }
                }
                // Reset individual scale / layout button on row 0..2 right edge
            }
            if (UiRect{tn.x + 226.0f, tn.y + 234.0f, 62.0f, 20.0f}.contains(x, y)) {
                toggleWidgetEditMode();
                return;
            }
            if (UiRect{tn.x + 226.0f, tn.y + 260.0f, 62.0f, 20.0f}.contains(x, y)) {
                dockAutoHide_ = !dockAutoHide_;
                pendingActions_.configChanged = true;
                markDirty(SurfaceRole::Dock);
                markDirty(SurfaceRole::Popups);
                return;
            }
            if (UiRect{tn.x + 226.0f, tn.y + 286.0f, 62.0f, 20.0f}.contains(x, y)) {
                resetWidgetLayout();
                return;
            }

            if (UiRect{tn.x + 16.0f, tn.y + tn.h - 68.0f, 130.0f, 22.0f}.contains(x, y)) {
                applyPreset(static_cast<GlassPreset>((static_cast<int>(preset_) + 1) % 4));
                pendingActions_.configChanged = true;
                return;
            }
            if (UiRect{tn.x + 158.0f, tn.y + tn.h - 68.0f, 130.0f, 22.0f}.contains(x, y)) {
                dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4);
                pendingActions_.configChanged = true;
                markAllDirty();
                return;
            }
            if (UiRect{tn.x + 16.0f, tn.y + tn.h - 36.0f, 130.0f, 22.0f}.contains(x, y)) {
                pendingActions_.requestRecapture = true;
                return;
            }
            if (UiRect{tn.x + 158.0f, tn.y + tn.h - 36.0f, 130.0f, 22.0f}.contains(x, y)) {
                pendingActions_.requestToggleBackdropMode = true;
                return;
            }

            dragTarget_ = DragTarget::Tuner;
            dragStartX_ = x; dragStartY_ = y;
            dragBaseDx_ = tunerDx_; dragBaseDy_ = tunerDy_;
            return;
        }

        if (showFinder_ && finderRect().contains(x, y)) {
            UiRect f = finderRect();
            if (UiRect{f.x + 14.0f, f.y + 14.0f, 16.0f, 16.0f}.contains(x, y)) {
                showFinder_ = false;
                markDirty(SurfaceRole::Popups);
                return;
            }
            // Page < > buttons
            if (UiRect{f.x + 74.0f, f.y + 10.0f, 24.0f, 22.0f}.contains(x, y)) {
                finderPage_ = std::max(0, finderPage_ - 1);
                markDirty(SurfaceRole::Popups);
                return;
            }
            if (UiRect{f.x + 98.0f, f.y + 10.0f, 24.0f, 22.0f}.contains(x, y)) {
                auto filtered = filteredFinderApps();
                int maxPage = std::max(0, (static_cast<int>(filtered.size()) - 1) / 12);
                finderPage_ = std::min(maxPage, finderPage_ + 1);
                markDirty(SurfaceRole::Popups);
                return;
            }
            // Category pills
            for (int c = 0; c < 4; ++c) {
                if (UiRect{f.x + f.w - 252.0f + c * 60.0f, f.y + 11.0f, 54.0f, 20.0f}.contains(x, y)) {
                    finderCategory_ = c;
                    finderPage_ = 0;
                    markDirty(SurfaceRole::Popups);
                    return;
                }
            }

            // App grid clicks
            auto filtered = filteredFinderApps();
            float startX = f.x + 48.0f;
            float startY = f.y + 62.0f;
            float colStep = (f.w - 96.0f) / 4.0f;
            float rowStep = 104.0f;
            int offset = finderPage_ * 12;
            for (int slot = 0; slot < 12 && offset + slot < static_cast<int>(filtered.size()); ++slot) {
                int col = slot % 4;
                int row = slot / 4;
                float ix = startX + col * colStep + (colStep - 56.0f) * 0.5f;
                float iy = startY + row * rowStep;
                if (UiRect{ix - 10.0f, iy - 4.0f, 76.0f, 84.0f}.contains(x, y)) {
                    launchDesktopApp(desktopApps_[filtered[offset + slot]]);
                    showFinder_ = false;
                    markDirty(SurfaceRole::Popups);
                    return;
                }
            }

            dragTarget_ = DragTarget::FinderWindow;
            dragStartX_ = x; dragStartY_ = y;
            dragBaseDx_ = findDx_; dragBaseDy_ = findDy_;
            return;
        }

        // In multi-surface Popups mode, clicking outside open transient popups dismisses them like macOS
        if (role == SurfaceRole::Popups) {
            bool dismissed = false;
            if (showControlCenter_) { showControlCenter_ = false; dismissed = true; }
            if (showSpotlight_) { showSpotlight_ = false; spotlightFocused_ = false; dismissed = true; }
            if (dismissed) {
                markDirty(SurfaceRole::Popups);
                return;
            }
        }
    }

    // 5. Desktop Widgets (individually movable & resizable!)
    if ((role == SurfaceRole::Widgets || role == SurfaceRole::All) && showWidgets_) {
        for (int i = 2; i >= 0; --i) {
            UiRect wr = widgetRect(i);
            if (!wr.contains(x, y)) continue;

            if (button == 3) {
                toggleWidgetEditMode();
                return;
            }

            // S / M / L quick size buttons
            for (int tier = 0; tier < 3; ++tier) {
                if (widgetSizeBtnRect(i, tier).contains(x, y)) {
                    setWidgetSizeTier(i, tier);
                    return;
                }
            }

            // Bottom-right corner resize handle
            if (widgetResizeHandleRect(i).contains(x, y)) {
                dragTarget_ = (i == 0) ? DragTarget::WidgetResize0 : ((i == 1) ? DragTarget::WidgetResize1 : DragTarget::WidgetResize2);
                dragStartX_ = x; dragStartY_ = y;
                dragBaseScale_ = widgets_[i].scale;
                return;
            }

            // Drag to move individual widget
            dragTarget_ = (i == 0) ? DragTarget::WidgetMove0 : ((i == 1) ? DragTarget::WidgetMove1 : DragTarget::WidgetMove2);
            dragStartX_ = x; dragStartY_ = y;
            dragBaseDx_ = widgets_[i].dx; dragBaseDy_ = widgets_[i].dy;
            return;
        }
    }
}

void LiquidGlassUi::onPointerUp(float x, float y, SurfaceRole role) {
    bool wasWidgetDrag = (dragTarget_ == DragTarget::WidgetMove0 ||
                          dragTarget_ == DragTarget::WidgetMove1 ||
                          dragTarget_ == DragTarget::WidgetMove2 ||
                          dragTarget_ == DragTarget::WidgetResize0 ||
                          dragTarget_ == DragTarget::WidgetResize1 ||
                          dragTarget_ == DragTarget::WidgetResize2);
    if (wasWidgetDrag) {
        pendingActions_.configChanged = true;
    }
    dragTarget_ = DragTarget::None;
    if (pressedDockIdx_ >= 0) {
        pressedDockIdx_ = -1;
        markDirty(SurfaceRole::Dock);
    }
}

bool LiquidGlassUi::onKeyPressed(uint32_t keyval, uint32_t unicodeChar, bool ctrl) {
    if (keyval == 0xff1b) { // Escape
        if (activeMenu_ >= 0) {
            activeMenu_ = -1;
            markDirty(SurfaceRole::MenuBar);
            markDirty(SurfaceRole::Popups);
            return true;
        }
        if (showSpotlight_) {
            if (!spotlightQuery_.empty()) {
                spotlightQuery_.clear();
                spotlightSelected_ = 0;
            } else {
                showSpotlight_ = false;
                spotlightFocused_ = false;
            }
            markDirty(SurfaceRole::Popups);
            return true;
        }
        if (showControlCenter_) { showControlCenter_ = false; markDirty(SurfaceRole::Popups); return true; }
        if (showAlert_) { showAlert_ = false; markDirty(SurfaceRole::Popups); return true; }
        if (showFinder_) { showFinder_ = false; markDirty(SurfaceRole::Popups); return true; }
        if (showTuner_) { showTuner_ = false; markDirty(SurfaceRole::Popups); return true; }
        if (!envMode_) {
            pendingActions_.requestExit = true;
        }
        return true;
    }

    if (spotlightFocused_) {
        if (keyval == 0xff08) { // BackSpace
            if (!spotlightQuery_.empty()) {
                spotlightQuery_.pop_back();
                spotlightSelected_ = 0;
                markDirty(SurfaceRole::Popups);
            }
            return true;
        }
        if (keyval == 0xff52) { // Up arrow
            spotlightSelected_ = std::max(0, spotlightSelected_ - 1);
            markDirty(SurfaceRole::Popups);
            return true;
        }
        if (keyval == 0xff54) { // Down arrow
            auto items = buildSpotlightResults();
            int maxSel = std::max(0, std::min<int>(static_cast<int>(items.size()), 6) - 1);
            spotlightSelected_ = std::min(maxSel, spotlightSelected_ + 1);
            markDirty(SurfaceRole::Popups);
            return true;
        }
        if (keyval == 0xff0d) { // Enter
            auto items = buildSpotlightResults();
            if (!items.empty()) {
                int idx = std::clamp(spotlightSelected_, 0, static_cast<int>(items.size()) - 1);
                executeSpotlightItem(items[idx]);
            } else {
                spotlightFocused_ = false;
                markDirty(SurfaceRole::Popups);
            }
            return true;
        }
        if (!ctrl && unicodeChar >= 32 && unicodeChar < 127 && spotlightQuery_.size() < 40) {
            spotlightQuery_.push_back(static_cast<char>(unicodeChar));
            spotlightSelected_ = 0;
            markDirty(SurfaceRole::Popups);
            return true;
        }
    }

    if (unicodeChar == '1') { applyPreset(GlassPreset::Clear); pendingActions_.configChanged = true; return true; }
    if (unicodeChar == '2') { applyPreset(GlassPreset::Frosted); pendingActions_.configChanged = true; return true; }
    if (unicodeChar == '3') { applyPreset(GlassPreset::Smoked); pendingActions_.configChanged = true; return true; }
    if (unicodeChar == '4') { applyPreset(GlassPreset::Aqua); pendingActions_.configChanged = true; return true; }
    if (unicodeChar == 'i' || unicodeChar == 'I') { dockStyle_ = static_cast<DockIconStyle>((static_cast<int>(dockStyle_) + 1) % 4); pendingActions_.configChanged = true; markAllDirty(); return true; }
    if (unicodeChar == 'e' || unicodeChar == 'E') { toggleWidgetEditMode(); return true; }
    if (unicodeChar == 'r' || unicodeChar == 'R') { pendingActions_.requestRecapture = true; return true; }
    if (unicodeChar == 'b' || unicodeChar == 'B') { pendingActions_.requestToggleBackdropMode = true; return true; }
    if (unicodeChar == 't' || unicodeChar == 'T') { toggleTuner(); return true; }
    if (unicodeChar == 'c' || unicodeChar == 'C') { toggleControlCenter(); return true; }
    if (unicodeChar == 'w' || unicodeChar == 'W') { toggleWidgets(); return true; }
    if (unicodeChar == 'f' || unicodeChar == 'F') { toggleFinder(); return true; }
    if (unicodeChar == 'a' || unicodeChar == 'A') { showAlert_ = !showAlert_; markDirty(SurfaceRole::Popups); return true; }
    if (unicodeChar == 's' || unicodeChar == 'S') { toggleSpotlight(true); return true; }
    if (unicodeChar == 'q' || unicodeChar == 'Q') { pendingActions_.requestExit = true; return true; }
    return false;
}

void LiquidGlassUi::toggleSpotlight(bool focus) {
    showSpotlight_ = !showSpotlight_;
    spotlightFocused_ = showSpotlight_ && focus;
    spotlightSelected_ = 0;
    activeMenu_ = -1;
    markDirty(SurfaceRole::Popups);
}

void LiquidGlassUi::toggleControlCenter() {
    showControlCenter_ = !showControlCenter_;
    activeMenu_ = -1;
    markDirty(SurfaceRole::Popups);
}

void LiquidGlassUi::toggleWidgets() {
    showWidgets_ = !showWidgets_;
    pendingActions_.configChanged = true;
    markDirty(SurfaceRole::Widgets);
    markDirty(SurfaceRole::MenuBar);
}

void LiquidGlassUi::toggleTuner() {
    showTuner_ = !showTuner_;
    markDirty(SurfaceRole::Popups);
}

void LiquidGlassUi::toggleFinder() {
    showFinder_ = !showFinder_;
    markDirty(SurfaceRole::Popups);
}

void LiquidGlassUi::toggleWidgetEditMode() {
    editWidgets_ = !editWidgets_;
    if (editWidgets_ && !showWidgets_) {
        showWidgets_ = true;
    }
    markDirty(SurfaceRole::Widgets);
    markDirty(SurfaceRole::Popups);
}

void LiquidGlassUi::resetWidgetLayout() {
    for (auto& w : widgets_) {
        w.sizeTier = 1;
        w.scale = 1.0f;
        w.dx = 0.0f;
        w.dy = 0.0f;
    }
    pendingActions_.configChanged = true;
    markDirty(SurfaceRole::Widgets);
    markDirty(SurfaceRole::Popups);
}

UiActions LiquidGlassUi::consumeActions() {
    UiActions out = pendingActions_;
    pendingActions_ = {};
    return out;
}

std::vector<GlassPanelSpec> LiquidGlassUi::collectGlassPanels(SurfaceRole role) const {
    std::vector<GlassPanelSpec> panels;

    if (role == SurfaceRole::All || role == SurfaceRole::MenuBar) {
        int vw = (role == SurfaceRole::MenuBar) ? surfaces_[static_cast<int>(SurfaceRole::MenuBar)].viewW : width_;
        UiRect mb = menuBarRect(vw);
        GlassMaterial barMat = material_;
        barMat.frost = 0.34f;
        barMat.tintR = darkAppearance_ ? 0.90f : 0.14f;
        barMat.tintG = darkAppearance_ ? 0.93f : 0.18f;
        barMat.tintB = darkAppearance_ ? 0.98f : 0.28f;
        barMat.tintAlpha = darkAppearance_ ? 0.12f : 0.14f;
        barMat.shadowAlpha = 0.0f;
        barMat.refraction = 0.20f;
        barMat.specular = 0.16f;
        panels.push_back({mb.x, mb.y, mb.w, mb.h, 0.0f, false, true, barMat, 0.34f});
    }

    if ((role == SurfaceRole::All || role == SurfaceRole::Widgets) && showWidgets_) {
        for (int i = 0; i < 3; ++i) {
            UiRect r = widgetRect(i);
            panels.push_back({r.x, r.y, r.w, r.h, 20.0f * widgets_[i].scale, false, false, {}, 0.42f});
        }
    }

    if (role == SurfaceRole::All || role == SurfaceRole::Popups) {
        if (activeMenu_ >= 0) {
            UiRect dr = menuDropdownRect();
            GlassMaterial menuMat = material_;
            menuMat.frost = 0.62f;
            menuMat.tintAlpha = std::max(menuMat.tintAlpha, 0.22f);
            panels.push_back({dr.x, dr.y, dr.w, dr.h, 14.0f, false, true, menuMat, 0.68f});
        }

        if (showFinder_) {
            UiRect f = finderRect();
            GlassMaterial winBody = material_;
            winBody.frost = 0.85f;
            winBody.tintR = 0.96f;
            winBody.tintG = 0.97f;
            winBody.tintB = 0.99f;
            winBody.tintAlpha = 0.78f;
            winBody.shadowAlpha = 0.36f;
            winBody.refraction = 0.35f;
            panels.push_back({f.x, f.y, f.w, f.h, 20.0f, false, true, winBody, 0.86f});
        }

        if (showSpotlight_) {
            UiRect sp = spotlightRect();
            panels.push_back({sp.x, sp.y, sp.w, sp.h, 22.0f, true, false, {}, 0.56f});
            if (spotlightFocused_ || !spotlightQuery_.empty()) {
                UiRect sr = spotlightResultsRect();
                panels.push_back({sr.x, sr.y, sr.w, sr.h, 18.0f, false, false, {}, 0.64f});
            }
        }

        if (showControlCenter_) {
            const UiRect pills[] = {
                ccWifiRect(), ccBtCircleRect(), ccAirCircleRect(), ccMediaRect(),
                ccFocusPillRect(), ccStageCircleRect(), ccMirrorCircleRect(),
                ccSoundRect(), ccDisplayRect(), ccEditBtnRect(),
            };
            for (const UiRect& r : pills) {
                panels.push_back({r.x, r.y, r.w, r.h, std::min(22.0f, r.h * 0.5f), true, false, {}, 0.52f});
            }
            for (int i = 0; i < 8; ++i) {
                UiRect qr = ccQuickCircleRect(i);
                panels.push_back({qr.x, qr.y, qr.w, qr.h, qr.h * 0.5f, true, false, {}, 0.50f});
            }
        }

        if (showTuner_) {
            UiRect tn = tunerRect();
            panels.push_back({tn.x, tn.y, tn.w, tn.h, 20.0f, false, false, {}, 0.62f});
        }

        if (showAlert_) {
            UiRect al = alertRect();
            GlassMaterial alMat = material_;
            alMat.frost = 0.82f;
            alMat.tintR = 0.16f;
            alMat.tintG = 0.20f;
            alMat.tintB = 0.28f;
            alMat.tintAlpha = 0.56f;
            alMat.shadowAlpha = 0.42f;
            panels.push_back({al.x, al.y, al.w, al.h, 24.0f, false, true, alMat, 0.84f});
        }
    }

    if (role == SurfaceRole::All || role == SurfaceRole::Dock) {
        int dw = (role == SurfaceRole::Dock) ? surfaces_[static_cast<int>(SurfaceRole::Dock)].viewW : width_;
        int dh = (role == SurfaceRole::Dock) ? surfaces_[static_cast<int>(SurfaceRole::Dock)].viewH : height_;
        UiRect dk = dockRectFor(dw, dh);
        panels.push_back({dk.x, dk.y, dk.w, dk.h, 19.0f, true, false, {}, 0.44f});

        UiRect tip = dockTooltipRectFor(dw, dh);
        if (tip.w > 1.0f) {
            panels.push_back({tip.x, tip.y, tip.w, tip.h, 11.0f, true, false, {}, 0.62f});
        }

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
                UiRect ir = dockIconRectFor(dw, dh, i);
                panels.push_back({ir.x, ir.y, ir.w, ir.h, ir.w * 0.23f, true, true, iconMat, 0.55f});
            }
        }
    }

    return panels;
}

std::vector<UiRect> LiquidGlassUi::collectInputRects(SurfaceRole role, bool& wantFullScreenCapture) const {
    wantFullScreenCapture = false;
    std::vector<UiRect> rects;

    if (role == SurfaceRole::MenuBar) {
        const auto& s = surfaces_[static_cast<int>(SurfaceRole::MenuBar)];
        rects.push_back(menuBarRect(s.viewW));
    } else if (role == SurfaceRole::Dock) {
        const auto& s = surfaces_[static_cast<int>(SurfaceRole::Dock)];
        if (dockAutoHide_ && dockHideProgress_ > 0.7f) {
            rects.push_back({0.0f, static_cast<float>(s.viewH) - 8.0f, static_cast<float>(s.viewW), 8.0f});
        } else {
            UiRect dk = dockRectFor(s.viewW, s.viewH);
            rects.push_back({dk.x - 10.0f, dk.y - 24.0f, dk.w + 20.0f, dk.h + 34.0f});
        }
    } else if (role == SurfaceRole::Widgets) {
        if (showWidgets_) {
            for (int i = 0; i < 3; ++i) {
                UiRect r = widgetRect(i);
                rects.push_back({r.x - 4.0f, r.y - 4.0f, r.w + 8.0f, r.h + 8.0f});
            }
        }
    } else if (role == SurfaceRole::Popups) {
        if (activeMenu_ >= 0 || showControlCenter_ || showSpotlight_) {
            wantFullScreenCapture = true;
            return rects;
        }
        if (showFinder_) rects.push_back(finderRect());
        if (showTuner_) rects.push_back(tunerRect());
        if (showAlert_) rects.push_back(alertRect());
    }

    return rects;
}

void LiquidGlassUi::destroySurfaces() {
    for (auto& s : surfaces_) {
        if (s.surface) {
            cairo_surface_destroy(s.surface);
            s.surface = nullptr;
        }
        s.surfW = 0;
        s.surfH = 0;
    }
}

void LiquidGlassUi::ensureSurface(SurfaceRole role) {
    SurfaceBuffer& s = surfaces_[static_cast<int>(role)];
    int targetW = s.viewW * s.scale;
    int targetH = s.viewH * s.scale;
    if (s.surface && s.surfW == targetW && s.surfH == targetH) return;
    if (s.surface) {
        cairo_surface_destroy(s.surface);
        s.surface = nullptr;
    }
    s.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, targetW, targetH);
    s.surfW = targetW;
    s.surfH = targetH;
    s.dirty = true;
}

bool LiquidGlassUi::renderUiIfDirty(int& outWidth, int& outHeight, const uint8_t*& outBgra, SurfaceRole role) {
    ensureSurface(role);
    SurfaceBuffer& s = surfaces_[static_cast<int>(role)];
    if (!s.dirty) return false;
    redrawSurface(role);
    cairo_surface_flush(s.surface);
    outWidth = s.surfW;
    outHeight = s.surfH;
    outBgra = cairo_image_surface_get_data(s.surface);
    s.dirty = false;
    return true;
}

void LiquidGlassUi::redrawSurface(SurfaceRole role) {
    SurfaceBuffer& s = surfaces_[static_cast<int>(role)];
    cairo_t* cr = cairo_create(s.surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_scale(cr, static_cast<double>(s.scale), static_cast<double>(s.scale));

    if (role == SurfaceRole::All || role == SurfaceRole::MenuBar) {
        drawMenuBar(cr, s.viewW);
    }
    if (role == SurfaceRole::All || role == SurfaceRole::Widgets) {
        if (showWidgets_) drawWidgets(cr);
    }
    if (role == SurfaceRole::All || role == SurfaceRole::Popups) {
        if (showFinder_) drawFinderWindow(cr);
        if (showSpotlight_) drawSpotlight(cr);
        if (showControlCenter_) drawControlCenter(cr);
        if (showTuner_) drawTuner(cr);
        if (showAlert_) drawAlertDialog(cr);
        if (activeMenu_ >= 0) drawMenuDropdown(cr);
    }
    if (role == SurfaceRole::All || role == SurfaceRole::Dock) {
        drawDock(cr, s.viewW, s.viewH);
    }

    cairo_destroy(cr);
}

void LiquidGlassUi::drawMenuBar(cairo_t* cr, int viewW) const {
    UiRect mb = menuBarRect(viewW);
    Rgba ink = darkAppearance_ ? Rgba{0.08, 0.10, 0.14, 0.94} : Rgba{0.98, 0.99, 1.0, 0.96};
    Rgba sub = darkAppearance_ ? Rgba{0.18, 0.22, 0.28, 0.84} : Rgba{0.92, 0.95, 1.0, 0.88};
    bool shadow = !darkAppearance_;

    // Bottom crisp hairline
    cairo_save(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.16);
    cairo_rectangle(cr, 0.0, mb.h - 0.8, mb.w, 0.8);
    cairo_fill(cr);
    cairo_restore(cr);

    // Highlight active menu item pill
    if (activeMenu_ >= 0) {
        int btnIdx = (activeMenu_ == 0) ? 0 : (activeMenu_ + 1);
        UiRect hr = menuTitleRect(btnIdx);
        pathRoundedRect(cr, hr.x + 2.0f, 3.0f, hr.w - 4.0f, mb.h - 6.0f, 6.0f);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.22);
        cairo_fill(cr);
    }

    drawAppleDropGlyph(cr, 22.0f, mb.h * 0.5f, 15.0f, ink);

    std::string appName = telemetry_.focusedAppName.empty() ? "Finder" : telemetry_.focusedAppName;
    drawText(cr, appName, 42.0f, 0.0f, 88.0f, "Sans Bold 9", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, mb.h, shadow);

    const char* menuNames[5] = {"File", "Edit", "View", "Window", "Help"};
    for (int i = 0; i < 5; ++i) {
        UiRect mr = menuTitleRect(i + 2);
        drawText(cr, menuNames[i], mr.x + 4.0f, 0.0f, mr.w - 4.0f, "Sans 9", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_LEFT, true, mb.h, shadow);
    }

    // Right status cluster
    float rx = mb.w - 14.0f;
    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);
    char clockBuf[36];
    std::strftime(clockBuf, sizeof(clockBuf), "%a %b %-d  %H:%M", &tmNow);

    drawText(cr, clockBuf, rx - 120.0f, 0.0f, 120.0f, "Sans 9", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_RIGHT, true, mb.h, shadow);
    drawControlCenterGlyph(cr, rx - 137.0f, mb.h * 0.5f, 13.5f, ink);
    drawSearchGlyph(cr, rx - 163.0f, mb.h * 0.5f, 13.0f, ink);
    drawWifiGlyph(cr, rx - 191.0f, mb.h * 0.5f, 13.0f, wifiOn_ ? ink : sub);

    if (telemetry_.hasBattery) {
        char batBuf[16];
        std::snprintf(batBuf, sizeof(batBuf), "%d%%", telemetry_.batteryPercent);
        drawText(cr, batBuf, rx - 252.0f, 0.0f, 34.0f, "Sans 8", ink.r, ink.g, ink.b, 0.90, PANGO_ALIGN_RIGHT, true, mb.h, shadow);
        drawBatteryGlyph(cr, rx - 216.0f - 10.0f, mb.h * 0.5f, static_cast<float>(telemetry_.batteryPercent), telemetry_.batteryCharging, ink);
    }

    float pillLeft = telemetry_.hasBattery ? (rx - 332.0f) : (rx - 282.0f);
    const char* modeStr = (backdropMode_ == 0)
        ? (envMode_ ? "Desktop" : (hasDesktopCapture_ ? "Live BG" : "Desktop"))
        : (backdropMode_ == 2 ? "Optics" : "Tahoe");
    pathRoundedRect(cr, pillLeft, 4.0f, 68.0f, 20.0f, 10.0f);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.14);
    cairo_fill(cr);
    drawText(cr, modeStr, pillLeft, 4.0f, 68.0f, "Sans Bold 7.5", ink.r, ink.g, ink.b, 0.94, PANGO_ALIGN_CENTER, true, 20.0f, shadow);
}

void LiquidGlassUi::drawMenuDropdown(cairo_t* cr) const {
    UiRect dr = menuDropdownRect();
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);
    auto entries = getMenuEntries(activeMenu_);
    float ey = dr.y + 6.0f;
    for (const auto& e : entries) {
        if (e.separator) {
            cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.16);
            cairo_rectangle(cr, dr.x + 12.0f, ey + 4.0f, dr.w - 24.0f, 1.0f);
            cairo_fill(cr);
            ey += 9.0f;
            continue;
        }
        UiRect row{dr.x + 6.0f, ey, dr.w - 12.0f, 24.0f};
        if (row.contains(pointerX_, pointerY_)) {
            pathRoundedRect(cr, row.x, row.y, row.w, row.h, 7.0f);
            cairo_set_source_rgba(cr, 0.18, 0.54, 0.98, 0.72);
            cairo_fill(cr);
        }
        drawText(cr, e.label, row.x + 10.0f, row.y, row.w - 48.0f, "Sans 9", ink.r, ink.g, ink.b, 0.96, PANGO_ALIGN_LEFT, true, row.h);
        if (e.shortcut && e.shortcut[0] != '\0') {
            drawText(cr, e.shortcut, row.x + row.w - 44.0f, row.y, 34.0f, "Sans Bold 8", sub.r, sub.g, sub.b, 0.82, PANGO_ALIGN_RIGHT, true, row.h);
        }
        ey += 26.0f;
    }
}

std::vector<LiquidGlassUi::SpotlightResultItem> LiquidGlassUi::buildSpotlightResults() const {
    std::vector<SpotlightResultItem> out;
    std::string q = toLower(spotlightQuery_);

    const struct Builtin { const char* name; const char* sub; double r, g, b; int id; } builtins[] = {
        {"Applications Grid", "Open App Launcher", 0.20, 0.56, 0.98, 0},
        {"Shader Lab & Settings", "Glass & Widget Sizing", 0.52, 0.32, 0.94, 1},
        {"Edit Desktop Widgets", "Resize & Move Widgets", 0.96, 0.62, 0.18, 2},
        {"Control Center", "System Controls", 0.26, 0.66, 0.96, 3},
        {"Toggle Dock Auto-Hide", "Show/Hide Dock Mode", 0.28, 0.76, 0.52, 4},
        {"Recapture Backdrop", "Refresh Glass Blur", 0.36, 0.52, 0.88, 5},
    };

    for (const auto& b : builtins) {
        if (q.empty() || toLower(b.name).find(q) != std::string::npos || toLower(b.sub).find(q) != std::string::npos) {
            out.push_back({b.name, b.sub, b.r, b.g, b.b, b.id, -1});
        }
    }

    if (!q.empty()) {
        const double palette[6][3] = {
            {0.22, 0.58, 0.96}, {0.92, 0.36, 0.42}, {0.26, 0.78, 0.48},
            {0.62, 0.38, 0.94}, {0.96, 0.64, 0.18}, {0.24, 0.74, 0.86},
        };
        for (size_t i = 0; i < desktopApps_.size(); ++i) {
            const auto& app = desktopApps_[i];
            if (toLower(app.name).find(q) != std::string::npos || toLower(app.comment).find(q) != std::string::npos) {
                const auto& c = palette[app.colorIndex % 6];
                out.push_back({app.name, app.comment, c[0], c[1], c[2], -1, static_cast<int>(i)});
                if (out.size() >= 6) break;
            }
        }
    }
    return out;
}

void LiquidGlassUi::executeSpotlightItem(const SpotlightResultItem& item) {
    if (item.appIdx >= 0 && item.appIdx < static_cast<int>(desktopApps_.size())) {
        launchDesktopApp(desktopApps_[item.appIdx]);
    } else if (item.actionId == 0) {
        showFinder_ = true;
    } else if (item.actionId == 1) {
        showTuner_ = true;
    } else if (item.actionId == 2) {
        toggleWidgetEditMode();
    } else if (item.actionId == 3) {
        showControlCenter_ = true;
    } else if (item.actionId == 4) {
        dockAutoHide_ = !dockAutoHide_;
        pendingActions_.configChanged = true;
        markDirty(SurfaceRole::Dock);
    } else if (item.actionId == 5) {
        pendingActions_.requestRecapture = true;
    }
    showSpotlight_ = false;
    spotlightFocused_ = false;
    spotlightQuery_.clear();
    markDirty(SurfaceRole::Popups);
}

std::vector<int> LiquidGlassUi::filteredFinderApps() const {
    std::vector<int> indices;
    for (size_t i = 0; i < desktopApps_.size(); ++i) {
        const std::string cat = toLower(desktopApps_[i].categories);
        if (finderCategory_ == 0) {
            indices.push_back(static_cast<int>(i));
        } else if (finderCategory_ == 1 && (cat.find("system") != std::string::npos || cat.find("settings") != std::string::npos || cat.find("utility") != std::string::npos)) {
            indices.push_back(static_cast<int>(i));
        } else if (finderCategory_ == 2 && (cat.find("development") != std::string::npos || cat.find("office") != std::string::npos)) {
            indices.push_back(static_cast<int>(i));
        } else if (finderCategory_ == 3 && (cat.find("audio") != std::string::npos || cat.find("video") != std::string::npos || cat.find("graphics") != std::string::npos || cat.find("network") != std::string::npos)) {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}

void LiquidGlassUi::drawSpotlight(cairo_t* cr) const {
    UiRect sp = spotlightRect();
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    drawSearchGlyph(cr, sp.x + 22.0f, sp.y + sp.h * 0.5f, 16.0f, sub);
    std::string display = spotlightQuery_.empty()
        ? (spotlightFocused_ ? "|" : "Spotlight Search (Apps & Mizzle)")
        : (spotlightFocused_ ? (spotlightQuery_ + "|") : spotlightQuery_);
    Rgba txtCol = spotlightQuery_.empty() ? sub : ink;
    drawText(cr, display, sp.x + 42.0f, sp.y, sp.w - 60.0f, "Sans 11", txtCol.r, txtCol.g, txtCol.b, txtCol.a, PANGO_ALIGN_LEFT, true, sp.h);

    if (spotlightFocused_ || !spotlightQuery_.empty()) {
        UiRect sr = spotlightResultsRect();
        auto results = buildSpotlightResults();
        if (results.empty()) {
            drawText(cr, "No matching apps or actions", sr.x + 18.0f, sr.y, sr.w - 36.0f, "Sans 9", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_CENTER, true, sr.h);
            return;
        }
        float rowY = sr.y + 9.0f;
        int shown = std::min<int>(static_cast<int>(results.size()), 6);
        int sel = std::clamp(spotlightSelected_, 0, shown - 1);
        for (int i = 0; i < shown; ++i) {
            const auto& entry = results[i];
            if (i == sel) {
                pathRoundedRect(cr, sr.x + 10.0f, rowY, sr.w - 20.0f, 36.0f, 11.0f);
                cairo_set_source_rgba(cr, 0.24, 0.58, 1.0, 0.30);
                cairo_fill(cr);
            }
            pathRoundedRect(cr, sr.x + 16.0f, rowY + 6.0f, 24.0f, 24.0f, 7.0f);
            cairo_set_source_rgba(cr, entry.r, entry.g, entry.b, 0.94);
            cairo_fill(cr);
            std::string initial(1, entry.title.empty() ? 'A' : entry.title[0]);
            drawText(cr, initial, sr.x + 16.0f, rowY + 6.0f, 24.0f, "Sans Bold 9", 1.0, 1.0, 1.0, 0.96, PANGO_ALIGN_CENTER, true, 24.0f);
            drawText(cr, entry.title, sr.x + 48.0f, rowY, 220.0f, "Sans Bold 9.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, 36.0f);
            drawText(cr, entry.subtitle, sr.x + sr.w - 210.0f, rowY, 192.0f, "Sans 8", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_RIGHT, true, 36.0f);
            rowY += 38.0f;
        }
    }
}

void LiquidGlassUi::drawControlCenter(cairo_t* cr) const {
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    // Wi-Fi slim pill
    UiRect wf = ccWifiRect();
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_arc(cr, wf.x + 22.0f, wf.y + wf.h * 0.5f, 14.0f, 0.0, 2.0 * kPi);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, wifiOn_ ? 0.26 : 0.10);
    cairo_fill(cr);
    cairo_restore(cr);
    drawWifiGlyph(cr, wf.x + 22.0f, wf.y + wf.h * 0.5f, 14.0f, wifiOn_ ? Rgba{0.35, 0.72, 1.0, 1.0} : ink);
    drawText(cr, "Wi-Fi", wf.x + 42.0f, wf.y + 8.0f, 70.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a);
    drawText(cr, wifiOn_ ? telemetry_.wifiSsid : "Off", wf.x + 42.0f, wf.y + 22.0f, 72.0f, "Sans 7", sub.r, sub.g, sub.b, sub.a);

    // Bluetooth + AirDrop circle droplets
    UiRect bt = ccBtCircleRect();
    drawBluetoothGlyph(cr, bt.x + bt.w * 0.5f, bt.y + bt.h * 0.5f, 16.0f, btOn_ ? Rgba{0.38, 0.74, 1.0, 1.0} : ink);
    UiRect ad = ccAirCircleRect();
    drawAirDropGlyph(cr, ad.x + ad.w * 0.5f, ad.y + ad.h * 0.5f, 16.0f, airdropOn_ ? Rgba{0.38, 0.74, 1.0, 1.0} : ink);

    // Media card
    UiRect mp = ccMediaRect();
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
    std::string trackName = !telemetry_.mediaTitle.empty() ? telemetry_.mediaTitle : kTracks[trackIndex_];
    drawText(cr, trackName, mp.x + 12.0f, mp.y + 49.0f, mp.w - 24.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a);

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
    UiRect fp = ccFocusPillRect();
    drawMoonGlyph(cr, fp.x + 22.0f, fp.y + fp.h * 0.5f, 14.0f, focusOn_ ? Rgba{0.62, 0.50, 1.0, 1.0} : ink);
    drawText(cr, "Focus", fp.x + 42.0f, fp.y, 70.0f, "Sans Bold 8", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_LEFT, true, fp.h);

    auto drawBoxIcon = [&](UiRect r, bool active, bool doubleBox) {
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

    auto drawCcSlider = [&](UiRect r, const char* title, float val, bool isSound) {
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

    const char* quickLabels[8] = {"◐", "▦", "◷", "⌑", "⊞", "◫", "◎", "⚙"};
    for (int i = 0; i < 8; ++i) {
        UiRect qr = ccQuickCircleRect(i);
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

    UiRect eb = ccEditBtnRect();
    drawText(cr, editWidgets_ ? "Done Widgets" : "Edit Widgets", eb.x, eb.y, eb.w, "Sans Bold 7.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER, true, eb.h);
}

void LiquidGlassUi::drawWidgets(cairo_t* cr) const {
    for (int i = 0; i < 3; ++i) {
        drawSingleWidget(cr, i);
    }
}

void LiquidGlassUi::drawSingleWidget(cairo_t* cr, int idx) const {
    UiRect r = widgetRect(idx);
    UiRect base = widgetBaseSize(idx);
    float sc = widgets_[idx].scale;
    int tier = widgets_[idx].sizeTier;

    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);
    bool shadow = !darkAppearance_;

    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);

    cairo_save(cr);
    cairo_translate(cr, r.x, r.y);
    cairo_scale(cr, sc, sc);

    if (idx == 0) {
        // Real Calendar Widget (S / M / L)
        int year = tmNow.tm_year + 1900;
        int month = tmNow.tm_mon;
        int today = tmNow.tm_mday;
        int totalDays = daysInMonth(year, month);
        int startCol = firstWeekdayMonday0(year, month, today, tmNow.tm_wday);

        if (tier == 0) {
            // Small Calendar
            drawText(cr, kMonthNamesUpper[month], 10.0f, 10.0f, base.w - 20.0f, "Sans Bold 7", 1.0, 0.38, 0.34, 0.96, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
            char dayBuf[8];
            std::snprintf(dayBuf, sizeof(dayBuf), "%d", today);
            drawText(cr, dayBuf, 8.0f, 24.0f, base.w - 16.0f, "Sans Bold 34", ink.r, ink.g, ink.b, 0.98, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
            drawText(cr, kWeekdayNames[tmNow.tm_wday], 8.0f, 78.0f, base.w - 16.0f, "Sans 8", sub.r, sub.g, sub.b, 0.90, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
        } else {
            // Medium or Large Calendar
            float gridW = (tier == 2) ? 182.0f : (base.w - 16.0f);
            float colW = gridW / 7.0f;
            float rowStep = (tier == 2) ? 21.0f : 19.0f;

            char headerBuf[32];
            std::snprintf(headerBuf, sizeof(headerBuf), "%s %d", kMonthNamesUpper[month], year);
            drawText(cr, headerBuf, 12.0f, 10.0f, gridW, "Sans Bold 7.5", 1.0, 0.42, 0.36, 0.95, PANGO_ALIGN_LEFT, false, 0.0f, shadow);

            const char* wd[7] = {"M", "T", "W", "T", "F", "S", "S"};
            for (int c = 0; c < 7; ++c) {
                drawText(cr, wd[c], 10.0f + c * colW, 27.0f, colW, "Sans Bold 6.5", sub.r, sub.g, sub.b, 0.80, PANGO_ALIGN_CENTER);
            }
            for (int day = 1; day <= totalDays; ++day) {
                int slot = startCol + (day - 1);
                int row = slot / 7;
                int col = slot % 7;
                float dx = 10.0f + col * colW;
                float dy = 42.0f + row * rowStep;
                if (day == today) {
                    cairo_save(cr);
                    cairo_new_path(cr);
                    cairo_arc(cr, dx + colW * 0.5f, dy + 6.5f, 7.6f, 0.0, 2.0 * kPi);
                    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.92);
                    cairo_fill(cr);
                    cairo_restore(cr);
                }
                char dBuf[4];
                std::snprintf(dBuf, sizeof(dBuf), "%d", day);
                drawText(cr, dBuf, dx, dy, colW, (day == today) ? "Sans Bold 7" : "Sans 6.5",
                         (day == today) ? 0.10 : ink.r, (day == today) ? 0.12 : ink.g, (day == today) ? 0.16 : ink.b,
                         0.95, PANGO_ALIGN_CENTER, false, 0.0f, (day != today && shadow));
            }

            if (tier == 2) {
                // Right-hand Large Calendar Today + Live Clock panel
                cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.18);
                cairo_rectangle(cr, 200.0f, 18.0f, 1.0f, base.h - 36.0f);
                cairo_fill(cr);

                drawText(cr, kWeekdayNames[tmNow.tm_wday], 210.0f, 22.0f, 90.0f, "Sans Bold 8.5", sub.r, sub.g, sub.b, 0.92, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
                char dayBuf[8];
                std::snprintf(dayBuf, sizeof(dayBuf), "%d", today);
                drawText(cr, dayBuf, 210.0f, 44.0f, 90.0f, "Sans Bold 34", ink.r, ink.g, ink.b, 0.98, PANGO_ALIGN_CENTER, false, 0.0f, shadow);

                char timeBuf[16];
                std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmNow);
                drawText(cr, timeBuf, 210.0f, 108.0f, 90.0f, "Sans Bold 10", ink.r, ink.g, ink.b, 0.92, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
                drawText(cr, "Local Time", 210.0f, 128.0f, 90.0f, "Sans 7", sub.r, sub.g, sub.b, 0.80, PANGO_ALIGN_CENTER);
            }
        }
    } else if (idx == 1) {
        // System Telemetry + Architectural Gallery Widget (S / M / L)
        if (tier == 0) {
            drawText(cr, "SYSTEM", 12.0f, 10.0f, base.w - 24.0f, "Sans Bold 6.5", sub.r, sub.g, sub.b, 0.88, PANGO_ALIGN_LEFT, false, 0.0f, shadow);
            char cpuBuf[16];
            std::snprintf(cpuBuf, sizeof(cpuBuf), "%.0f%%", telemetry_.cpuPercent);
            drawText(cr, cpuBuf, 12.0f, 28.0f, base.w - 24.0f, "Sans Bold 22", ink.r, ink.g, ink.b, 0.96, PANGO_ALIGN_LEFT, false, 0.0f, shadow);
            drawText(cr, "CPU Load", 12.0f, 62.0f, base.w - 24.0f, "Sans 7", sub.r, sub.g, sub.b, 0.86);
            char ramBuf[32];
            std::snprintf(ramBuf, sizeof(ramBuf), "RAM %.0f%%", telemetry_.ramPercent);
            drawText(cr, ramBuf, 12.0f, 82.0f, base.w - 24.0f, "Sans Bold 8", 0.38, 0.78, 1.0, 0.96, PANGO_ALIGN_LEFT, false, 0.0f, shadow);
        } else {
            // Architectural backdrop card on left/top + System Stats
            float artW = (tier == 2) ? 106.0f : 72.0f;
            float artH = base.h - 14.0f;
            cairo_save(cr);
            pathRoundedRect(cr, 7.0f, 7.0f, artW, artH, 14.0f);
            cairo_clip(cr);
            cairo_pattern_t* skyP = cairo_pattern_create_linear(7.0, 7.0, 7.0, 7.0 + artH);
            cairo_pattern_add_color_stop_rgba(skyP, 0.0, 0.52, 0.62, 0.76, 0.56);
            cairo_pattern_add_color_stop_rgba(skyP, 1.0, 0.28, 0.36, 0.48, 0.56);
            cairo_set_source(cr, skyP);
            cairo_paint(cr);
            cairo_pattern_destroy(skyP);
            cairo_set_source_rgba(cr, 0.94, 0.97, 1.0, 0.52);
            cairo_move_to(cr, 14.0f, 40.0f);
            cairo_line_to(cr, 7.0f + artW * 0.5f, 18.0f);
            cairo_line_to(cr, 7.0f + artW - 7.0f, 40.0f);
            cairo_close_path(cr);
            cairo_fill(cr);
            for (int c = 0; c < 3; ++c) {
                cairo_rectangle(cr, 18.0f + c * (artW * 0.26f), 43.0f, 6.0f, artH - 42.0f);
            }
            cairo_fill(cr);
            cairo_restore(cr);

            float sx = artW + 16.0f;
            float sw = base.w - sx - 12.0f;
            drawText(cr, "SYSTEM", sx, 12.0f, sw, "Sans Bold 6.5", sub.r, sub.g, sub.b, 0.88, PANGO_ALIGN_LEFT, false, 0.0f, shadow);

            auto drawStatBar = [&](float y, const char* label, float pct, double cr_, double cg_, double cb_) {
                char vbuf[20];
                std::snprintf(vbuf, sizeof(vbuf), "%.0f%%", pct);
                drawText(cr, label, sx, y, sw - 28.0f, "Sans Bold 7.5", ink.r, ink.g, ink.b, 0.94, PANGO_ALIGN_LEFT, false, 0.0f, shadow);
                drawText(cr, vbuf, sx + sw - 32.0f, y, 32.0f, "Sans Bold 7.5", sub.r, sub.g, sub.b, 0.90, PANGO_ALIGN_RIGHT);
                pathRoundedRect(cr, sx, y + 14.0f, sw, 4.5f, 2.25f);
                cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.16);
                cairo_fill(cr);
                pathRoundedRect(cr, sx, y + 14.0f, std::max(4.5f, sw * std::clamp(pct / 100.0f, 0.0f, 1.0f)), 4.5f, 2.25f);
                cairo_set_source_rgba(cr, cr_, cg_, cb_, 0.95);
                cairo_fill(cr);
            };

            drawStatBar(30.0f, "CPU", telemetry_.cpuPercent, 0.30, 0.72, 1.0);
            drawStatBar(60.0f, "RAM", telemetry_.ramPercent, 0.45, 0.86, 0.62);
            if (tier == 2) {
                if (telemetry_.hasBattery) {
                    drawStatBar(90.0f, "Battery", static_cast<float>(telemetry_.batteryPercent), 0.98, 0.72, 0.24);
                } else {
                    drawStatBar(90.0f, "Volume", volume_ * 100.0f, 0.72, 0.54, 0.98);
                }
                char memStr[48];
                std::snprintf(memStr, sizeof(memStr), "%.1f / %.1f GB  •  %s", telemetry_.ramUsedGb, telemetry_.ramTotalGb, telemetry_.wifiSsid.c_str());
                drawText(cr, memStr, sx, 128.0f, sw, "Sans 7", sub.r, sub.g, sub.b, 0.86, PANGO_ALIGN_LEFT, false, 0.0f, shadow);
            }
        }
    } else {
        // World Clocks Widget (S / M / L)
        std::tm tmUtc{};
        gmtime_r(&now, &tmUtc);
        const char* cities[] = {"Cupertino", "Tokyo", "Sydney", "Paris"};
        const int utcOff[] = {-8, 9, 11, 1};

        if (tier == 0) {
            drawTranslucentClock(cr, base.w * 0.5f, 40.0f, 26.0f, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
            char dig[16];
            std::strftime(dig, sizeof(dig), "%H:%M", &tmNow);
            drawText(cr, dig, 6.0f, 70.0f, base.w - 12.0f, "Sans Bold 10", ink.r, ink.g, ink.b, 0.96, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
            drawText(cr, "Local", 6.0f, 88.0f, base.w - 12.0f, "Sans 6.5", sub.r, sub.g, sub.b, 0.85, PANGO_ALIGN_CENTER);
        } else {
            float step = base.w / 4.0f;
            float clockR = (tier == 2) ? 24.0f : 18.0f;
            float clockY = (tier == 2) ? 38.0f : 31.0f;
            for (int i = 0; i < 4; ++i) {
                float cx = step * (i + 0.5f);
                int hh = (tmUtc.tm_hour + utcOff[i] + 24) % 24;
                drawTranslucentClock(cr, cx, clockY, clockR, hh, tmUtc.tm_min, (tier == 2) ? tmUtc.tm_sec : -1);
                drawText(cr, cities[i], cx - step * 0.48f, clockY + clockR + 6.0f, step * 0.96f,
                         tier == 2 ? "Sans Bold 7.5" : "Sans Bold 6.5", ink.r, ink.g, ink.b, 0.94, PANGO_ALIGN_CENTER, false, 0.0f, shadow);
                if (tier == 2) {
                    char buf[24];
                    std::snprintf(buf, sizeof(buf), "%02d:%02d  (%+dh)", hh, tmUtc.tm_min, utcOff[i]);
                    drawText(cr, buf, cx - step * 0.48f, clockY + clockR + 22.0f, step * 0.96f, "Sans 6.5", sub.r, sub.g, sub.b, 0.85, PANGO_ALIGN_CENTER);
                } else {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%02d:%02d", hh, tmUtc.tm_min);
                    drawText(cr, buf, cx - step * 0.48f, clockY + clockR + 18.0f, step * 0.96f, "Sans 6", sub.r, sub.g, sub.b, 0.82, PANGO_ALIGN_CENTER);
                }
            }
        }
    }

    cairo_restore(cr);

    // Draw S / M / L size controls + corner resize handle when hovered or in Edit Widgets mode
    if (editWidgets_ || hoveredWidgetIdx_ == idx) {
        const char* labels[3] = {"S", "M", "L"};
        for (int t = 0; t < 3; ++t) {
            UiRect sb = widgetSizeBtnRect(idx, t);
            bool active = (tier == t);
            pathRoundedRect(cr, sb.x, sb.y, sb.w, sb.h, 5.0f);
            cairo_set_source_rgba(cr, active ? 0.22 : 0.10, active ? 0.60 : 0.14, active ? 1.0 : 0.20, active ? 0.92 : 0.62);
            cairo_fill(cr);
            drawText(cr, labels[t], sb.x, sb.y, sb.w, "Sans Bold 6.5", 1.0, 1.0, 1.0, 0.96, PANGO_ALIGN_CENTER, true, sb.h);
        }

        // Bottom-right resize grip arc
        cairo_save(cr);
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.78);
        cairo_set_line_width(cr, 1.8);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_arc(cr, r.x + r.w - 15.0f, r.y + r.h - 15.0f, 10.0f, 0.15, kPi * 0.5 - 0.15);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

void LiquidGlassUi::drawFinderWindow(cairo_t* cr) const {
    UiRect f = finderRect();
    Rgba darkInk{0.10, 0.12, 0.16, 0.94};
    Rgba darkSub{0.34, 0.38, 0.45, 0.82};

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

    drawText(cr, "<   >", f.x + 76.0f, f.y + 11.0f, 44.0f, "Sans Bold 9", darkSub.r, darkSub.g, darkSub.b, 0.88);
    drawText(cr, "Applications", f.x + 124.0f, f.y + 11.0f, 160.0f, "Sans Bold 10", darkInk.r, darkInk.g, darkInk.b, 0.95);

    const char* cats[4] = {"All", "System", "Work", "Media"};
    for (int c = 0; c < 4; ++c) {
        float cx = f.x + f.w - 252.0f + c * 60.0f;
        float cy = f.y + 11.0f;
        bool act = (finderCategory_ == c);
        pathRoundedRect(cr, cx, cy, 54.0f, 20.0f, 10.0f);
        cairo_set_source_rgba(cr, act ? 0.16 : 0.12, act ? 0.52 : 0.14, act ? 0.98 : 0.20, act ? 0.90 : 0.12);
        cairo_fill(cr);
        drawText(cr, cats[c], cx, cy, 54.0f, "Sans Bold 7.5", act ? 1.0 : darkInk.r, act ? 1.0 : darkInk.g, act ? 1.0 : darkInk.b, 0.92, PANGO_ALIGN_CENTER, true, 20.0f);
    }

    auto filtered = filteredFinderApps();
    int totalApps = static_cast<int>(filtered.size());
    int maxPage = std::max(0, (totalApps - 1) / 12);
    int page = std::clamp(finderPage_, 0, maxPage);
    int offset = page * 12;

    float startX = f.x + 48.0f;
    float startY = f.y + 62.0f;
    float colStep = (f.w - 96.0f) / 4.0f;
    float rowStep = 104.0f;

    const double palette[6][3] = {
        {0.20, 0.56, 0.98}, {0.92, 0.34, 0.42}, {0.22, 0.78, 0.48},
        {0.58, 0.36, 0.96}, {0.98, 0.62, 0.16}, {0.20, 0.70, 0.86},
    };

    for (int slot = 0; slot < 12 && offset + slot < totalApps; ++slot) {
        const auto& app = desktopApps_[filtered[offset + slot]];
        int col = slot % 4;
        int row = slot / 4;
        float ix = startX + col * colStep + (colStep - 54.0f) * 0.5f;
        float iy = startY + row * rowStep;

        if (slot < 12 && app.name == "Files") {
            drawVectorAppIcon(cr, 0, ix, iy, 54.0f, DockIconStyle::Default, 27);
        } else {
            const auto& c = palette[app.colorIndex % 6];
            cairo_pattern_t* pat = cairo_pattern_create_linear(ix, iy, ix, iy + 54.0f);
            cairo_pattern_add_color_stop_rgba(pat, 0.0, std::min(1.0, c[0] + 0.16), std::min(1.0, c[1] + 0.16), std::min(1.0, c[2] + 0.16), 0.96);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, c[0], c[1], c[2], 0.98);
            pathRoundedRect(cr, ix, iy, 54.0f, 54.0f, 13.0f);
            cairo_set_source(cr, pat);
            cairo_fill(cr);
            cairo_pattern_destroy(pat);
            std::string initial(1, app.name.empty() ? 'A' : app.name[0]);
            drawText(cr, initial, ix, iy, 54.0f, "Sans Bold 18", 1.0, 1.0, 1.0, 0.96, PANGO_ALIGN_CENTER, true, 54.0f);
        }
        drawText(cr, app.name, ix - 16.0f, iy + 60.0f, 86.0f, "Sans 8", darkInk.r, darkInk.g, darkInk.b, 0.92, PANGO_ALIGN_CENTER);
    }

    char footer[128];
    std::snprintf(footer, sizeof(footer), "%d installed apps  •  Page %d of %d  (click to launch)", totalApps, page + 1, maxPage + 1);
    drawText(cr, footer, f.x, f.y + f.h - 24.0f, f.w, "Sans 7.5", darkSub.r, darkSub.g, darkSub.b, 0.86, PANGO_ALIGN_CENTER);
}

void LiquidGlassUi::drawAlertDialog(cairo_t* cr) const {
    UiRect al = alertRect();
    Rgba ink{0.98, 0.99, 1.0, 0.96};
    Rgba sub{0.84, 0.89, 0.97, 0.84};

    pathRoundedRect(cr, al.x + al.w * 0.5f - 20.0f, al.y + 18.0f, 40.0f, 40.0f, 11.0f);
    cairo_set_source_rgba(cr, 0.16, 0.48, 0.96, 0.96);
    cairo_fill(cr);
    drawAppleDropGlyph(cr, al.x + al.w * 0.5f, al.y + 38.0f, 22.0f, {1.0, 1.0, 1.0, 0.98});

    drawText(cr, "Mizzle Liquid Glass Environment",
             al.x + 16.0f, al.y + 66.0f, al.w - 32.0f, "Sans Bold 9.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER);
    drawText(cr, "1-4 Presets • E Edit Widgets • I Dock Style\nS Spotlight • C Control Center • T Shader Lab",
             al.x + 16.0f, al.y + 92.0f, al.w - 32.0f, "Sans 7.5", sub.r, sub.g, sub.b, sub.a, PANGO_ALIGN_CENTER);

    pathRoundedRect(cr, al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, 30.0f, 15.0f);
    cairo_set_source_rgba(cr, 0.08, 0.52, 1.0, 0.96);
    cairo_fill(cr);
    drawText(cr, "Done", al.x + 18.0f, al.y + al.h - 82.0f, al.w - 36.0f, "Sans Bold 9", 1.0, 1.0, 1.0, 0.98, PANGO_ALIGN_CENTER, true, 30.0f);

    pathRoundedRect(cr, al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, 30.0f, 15.0f);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.16);
    cairo_fill(cr);
    drawText(cr, "Dismiss", al.x + 18.0f, al.y + al.h - 44.0f, al.w - 36.0f, "Sans Bold 9", 1.0, 1.0, 1.0, 0.95, PANGO_ALIGN_CENTER, true, 30.0f);
}

void LiquidGlassUi::drawTuner(cairo_t* cr) const {
    UiRect tn = tunerRect();
    Rgba ink = inkPrimary(darkAppearance_);
    Rgba sub = inkSecondary(darkAppearance_);

    drawText(cr, "Mizzle Shader & Widget Lab", tn.x + 16.0f, tn.y + 14.0f, tn.w - 32.0f, "Sans Bold 9.5", ink.r, ink.g, ink.b, ink.a);
    drawText(cr, "1-4 Presets • E Resize Widgets • I Icons • B BG", tn.x + 16.0f, tn.y + 30.0f, tn.w - 32.0f, "Sans 6.5", sub.r, sub.g, sub.b, sub.a);

    struct Row { const char* name; float norm; float val; };
    const Row rows[] = {
        {"Refraction", material_.refraction / 2.0f, material_.refraction},
        {"Dispersion", material_.dispersion / 2.5f, material_.dispersion},
        {"Frost / Blur", material_.frost, material_.frost},
        {"Rim Shine", material_.specular / 2.0f, material_.specular},
        {"Tint Opacity", material_.tintAlpha / 0.45f, material_.tintAlpha},
    };
    for (int i = 0; i < 5; ++i) {
        UiRect sr = sliderRectInTuner(i);
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

    // Widget Size Controls
    const char* wNames[3] = {"Calendar", "System", "Clocks"};
    const char* sLabels[3] = {"S", "M", "L"};
    for (int wIdx = 0; wIdx < 3; ++wIdx) {
        float ry = tn.y + 234.0f + wIdx * 26.0f;
        drawText(cr, wNames[wIdx], tn.x + 16.0f, ry, 92.0f, "Sans 8", ink.r, ink.g, ink.b, 0.92, PANGO_ALIGN_LEFT, true, 20.0f);
        for (int tier = 0; tier < 3; ++tier) {
            float bx = tn.x + 114.0f + tier * 36.0f;
            bool act = (widgets_[wIdx].sizeTier == tier);
            pathRoundedRect(cr, bx, ry, 32.0f, 20.0f, 7.0f);
            cairo_set_source_rgba(cr, act ? 0.20 : ink.r, act ? 0.58 : ink.g, act ? 1.0 : ink.b, act ? 0.88 : 0.14);
            cairo_fill(cr);
            drawText(cr, sLabels[tier], bx, ry, 32.0f, "Sans Bold 7.5", act ? 1.0 : ink.r, act ? 1.0 : ink.g, act ? 1.0 : ink.b, 0.95, PANGO_ALIGN_CENTER, true, 20.0f);
        }
    }

    auto drawSmallAction = [&](float x, float y, float w, const char* label, bool active) {
        pathRoundedRect(cr, x, y, w, 20.0f, 7.0f);
        cairo_set_source_rgba(cr, active ? 0.20 : ink.r, active ? 0.58 : ink.g, active ? 1.0 : ink.b, active ? 0.88 : 0.14);
        cairo_fill(cr);
        drawText(cr, label, x, y, w, "Sans Bold 7", active ? 1.0 : ink.r, active ? 1.0 : ink.g, active ? 1.0 : ink.b, 0.95, PANGO_ALIGN_CENTER, true, 20.0f);
    };
    drawSmallAction(tn.x + 226.0f, tn.y + 234.0f, 62.0f, editWidgets_ ? "Editing" : "Edit W.", editWidgets_);
    drawSmallAction(tn.x + 226.0f, tn.y + 260.0f, 62.0f, dockAutoHide_ ? "Hide: On" : "Hide: Off", dockAutoHide_);
    drawSmallAction(tn.x + 226.0f, tn.y + 286.0f, 62.0f, "Reset Pos", false);

    const char* presetNames[] = {"Preset: Clear", "Preset: Frost", "Preset: Smoked", "Preset: Aqua"};
    const char* dockNames[] = {"Icons: Default", "Icons: Dark", "Icons: Clear", "Icons: Tinted"};
    auto drawBtn = [&](float x, float y, float w, const char* label) {
        pathRoundedRect(cr, x, y, w, 22.0f, 11.0f);
        cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.14);
        cairo_fill(cr);
        drawText(cr, label, x, y, w, "Sans Bold 7.5", ink.r, ink.g, ink.b, ink.a, PANGO_ALIGN_CENTER, true, 22.0f);
    };
    drawBtn(tn.x + 16.0f, tn.y + tn.h - 68.0f, 130.0f, presetNames[static_cast<int>(preset_)]);
    drawBtn(tn.x + 158.0f, tn.y + tn.h - 68.0f, 130.0f, dockNames[static_cast<int>(dockStyle_)]);
    drawBtn(tn.x + 16.0f, tn.y + tn.h - 36.0f, 130.0f, "Recapture (R)");
    drawBtn(tn.x + 158.0f, tn.y + tn.h - 36.0f, 130.0f, "Toggle BG (B)");
}

void LiquidGlassUi::drawDock(cairo_t* cr, int viewW, int viewH) const {
    UiRect dk = dockRectFor(viewW, viewH);
    Rgba ink = inkPrimary(darkAppearance_);

    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_r(&now, &tmNow);

    // Separator before Trash (between item 10 and 11)
    UiRect r10 = dockIconRectFor(viewW, viewH, 10);
    UiRect r11 = dockIconRectFor(viewW, viewH, 11);
    float sepX = (r10.x + r10.w + r11.x) * 0.5f;
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.26);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, sepX, dk.y + 12.0f);
    cairo_line_to(cr, sepX, dk.y + dk.h - 12.0f);
    cairo_stroke(cr);
    cairo_restore(cr);

    const bool running[kDockItemCount] = {
        showFinder_ || telemetry_.filesRunning,
        showFinder_,
        telemetry_.browserRunning,
        showWidgets_,
        false,
        false,
        false,
        false,
        musicPlaying_ || telemetry_.musicRunning,
        telemetry_.terminalRunning,
        showTuner_,
        false,
    };

    for (int i = 0; i < kDockItemCount; ++i) {
        UiRect ir = dockIconRectFor(viewW, viewH, i);
        drawVectorAppIcon(cr, i, ir.x, ir.y, ir.w, dockStyle_, tmNow.tm_mday);

        float cx = ir.x + ir.w * 0.5f;
        if (running[i]) {
            cairo_save(cr);
            cairo_new_path(cr);
            cairo_set_source_rgba(cr, ink.r, ink.g, ink.b, 0.90);
            cairo_arc(cr, cx, dk.y + dk.h - 4.5f, 1.8f, 0.0, 2.0 * kPi);
            cairo_fill(cr);
            cairo_restore(cr);
        }
    }

    UiRect tip = dockTooltipRectFor(viewW, viewH);
    if (tip.w > 1.0f && hoveredDockIdx_ >= 0) {
        drawText(cr, kDockLabels[hoveredDockIdx_], tip.x, tip.y, tip.w, "Sans Bold 8", ink.r, ink.g, ink.b, 0.96, PANGO_ALIGN_CENTER, true, tip.h, !darkAppearance_);
    }
}

} // namespace lg
