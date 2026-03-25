// xm/widgets.hpp — Improved Immediate-Mode Widgets
// Unicode-safe text input, stable IDs, corrected hover/active logic
// Fully backward-compatible with original API.

#pragma once
#include "renderer.hpp"
#include "input.hpp"
#include <string_view>
#include <string>
#include <algorithm>

// Simple UTF-8 decoder (no external deps)
namespace xm::utf8 {
    inline uint32_t decode(const char*& p) {
        uint8_t c = (uint8_t)*p++;
        if (c < 0x80) return c;
        if ((c >> 5) == 0x6) {
            uint32_t r = ((c & 0x1F) << 6) | ((uint8_t)*p++ & 0x3F);
            return r;
        }
        if ((c >> 4) == 0xE) {
            uint32_t r = ((c & 0x0F) << 12)
                       | (((uint8_t)*p++ & 0x3F) << 6)
                       | ((uint8_t)*p++ & 0x3F);
            return r;
        }
        if ((c >> 3) == 0x1E) {
            uint32_t r = ((c & 0x07) << 18)
                       | (((uint8_t)*p++ & 0x3F) << 12)
                       | (((uint8_t)*p++ & 0x3F) << 6)
                       | ((uint8_t)*p++ & 0x3F);
            return r;
        }
        return '?';
    }

    inline void backspace_utf8(std::string& s) {
        if (s.empty()) return;
        int i = int(s.size()) - 1;
        while (i > 0 && (uint8_t(s[i]) >> 6) == 0b10)
            --i;
        s.erase(i);
    }
}

namespace xm {

// ───────────────────────────────────────────────────────────────────────────
// Style
// ───────────────────────────────────────────────────────────────────────────

struct Style {
    Color bg          = colors::bg;
    Color surface     = colors::surface;
    Color surface_hov = Color::hex(0xECEAE0);
    Color surface_act = Color::hex(0xDDDBD0);
    Color border      = colors::border;
    Color text        = colors::text;
    Color text_muted  = Color::hex(0x888780);

    Color accent      = colors::accent;
    Color accent_hov  = Color::hex(0x3C3489);
    Color accent_act  = Color::hex(0x2A2466);
    Color accent_text = colors::white;

    int   radius      = 5;
    int   padding     = 8;
    int   font_scale  = 1;
};

inline Style dark_style() {
    Style s;
    s.bg          = colors::bg_dark;
    s.surface     = colors::surface_dark;
    s.surface_hov = Color::hex(0x3A3A38);
    s.surface_act = Color::hex(0x4A4A48);
    s.border      = Color::hex(0x55534F);
    s.text        = colors::text_dark;
    s.text_muted  = Color::hex(0x888780);
    s.accent      = Color::hex(0x7F77DD);
    s.accent_hov  = Color::hex(0xAFA9EC);
    s.accent_act  = Color::hex(0x6B64C6);
    s.accent_text = colors::black;
    return s;
}

// ───────────────────────────────────────────────────────────────────────────
// Stable ID
// ───────────────────────────────────────────────────────────────────────────

using UIID = uint32_t;

inline UIID hash_id(std::string_view s) {
    uint32_t h = 2166136261u;
    for (char c : s) h = (h ^ uint8_t(c)) * 16777619u;
    return h;
}

inline UIID ptr_id(const void* p) {
    return uint32_t(reinterpret_cast<uintptr_t>(p) * 2654435761u);
}

// ───────────────────────────────────────────────────────────────────────────
// UI Context
// ───────────────────────────────────────────────────────────────────────────

class UI {
public:
    UI(Renderer& r, const InputState& in, const Style& s = {})
        : r_(r), in_(in), s_(s) {}

    void set_style(const Style& s) { s_ = s; }

    // Layout
    void set_cursor(int x, int y) { cx_ = x; cy_ = y; }
    void spacing(int px = 8) { cy_ += px; }

    // Fixed same-line (only x moves)
    void same_line(int gap = 8) {
        cx_ += last_w_ + gap;
        cy_ -= last_h_; // keep baseline
    }

    int cursor_x() const { return cx_; }
    int cursor_y() const { return cy_; }

    // ───────────────────────────────────────────────────────────────────
    // Label
    // ───────────────────────────────────────────────────────────────────
    void label(std::string_view text, Color c = colors::transparent) {
        if (c.a == 0) c = s_.text;
        r_.draw_text(cx_, cy_, text, c, s_.font_scale);
        last_w_ = r_.text_width(text, s_.font_scale);
        last_h_ = r_.text_height(s_.font_scale);
        cy_ += last_h_ + s_.padding;
    }

    // ───────────────────────────────────────────────────────────────────
    // Button with correct active/hover handling
    // ───────────────────────────────────────────────────────────────────
    bool button(std::string_view text, int w = 0) {
        int tw = r_.text_width(text, s_.font_scale);
        int th = r_.text_height(s_.font_scale);
        if (w == 0) w = tw + s_.padding * 4;
        int h = th + s_.padding * 2;

        Recti rect{cx_, cy_, w, h};
        UIID id = hash_id(text);

        bool hovered = in_.hovered(rect);

        // IMGUI active logic
        bool pressed = false;

        if (in_.mouse_pressed[0] && hovered)
            active_id_ = id;

        if (!in_.mouse_down[0] && active_id_ == id && hovered)
            pressed = true; // click release inside

        if (!in_.mouse_down[0] && active_id_ == id)
            active_id_ = 0;

        bool active = (active_id_ == id);

        Color bg = active ? s_.accent_act :
                   hovered ? s_.accent_hov :
                             s_.accent;

        r_.fill_rounded_rect(rect, s_.radius, bg);
        r_.draw_text(cx_ + (w - tw) / 2, cy_ + (h - th) / 2, text, s_.accent_text, s_.font_scale);

        last_w_ = w;
        last_h_ = h;
        cy_ += h + s_.padding;

        return pressed;
    }

    // ───────────────────────────────────────────────────────────────────
    // Outline button
    // ───────────────────────────────────────────────────────────────────
    bool button_outline(std::string_view text, int w = 0) {
        int tw = r_.text_width(text, s_.font_scale);
        int th = r_.text_height(s_.font_scale);
        if (w == 0) w = tw + s_.padding * 4;
        int h = th + s_.padding * 2;

        Recti rect{cx_, cy_, w, h};
        bool hovered = in_.hovered(rect);
        bool pressed = in_.clicked_in(rect);

        r_.fill_rounded_rect(rect, s_.radius, hovered ? s_.surface_hov : s_.surface);
        r_.stroke_rect(rect, s_.border);

        r_.draw_text(
            cx_ + (w - tw) / 2,
            cy_ + (h - th) / 2,
            text,
            s_.text,
            s_.font_scale
        );

        last_w_ = w;
        last_h_ = h;
        cy_ += h + s_.padding;
        return pressed;
    }

    // ───────────────────────────────────────────────────────────────────
    // Checkbox (cleaner hitbox, improved mark)
    // ───────────────────────────────────────────────────────────────────
    bool checkbox(std::string_view text, bool& value) {
        int th = r_.text_height(s_.font_scale);
        int box = th + 4;

        Recti rbox{cx_, cy_, box, box};
        Recti full{cx_, cy_, box + s_.padding + r_.text_width(text, s_.font_scale), box};

        bool clicked = in_.clicked_in(full);
        if (clicked) value = !value;

        bool hovered = in_.hovered(full);
        Color bg = value ? s_.accent : hovered ? s_.surface_hov : s_.surface;

        r_.draw_panel(rbox, bg, s_.border);

        // checkmark
        if (value) {
            r_.draw_line(rbox.x + 3, rbox.y + box / 2,
                         rbox.x + box / 2, rbox.y + box - 4,
