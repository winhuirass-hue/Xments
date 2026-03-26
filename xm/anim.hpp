// xm/anim.hpp  —  Lightweight animation engine
// Drives per-widget float transitions used by buttons, lists, menus.
// C++17, no deps.
#pragma once
#include <cmath>
#include <unordered_map>
#include <cstdint>

namespace xm {

// ── Easing functions ──────────────────────────────────────────────────────────

namespace ease {
    inline float linear   (float t) { return t; }
    inline float in_quad  (float t) { return t*t; }
    inline float out_quad (float t) { return t*(2.f-t); }
    inline float in_out   (float t) {
        return t < .5f ? 2.f*t*t : -1.f+(4.f-2.f*t)*t;
    }
    // Exponential — snappy deceleration (great for menus opening)
    inline float out_expo (float t) {
        return t >= 1.f ? 1.f : 1.f - std::pow(2.f, -10.f*t);
    }
    // Overshoot spring (great for list item selection)
    inline float out_back (float t) {
        constexpr float c = 1.70158f, c1 = c + 1.f;
        return 1.f + c1*(t-1.f)*(t-1.f)*(t-1.f) + c*(t-1.f)*(t-1.f);
    }
}

// ── Tween ─────────────────────────────────────────────────────────────────────
// A single animated float.  Call tick() every frame, read value().

struct Tween {
    float from   = 0.f;
    float to     = 0.f;
    float dur    = 0.15f;   // seconds
    float elapsed= 0.f;
    bool  active = false;
    float (*ease_fn)(float) = ease::out_quad;

    void start(float target, float duration = 0.15f,
               float(*fn)(float) = ease::out_quad)
    {
        from    = current();
        to      = target;
        dur     = duration;
        elapsed = 0.f;
        active  = true;
        ease_fn = fn;
    }

    void snap(float v) { from=to=v; elapsed=dur; active=false; }

    // Returns true while still animating
    bool tick(float dt) {
        if (!active) return false;
        elapsed += dt;
        if (elapsed >= dur) { elapsed=dur; active=false; }
        return active;
    }

    float current() const {
        if (!active || dur <= 0.f) return to;
        float t = std::min(elapsed / dur, 1.f);
        return from + (to - from) * ease_fn(t);
    }

    bool done() const { return !active; }
};

// ── AnimPool ──────────────────────────────────────────────────────────────────
// One pool per UI context.  Key = widget ID + channel index.
// Automatically creates Tweens on first access.

class AnimPool {
public:
    // Retrieve or create a tween keyed by (widget_id, channel)
    Tween& get(uint32_t id, int channel = 0) {
        return pool_[(uint64_t(id) << 8) | uint8_t(channel)];
    }

    // Tick all active tweens; dt = delta seconds
    void tick_all(float dt) {
        for (auto& [k, tw] : pool_) tw.tick(dt);
    }

    // Convenience: drive a 0→1 hover tween (call every frame)
    // Returns current animated value
    float hover(uint32_t id, bool hovered, float dt,
                float dur_in=0.08f, float dur_out=0.12f)
    {
        auto& tw = get(id, 0);
        float target = hovered ? 1.f : 0.f;
        if (tw.to != target)
            tw.start(target, hovered ? dur_in : dur_out, ease::out_quad);
        tw.tick(dt);
        return tw.current();
    }

    // Drive a 0→1 press/active tween
    float press(uint32_t id, bool active, float dt, float dur=0.06f)
    {
        auto& tw = get(id, 1);
        float target = active ? 1.f : 0.f;
        if (tw.to != target) tw.start(target, dur, ease::in_out);
        tw.tick(dt);
        return tw.current();
    }

    // Drive open/close (0→1) for menus/dropdowns
    float open(uint32_t id, bool is_open, float dt, float dur=0.14f)
    {
        auto& tw = get(id, 2);
        float target = is_open ? 1.f : 0.f;
        if (tw.to != target)
            tw.start(target, dur, is_open ? ease::out_expo : ease::in_out);
        tw.tick(dt);
        return tw.current();
    }

    // Selection slide: animates a float toward a target row index position
    float select(uint32_t id, float target_y, float dt, float dur=0.18f)
    {
        auto& tw = get(id, 3);
        if (tw.to != target_y)
            tw.start(target_y, dur, ease::out_back);
        tw.tick(dt);
        return tw.current();
    }

private:
    std::unordered_map<uint64_t, Tween> pool_;
};

// ── Color lerp (for animated color transitions) ───────────────────────────────

inline Color lerp_color(Color a, Color b, float t) {
    t = std::min(std::max(t, 0.f), 1.f);
    return {
        uint8_t(a.r + (b.r - a.r) * t),
        uint8_t(a.g + (b.g - a.g) * t),
        uint8_t(a.b + (b.b - a.b) * t),
        uint8_t(a.a + (b.a - a.a) * t),
    };
}

} // namespace xm