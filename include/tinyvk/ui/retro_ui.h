/**
 * @file retro_ui.h
 * @brief 8-bit RPG style UI controls for TinyVK
 * 
 * This provides a high-level UI API that wraps ImGui with custom 8-bit retro rendering.
 * ImGui is an implementation detail - users interact with this API only.
 */

#pragma once

#include "../core/types.h"
#include "retro_theme.h"

namespace tvk {

struct RetroRect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;

    RetroRect() : x(0), y(0), w(0), h(0) {}
    RetroRect(f32 x_, f32 y_, f32 w_, f32 h_) : x(x_), y(y_), w(w_), h(h_) {}

    Vec2 min() const { return Vec2(x, y); }
    Vec2 max() const { return Vec2(x + w, y + h); }
    Vec2 center() const { return Vec2(x + w * 0.5f, y + h * 0.5f); }
    Vec2 size() const { return Vec2(w, h); }
    bool contains(const Vec2& p) const { return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h; }
};

enum class RetroTextAlign {
    Left,
    Center,
    Right
};

enum class RetroWindowFlags {
    None = 0,
    NoTitleBar = 1 << 0,
    NoResize = 1 << 1,
    NoMove = 1 << 2,
    NoScrollbar = 1 << 3,
    NoCollapse = 1 << 4,
    NoBorder = 1 << 5,
    NoBackground = 1 << 6,
    AlwaysAutoResize = 1 << 7,
    Modal = 1 << 8
};

inline RetroWindowFlags operator|(RetroWindowFlags a, RetroWindowFlags b) {
    return static_cast<RetroWindowFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(RetroWindowFlags a, RetroWindowFlags b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

enum class RetroButtonStyle {
    Normal,
    Primary,
    Secondary,
    Danger,
    Success,
    Ghost
};

enum class RetroIconType {
    None,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowDown,
    Check,
    Cross,
    Plus,
    Minus,
    Menu,
    Settings,
    Search,
    Refresh,
    Save,
    Load,
    Folder,
    File,
    Edit,
    Delete,
    Copy,
    Paste,
    Undo,
    Redo,
    Play,
    Pause,
    Stop,
    Heart,
    Star,
    Coin,
    Sword,
    Shield,
    Potion,
    Key,
    Chest,
    Skull,
    Crown
};

class RetroUI {
public:
    static void init();
    static void shutdown();

    static bool begin_window(const char* title, bool* open = nullptr, RetroWindowFlags flags = RetroWindowFlags::None);
    static void end_window();

    static bool begin_child(const char* id, const Vec2& size = Vec2(0, 0), bool border = true);
    static void end_child();

    static void begin_group();
    static void end_group();

    static void separator();
    static void spacing();
    static void same_line(f32 offset = 0.0f, f32 spacing = -1.0f);
    static void new_line();
    static void indent(f32 width = 0.0f);
    static void unindent(f32 width = 0.0f);

    static void text(const char* text);
    static void text_colored(const char* text, const RetroColor& color);
    static void text_wrapped(const char* text);
    static void label(const char* label, const char* text);

    static bool button(const char* label, const Vec2& size = Vec2(0, 0), RetroButtonStyle style = RetroButtonStyle::Normal);
    static bool icon_button(RetroIconType icon, const Vec2& size = Vec2(24, 24), RetroButtonStyle style = RetroButtonStyle::Normal);
    static bool image_button(const char* id, u64 texture_id, const Vec2& size);

    static bool checkbox(const char* label, bool* value);
    static bool radio_button(const char* label, bool active);
    static bool radio_button(const char* label, int* value, int button_value);

    static bool slider_int(const char* label, i32* value, i32 min_val, i32 max_val);
    static bool slider_float(const char* label, f32* value, f32 min_val, f32 max_val);
    static bool progress_bar(f32 fraction, const Vec2& size = Vec2(-1, 0), const char* overlay = nullptr);

    static bool input_text(const char* label, char* buf, size_t buf_size);
    static bool input_text_multiline(const char* label, char* buf, size_t buf_size, const Vec2& size = Vec2(0, 0));
    static bool input_int(const char* label, i32* value);
    static bool input_float(const char* label, f32* value);

    static bool combo(const char* label, i32* current_item, const char* const* items, i32 item_count);
    static bool begin_combo(const char* label, const char* preview);
    static void end_combo();

    static bool selectable(const char* label, bool selected, const Vec2& size = Vec2(0, 0));
    static bool selectable(const char* label, bool* selected, const Vec2& size = Vec2(0, 0));

    static bool begin_listbox(const char* label, const Vec2& size = Vec2(0, 0));
    static void end_listbox();

    static bool begin_menu_bar();
    static void end_menu_bar();
    static bool begin_menu(const char* label);
    static void end_menu();
    static bool menu_item(const char* label, const char* shortcut = nullptr, bool selected = false);
    static bool menu_item(const char* label, const char* shortcut, bool* selected);

    static bool tree_node(const char* label);
    static bool tree_node_ex(const char* label, bool default_open = false, bool leaf = false);
    static void tree_pop();

    static bool collapsing_header(const char* label, bool default_open = false);

    static bool begin_tab_bar(const char* id);
    static void end_tab_bar();
    static bool begin_tab_item(const char* label, bool* open = nullptr);
    static void end_tab_item();

    static bool begin_table(const char* id, i32 columns);
    static void end_table();
    static void table_next_row();
    static void table_next_column();
    static void table_set_column_index(i32 index);
    static void table_headers_row();
    static void table_setup_column(const char* label, f32 width = 0.0f);

    static bool begin_popup(const char* id);
    static bool begin_popup_modal(const char* name, bool* open = nullptr);
    static void end_popup();
    static void open_popup(const char* id);
    static void close_popup();
    static bool begin_popup_context_item(const char* id = nullptr);
    static bool begin_popup_context_window(const char* id = nullptr);

    static bool begin_tooltip();
    static void end_tooltip();
    static void set_tooltip(const char* text);

    static void draw_rect(const RetroRect& rect, const RetroColor& color, f32 border_thickness = 0.0f);
    static void draw_rect_filled(const RetroRect& rect, const RetroColor& color);
    static void draw_rect_border(const RetroRect& rect, const RetroColor& color, f32 thickness = 2.0f);
    static void draw_line(const Vec2& a, const Vec2& b, const RetroColor& color, f32 thickness = 1.0f);
    static void draw_text(const Vec2& pos, const char* text, const RetroColor& color, RetroTextAlign align = RetroTextAlign::Left);
    static void draw_icon(const Vec2& pos, RetroIconType icon, const RetroColor& color, f32 size = 16.0f);

    static void draw_rpg_frame(const RetroRect& rect, const RetroColor& bg, const RetroColor& border_light, const RetroColor& border_dark);
    static void draw_rpg_button(const RetroRect& rect, const RetroColor& face, bool pressed = false);
    static void draw_scanlines(const RetroRect& rect, f32 opacity = 0.1f);
    static void draw_pixel_grid(const RetroRect& rect, f32 grid_size, const RetroColor& color);

    static bool is_item_hovered();
    static bool is_item_active();
    static bool is_item_clicked(i32 button = 0);
    static bool is_item_focused();
    static Vec2 get_item_rect_min();
    static Vec2 get_item_rect_max();
    static Vec2 get_item_rect_size();

    static Vec2 get_cursor_pos();
    static void set_cursor_pos(const Vec2& pos);
    static Vec2 get_content_region_avail();
    static Vec2 get_window_pos();
    static Vec2 get_window_size();

    static void set_next_item_width(f32 width);
    static void push_item_width(f32 width);
    static void pop_item_width();

    static void push_id(const char* id);
    static void push_id(i32 id);
    static void pop_id();

    static void push_style_color(const RetroColor& color);
    static void pop_style_color(i32 count = 1);

    static u32 get_id(const char* str);

    static bool is_window_focused();
    static bool is_window_hovered();

    static f32 get_scroll_x();
    static f32 get_scroll_y();
    static f32 get_scroll_max_x();
    static f32 get_scroll_max_y();
    static void set_scroll_x(f32 scroll);
    static void set_scroll_y(f32 scroll);
    static void set_scroll_here_x(f32 center_ratio = 0.5f);
    static void set_scroll_here_y(f32 center_ratio = 0.5f);

    static void set_keyboard_focus_here(i32 offset = 0);

    static bool is_key_pressed(i32 key);
    static bool is_key_down(i32 key);
    static bool is_key_released(i32 key);

    static bool is_mouse_clicked(i32 button);
    static bool is_mouse_down(i32 button);
    static bool is_mouse_released(i32 button);
    static bool is_mouse_double_clicked(i32 button);
    static Vec2 get_mouse_pos();
    static Vec2 get_mouse_drag_delta(i32 button = 0, f32 lock_threshold = -1.0f);
    static void reset_mouse_drag_delta(i32 button = 0);

private:
    static void draw_icon_internal(const Vec2& pos, RetroIconType icon, const RetroColor& color, f32 size);
};

} // namespace tvk
