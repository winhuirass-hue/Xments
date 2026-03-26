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
        std::string_view title = "Xments";
        int width  = 800;
        int height = 600;
        bool resizable = true;
        bool dark_mode = false;
    };

    explicit Window(const Config& cfg = {})
        : width_(cfg.width), height_(cfg.height)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error(SDL_GetError());

        uint32_t flags = SDL_WINDOW_SHOWN;
        if (cfg.resizable) flags |= SDL_WINDOW_RESIZABLE;

        win_ = SDL_CreateWindow(
            cfg.title.data(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width_, height_, flags);

        if (!win_)
            throw std::runtime_error(SDL_GetError());

        surf_ = SDL_GetWindowSurface(win_);
        fb_.resize(width_, height_);
    }

    ~Window() {
        if (win_) SDL_DestroyWindow(win_);
        SDL_Quit();
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    using FrameFn = std::function<void(Renderer&, UI&, const InputState&)>;

    void run(FrameFn frame, const Style& style = {}) {
        bool running = true;

        while (running) {
            double t = SDL_GetTicks64() / 1000.0;
            input_.begin_frame(t);

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                switch (ev.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        width_  = ev.window.data1;
                        height_ = ev.window.data2;
                        fb_.resize(width_, height_);
                        surf_ = SDL_GetWindowSurface(win_);
                    }
                    break;

                case SDL_MOUSEMOTION:
                    input_.on_mouse_move(ev.motion.x, ev.motion.y);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    input_.on_mouse_down(ev.button.button);
                    break;

                case SDL_MOUSEBUTTONUP:
                    input_.on_mouse_up(ev.button.button);
                    break;

                case SDL_MOUSEWHEEL:
                    input_.on_scroll(ev.wheel.y);
                    break;

                case SDL_KEYDOWN:
                    input_.on_key_down(ev.key.keysym.sym);
                    break;

                case SDL_KEYUP:
                    input_.on_key_up(ev.key.keysym.sym);
                    break;

                case SDL_TEXTINPUT:
                    input_.on_text_input(ev.text.text);
                    break;
                }
            }

            if (!running) break;

            fb_.clear(style.bg);

            Renderer rend(fb_);
            UI ui(rend, input_, style);
            frame(rend, ui, input_);

            SDL_UpdateWindowSurface(win_);
        }
    }

private:
    SDL_Window* win_ = nullptr;
    SDL_Surface* surf_ = nullptr;

    int width_  = 0;
    int height_ = 0;

    Framebuffer fb_;
    InputState input_;
};

} // namespace xm
