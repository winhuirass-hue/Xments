// demo/main.cpp  —  Xments demo app (~50 lines)
// Build:  g++ -std=c++17 main.cpp -lSDL2 -o demo
#include "../xm/window.hpp"
#include "../xm/widgets.hpp"

int main() {
    xm::Window::Config cfg;
    cfg.title  = "Xments Demo";
    cfg.width  = 640;
    cfg.height = 480;

    xm::Window win(cfg);
    xm::Style  style;   // or xm::dark_style()

    // — App state —
    std::string name;
    bool        check  = false;
    float       volume = 0.4f;
    int         clicks = 0;

    win.run([&](xm::Renderer& r, xm::UI& ui, const xm::InputState& in) {
        ui.set_cursor(40, 40);

        // Title
        ui.label("Xments  —  minimal GUI toolkit", xm::colors::accent);
        ui.separator(560);
        ui.spacing(4);

        // Text field
        ui.label("Your name:");
        ui.text_field(name, 300, "Type here...");
        if (!name.empty()) {
            ui.label("Hello, " + name + "!");
        }
        ui.spacing(8);

        // Buttons
        if (ui.button("Click me"))    ++clicks;
        if (ui.button_outline("Reset")) { clicks=0; name.clear(); }
        ui.label("Clicks: " + std::to_string(clicks), xm::colors::teal);
        ui.spacing(8);

        // Checkbox
        ui.checkbox("Enable feature", check);
        ui.spacing(8);

        // Slider
        ui.label("Volume:");
        ui.slider("vol", volume, 0.f, 1.f, 300);
        ui.label(std::to_string(int(volume * 100)) + " %", xm::colors::gray);
        ui.spacing(12);

        // Draw API used directly (renderer primitives)
        r.fill_rounded_rect({40, 380, 200, 60}, 8, xm::colors::accent);
        r.draw_text(52, 404, "Direct draw API", xm::colors::white, 1);
    }, style);
}
