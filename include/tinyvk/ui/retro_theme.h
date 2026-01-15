/**
 * @file retro_theme.h
 * @brief 8-bit RPG style theme for TinyVK UI
 */

#pragma once

#include "../core/types.h"

namespace tvk {

struct RetroColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    constexpr RetroColor() : r(0), g(0), b(0), a(255) {}
    constexpr RetroColor(u8 r_, u8 g_, u8 b_, u8 a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

    f32 rf() const { return r / 255.0f; }
    f32 gf() const { return g / 255.0f; }
    f32 bf() const { return b / 255.0f; }
    f32 af() const { return a / 255.0f; }

    u32 to_u32() const { return (a << 24) | (b << 16) | (g << 8) | r; }

    RetroColor darken(u8 amount) const {
        return RetroColor(
            r > amount ? r - amount : 0,
            g > amount ? g - amount : 0,
            b > amount ? b - amount : 0,
            a
        );
    }

    RetroColor lighten(u8 amount) const {
        return RetroColor(
            r + amount < 255 ? r + amount : 255,
            g + amount < 255 ? g + amount : 255,
            b + amount < 255 ? b + amount : 255,
            a
        );
    }
};

struct RetroPalette {
    RetroColor bg_dark;
    RetroColor bg_medium;
    RetroColor bg_light;
    RetroColor border_dark;
    RetroColor border_light;
    RetroColor text_primary;
    RetroColor text_secondary;
    RetroColor text_disabled;
    RetroColor accent_primary;
    RetroColor accent_secondary;
    RetroColor accent_highlight;
    RetroColor button_normal;
    RetroColor button_hover;
    RetroColor button_pressed;
    RetroColor input_bg;
    RetroColor input_border;
    RetroColor selection;
    RetroColor scrollbar_bg;
    RetroColor scrollbar_thumb;
    RetroColor shadow;

    static RetroPalette default_dark();
    static RetroPalette default_light();
    static RetroPalette gameboy();
    static RetroPalette nes();
    static RetroPalette snes();
    static RetroPalette c64();
};

enum class RetroFontStyle {
    Normal,
    Bold,
    Pixel
};

struct RetroThemeConfig {
    RetroPalette palette;
    f32 border_thickness = 2.0f;
    f32 corner_segments = 0;
    f32 shadow_offset = 2.0f;
    f32 button_padding_x = 12.0f;
    f32 button_padding_y = 6.0f;
    f32 window_padding = 8.0f;
    f32 item_spacing = 4.0f;
    f32 scrollbar_width = 12.0f;
    f32 font_size = 16.0f;
    bool pixel_perfect = true;
    bool draw_shadows = true;
    bool draw_borders = true;
    bool scanlines = false;
    f32 scanline_opacity = 0.1f;
};

class RetroTheme {
public:
    static RetroTheme& get();

    void set_config(const RetroThemeConfig& config);
    const RetroThemeConfig& get_config() const { return _config; }
    RetroThemeConfig& get_config_mut() { return _config; }

    void set_palette(const RetroPalette& palette);
    const RetroPalette& get_palette() const { return _config.palette; }

    void apply();

    void push_color(const RetroColor& color);
    void pop_color();

    f32 snap_to_pixel(f32 value) const;
    Vec2 snap_to_pixel(const Vec2& value) const;

private:
    RetroTheme();

    RetroThemeConfig _config;
    bool _applied = false;
};

namespace retro_colors {
    constexpr RetroColor black(0, 0, 0);
    constexpr RetroColor white(255, 255, 255);
    constexpr RetroColor dark_gray(40, 40, 40);
    constexpr RetroColor gray(128, 128, 128);
    constexpr RetroColor light_gray(192, 192, 192);

    constexpr RetroColor dark_blue(32, 32, 64);
    constexpr RetroColor blue(64, 64, 192);
    constexpr RetroColor light_blue(128, 160, 255);
    constexpr RetroColor cyan(64, 224, 208);

    constexpr RetroColor dark_green(32, 64, 32);
    constexpr RetroColor green(64, 192, 64);
    constexpr RetroColor light_green(144, 238, 144);

    constexpr RetroColor dark_red(64, 32, 32);
    constexpr RetroColor red(192, 64, 64);
    constexpr RetroColor light_red(255, 128, 128);

    constexpr RetroColor dark_purple(64, 32, 64);
    constexpr RetroColor purple(128, 64, 192);
    constexpr RetroColor light_purple(192, 128, 255);

    constexpr RetroColor dark_yellow(64, 64, 32);
    constexpr RetroColor yellow(224, 192, 64);
    constexpr RetroColor gold(255, 215, 0);

    constexpr RetroColor dark_brown(48, 32, 16);
    constexpr RetroColor brown(139, 90, 43);
    constexpr RetroColor tan(210, 180, 140);

    constexpr RetroColor orange(255, 140, 0);
    constexpr RetroColor pink(255, 182, 193);
    constexpr RetroColor magenta(255, 0, 255);
}

} // namespace tvk
