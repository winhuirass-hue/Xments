// xm/widgets.hpp  —  Immediate-mode widgets
// All widgets return bool (true = interacted this frame).
#pragma once
#include "renderer.hpp"
#include "input.hpp"
#include <string_view>
#include <string>
#include <algorithm>

namespace xm {

// ── UI style ─────────────────────────────────────────────────────────────────

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
    Color accent_text = colors::white;

    int   radius      = 5;     // corner radius
    int   padding     = 8;     // inner padding
    int   font_scale  = 1;
};

inline Style dark_style() {
    return Style{
        .bg          = colors::bg_dark,
        .surface     = colors::surface_dark,
        .surface_hov = Color::hex(0x3A3A38),
        .surface_act = Color::hex(0x4A4A48),
        .border      = Color::hex(0x55534F),
        .text        = colors::text_dark,
        .text_muted  = Color::hex(0x888780),
        .accent      = Color::hex(0x7F77DD),
        .accent_hov  = Color::hex(0xAFA9EC),
        .accent_text = Color::hex(0x1A1A18),
        .radius      = 5,
        .padding     = 8,
        .font_scale  = 1,
    };
}

// ── Context ───────────────────────────────────────────────────────────────────

class UI {
public:
    UI(Renderer& r, const InputState& in, const Style& s = {})
        : r_(r), in_(in), s_(s) {}

    void set_style(const Style& s) { s_ = s; }

    // ── Layout cursor ──────────────────────────────────────────────────────
    void set_cursor(int x, int y)  { cx_=x; cy_=y; }
    void same_line (int gap=8)     { cy_-=last_h_+gap; cx_+=last_w_+gap; }
    void spacing   (int px=8)      { cy_+=px; }
    [[nodiscard]] int cursor_x() const { return cx_; }
    [[nodiscard]] int cursor_y() const { return cy_; }

    // ── Label ──────────────────────────────────────────────────────────────
    void label(std::string_view text, Color c = {0,0,0,0}) {
        if (c.a == 0) c = s_.text;
        r_.draw_text(cx_, cy_, text, c, s_.font_scale);
        last_w_ = r_.text_width(text,  s_.font_scale);
        last_h_ = r_.text_height(s_.font_scale);
        cy_ += last_h_ + s_.padding;
    }

    // ── Button ─────────────────────────────────────────────────────────────
    // Returns true the frame the button is clicked.
    bool button(std::string_view text, int w = 0) {
        int tw = r_.text_width(text, s_.font_scale);
        int th = r_.text_height(s_.font_scale);
        if (w == 0) w = tw + s_.padding * 4;
        int h = th + s_.padding * 2;
        Recti rect{cx_, cy_, w, h};

        bool hov = in_.hovered(rect);
        bool act = hov && in_.mouse_down[0];
        bool clk = in_.clicked_in(rect);

        Color bg  = act ? s_.accent_hov
                  : hov ? s_.accent
                        : s_.accent;
        Color fg  = s_.accent_text;

        r_.fill_rounded_rect(rect, s_.radius, bg);
        r_.draw_text(cx_ + (w-tw)/2, cy_ + (h-th)/2, text, fg, s_.font_scale);

        last_w_ = w; last_h_ = h;
        cy_ += h + s_.padding;
        return clk;
    }

    // ── Ghost button (outlined) ────────────────────────────────────────────
    bool button_outline(std::string_view text, int w = 0) {
        int tw = r_.text_width(text, s_.font_scale);
        int th = r_.text_height(s_.font_scale);
        if (w == 0) w = tw + s_.padding * 4;
        int h = th + s_.padding * 2;
        Recti rect{cx_, cy_, w, h};

        bool hov = in_.hovered(rect);
        bool clk = in_.clicked_in(rect);

        r_.fill_rounded_rect(rect, s_.radius, hov ? s_.surface_hov : s_.surface);
        r_.stroke_rect(rect, s_.border);
        r_.draw_text(cx_+(w-tw)/2, cy_+(h-th)/2, text, s_.text, s_.font_scale);

        last_w_ = w; last_h_ = h;
        cy_ += h + s_.padding;
        return clk;
    }

    // ── Checkbox ───────────────────────────────────────────────────────────
    // Pass a bool by reference; toggled on click.  Returns true on toggle.
    bool checkbox(std::string_view text, bool& value) {
        int th  = r_.text_height(s_.font_scale);
        int box = th + 4;
        Recti box_r{cx_, cy_, box, box};
        Recti full_r{cx_, cy_, box + s_.padding + r_.text_width(text,s_.font_scale), box};

        bool clk = in_.clicked_in(full_r);
        if (clk) value = !value;

        bool hov = in_.hovered(full_r);
        r_.draw_panel(box_r, value ? s_.accent : (hov ? s_.surface_hov : s_.surface), s_.border);
        if (value) {
            // Draw checkmark
            r_.draw_line(box_r.x+2, box_r.y+box/2, box_r.x+box/2-1, box_r.y+box-3, s_.accent_text, 2);
            r_.draw_line(box_r.x+box/2-1,box_r.y+box-3, box_r.x+box-2, box_r.y+2,  s_.accent_text, 2);
        }
        r_.draw_text(cx_+box+s_.padding, cy_+(box-th)/2, text, s_.text, s_.font_scale);

        last_w_ = full_r.w; last_h_ = box;
        cy_ += box + s_.padding;
        return clk;
    }

    // ── Slider ─────────────────────────────────────────────────────────────
    // Returns true while dragging.
    bool slider(std::string_view id, float& value, float lo=0.f, float hi=1.f, int w=200) {
        (void)id;
        int th  = r_.text_height(s_.font_scale);
        int h   = 6;
        int ky  = cy_ + th/2 - h/2;   // track vertical centre
        Recti track{cx_, ky, w, h};

        bool hov  = in_.hovered({cx_, cy_-4, w, th+8});
        bool drag = hov && in_.mouse_down[0];
        bool clk  = in_.clicked_in({cx_, cy_-6, w, th+12});

        if (drag || clk) {
            float t = float(in_.mouse_x - cx_) / float(w);
            value = std::clamp(lo + t*(hi-lo), lo, hi);
        }

        float t    = (value-lo)/(hi-lo);
        int   knob = cx_ + int(t * w);

        r_.fill_rounded_rect(track, 3, s_.border);
        r_.fill_rounded_rect({track.x, track.y, int(t*w), h}, 3, s_.accent);
        r_.fill_circle(knob, ky+h/2, 6, hov ? s_.accent_hov : s_.accent);
        r_.fill_circle(knob, ky+h/2, 3, s_.accent_text);

        last_w_ = w; last_h_ = th;
        cy_ += th + s_.padding + 4;
        return drag;
    }

    // ── Text input ─────────────────────────────────────────────────────────
    bool text_field(std::string& buf, int w=200, std::string_view placeholder={}) {
        int th  = r_.text_height(s_.font_scale);
        int h   = th + s_.padding * 2;
        Recti rect{cx_, cy_, w, h};

        bool focused = focused_id_ == &buf;
        if (in_.clicked_in(rect))   { focused_id_ = &buf;  focused = true; }
        if (in_.clicked_in({0,0,2000,2000}) && !in_.clicked_in(rect))
            if (focused) { focused_id_ = nullptr; focused = false; }

        if (focused) {
            buf += in_.text_input;
            if (in_.pressed(Key::Backspace) && !buf.empty())
                buf.pop_back();
        }

        r_.draw_panel(rect, focused ? s_.surface_hov : s_.surface, focused ? s_.accent : s_.border);

        std::string_view display = buf.empty() ? placeholder : std::string_view(buf);
        Color tc = buf.empty() ? s_.text_muted : s_.text;
        r_.draw_text(cx_+s_.padding, cy_+(h-th)/2, display, tc, s_.font_scale);

        // Cursor blink
        if (focused && int(in_.frame_time * 2) % 2 == 0) {
            int cx = cx_+s_.padding + r_.text_width(buf, s_.font_scale);
            r_.draw_line(cx, cy_+s_.padding, cx, cy_+h-s_.padding, s_.text, 1);
        }

        last_w_ = w; last_h_ = h;
        cy_ += h + s_.padding;
        return focused;
    }

    // ── Separator ──────────────────────────────────────────────────────────
    void separator(int w=0) {
        if (w==0) w = 400;
        r_.draw_line(cx_, cy_+4, cx_+w, cy_+4, s_.border, 1);
        cy_ += 12;
    }

private:
    Renderer&         r_;
    const InputState& in_;
    Style             s_;
    int  cx_=0, cy_=0;
    int  last_w_=0, last_h_=0;
    void* focused_id_ = nullptr;
};

} // namespace xm
