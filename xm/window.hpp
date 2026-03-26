// xm/window.hpp  —  Platform window (SDL2 backend, swappable)
// Swap SDL2 for Win32/X11 by implementing the same interface.
#pragma once
#include "renderer.hpp"
#include "input.hpp"
#include <string_view>
#include <functional>
#include <stdexcept>
#include <SDL2/SDL.h>

namespace xm {

class Window {
public:
    struct Config {
        std::string_view title  = "Xments";
        int  width  = 800;
        int  height = 600;
        bool resizable = true;
        bool dark_mode = false;
    };

    explicit Window(const Config& cfg = {})
        : width_(cfg.width), height_(cfg.height)
    {
       
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(SDL_GetError());

    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (!cfg.resizable) flags &= ~SDL_WINDOW_RESIZABLE;
    else flags |= SDL_WINDOW_RESIZABLE;

    win_ = SDL_CreateWindow(
        cfg.title.data(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg.width, cfg.height,
        flags
    );
    if (!win_) throw std::runtime_error(SDL_GetError());

    // Читання HiDPI розміру
    get_drawable_size(win_, width_, height_);

    surf_ = SDL_GetWindowSurface(win_);
    fb_.resize(width_, height_);

    }

    ~Window() {
        if (win_) SDL_DestroyWindow(win_);
        SDL_Quit();
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // ── Event loop ─────────────────────────────────────────────────────────
    // Call run() with your frame lambda.  Returns when window closes.
    using FrameFn = std::function<void(Renderer&, UI&, const InputState&)>;

    void run(FrameFn frame, const Style& style = {}) {
        bool running = true;
        while (running) {
            double t = SDL_GetTicks64() / 1000.0;
            input_.begin_frame(t);

            // ── Pump events ──────────────────────────────────────────────
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                switch (ev.type) {
                case SDL_QUIT: running=false; break;
                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        SDL_GetWindowSize(win_, &win_logical_w, &win_logical_h)
                        get_drawable_size(win_, width_, height_);
                        
                        fb_.resize(width_, height_);
                        surf_ = SDL_GetWindowSurface(win_);
                    }
                  } break;
                case SDL_MOUSEMOTION:
                float scale_x = float(fb_w) / float(win_logical_w);
                float scale_y = float(fb_h) / float(win_logical_h);
                    
                input_.on_mouse_move(
                    int(ev.motion.x * scale_x),
                    int(ev.motion.y * scale_y)
                );               
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    input_.on_mouse_down(sdl_button(ev.button.button));
                    break;
                case SDL_MOUSEBUTTONUP:
                    input_.on_mouse_up(sdl_button(ev.button.button));
                    break;
                case SDL_MOUSEWHEEL:
                    input_.on_scroll(ev.wheel.y);
                    break;
                case SDL_KEYDOWN:
                    input_.on_key_down(sdl_key(ev.key.keysym.sym));
                    break;
                case SDL_KEYUP:
                    input_.on_key_up(sdl_key(ev.key.keysym.sym));
                    break;
                case SDL_TEXTINPUT:
                    input_.on_text_input(ev.text.text);
                    break;
                }
            }
            if (!running) break;

            // ── Render frame ─────────────────────────────────────────────
            fb_.clear(style.bg);
            Renderer rend(fb_);
            UI       ui(rend, input_, style);
            frame(rend, ui, input_);

            // ── Blit framebuffer → window surface ────────────────────────
            SDL_LockSurface(surf_);
            const uint32_t* src = fb_.data();
            uint8_t*        dst = static_cast<uint8_t*>(surf_->pixels);
            int pitch = surf_->pitch;
            for (int y=0; y<height_; ++y) {
                uint32_t* row = reinterpret_cast<uint32_t*>(dst + y*pitch);
                for (int x=0; x<width_; ++x) {
                    uint32_t px = src[y*width_+x];
                    uint8_t r=(px>>16)&0xFF, g=(px>>8)&0xFF, b=px&0xFF;
                    row[x] = SDL_MapRGB(surf_->format, r, g, b);
                }
            }
            SDL_UnlockSurface(surf_);
            SDL_UpdateWindowSurface(win_);

            // ~60 fps cap
            SDL_Delay(16);
        }
    }

    [[nodiscard]] int width()  const { return width_;  }
    [[nodiscard]] int height() const { return height_; }

private:
    SDL_Window*   win_  = nullptr;
    SDL_Surface*  surf_ = nullptr;
    Framebuffer   fb_;
    InputState    input_;
    int           width_, height_;

    static MouseButton sdl_button(uint8_t b) {
        switch (b) {
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        default:                return MouseButton::Left;
        }
    }

    static Key sdl_key(SDL_Keycode k) {
        switch (k) {
        case SDLK_ESCAPE:    return Key::Escape;
        case SDLK_RETURN:    return Key::Enter;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_TAB:       return Key::Tab;
        case SDLK_LEFT:      return Key::Left;
        case SDLK_RIGHT:     return Key::Right;
        case SDLK_UP:        return Key::Up;
        case SDLK_DOWN:      return Key::Down;
        case SDLK_DELETE:    return Key::Delete;
        default:
            if (k >= 32 && k < 127) return Key(k);
            return Key::None;
        }
    }
};

} // namespace xm
