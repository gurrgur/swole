#pragma once

#include "widget.hpp"
#include <functional>
#include <memory>
#include <string>

namespace swole {

enum class WindowFlags : uint32_t {
    None          = 0,
    Resizable     = 1 << 0,
    Borderless    = 1 << 1,
    AlwaysOnTop   = 1 << 2,
    Transparent   = 1 << 3,
    NoTaskbar     = 1 << 4,
    FullScreen    = 1 << 5,
    HighDPI       = 1 << 6,
    VSync         = 1 << 7,
};
inline constexpr WindowFlags operator|(WindowFlags a, WindowFlags b) {
    return WindowFlags(uint32_t(a) | uint32_t(b));
}
inline constexpr WindowFlags operator&(WindowFlags a, WindowFlags b) {
    return WindowFlags(uint32_t(a) & uint32_t(b));
}
inline constexpr bool has_flag(WindowFlags set, WindowFlags flag) {
    return (uint32_t(set) & uint32_t(flag)) != 0;
}

struct WindowConfig {
    std::string  title;
    SizeI        size{800, 600};
    PointI       position{-1, -1};   // -1 = platform default / centered
    WindowFlags  flags{WindowFlags::Resizable | WindowFlags::HighDPI | WindowFlags::VSync};
    int          min_width{0}, min_height{0};
    int          max_width{0}, max_height{0}; // 0 = no limit
};

// Top-level window backed by an SDL3 window + Skia GPU surface.
// Owns the root widget and the rendering context.
class Window {
public:
    explicit Window(WindowConfig cfg = {});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // ── Visibility & state ──

    void show();
    void hide();
    void close();
    void raise(); // bring to front

    [[nodiscard]] bool is_visible() const;
    [[nodiscard]] bool is_closed()  const;

    // ── Geometry ──

    void set_title(std::string_view title);
    void set_size(SizeI sz);
    void set_position(PointI pos);
    void set_min_size(SizeI sz);
    void set_max_size(SizeI sz);
    void center_on_screen();
    void enter_fullscreen();
    void exit_fullscreen();

    [[nodiscard]] std::string title()    const;
    [[nodiscard]] SizeI       size()     const;
    [[nodiscard]] PointI      position() const;
    [[nodiscard]] float       dpi_scale() const; // e.g. 2.0 on HiDPI

    // ── Root widget ──

    // The root widget always matches the window client area.
    [[nodiscard]] Widget& root();
    [[nodiscard]] const Widget& root() const;

    // Convenience: set layout on root widget.
    void set_layout(std::unique_ptr<Layout> layout);

    // ── Focus ──

    [[nodiscard]] Widget* focused_widget() const { return focused_; }
    void set_focused_widget(Widget* w);

    // ── Signals ──

    Signal<> on_close;          // emitted before the window actually closes
    Signal<SizeI> on_resized;
    Signal<PointI> on_moved;
    Signal<> on_focus_gained;
    Signal<> on_focus_lost;

    // ── Rendering ──

    // Force an immediate repaint (bypasses invalidation coalescing).
    void repaint_now();

    // Internal: called by Application to process SDL events for this window.
    void process_sdl_event(void* sdl_event);

    // Internal: returns the SDL window ID.
    [[nodiscard]] uint32_t sdl_window_id() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    Widget* focused_{nullptr};
    Widget* hovered_{nullptr};
};

} // namespace swole
