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

    // ── Shortcuts ──

    // Register a window-local keyboard shortcut fired before widget key dispatch.
    // Returns an opaque ID for later removal.
    uint32_t add_shortcut(Key key, KeyMods mods, std::function<void()> fn);
    void     remove_shortcut(uint32_t id);

    // ── Mouse capture ──

    // Route all mouse events to w regardless of pointer position (e.g. during drag).
    void capture_mouse(Widget* w);
    void release_capture();
    [[nodiscard]] Widget* mouse_capture() const { return captured_; }

    // ── Cursor ──

    // Apply a cursor shape immediately (called by Widget::set_cursor).
    void apply_cursor(CursorShape shape);

    // ── Tab navigation ──

    // Move keyboard focus to the next/previous widget in tab order.
    void focus_next();
    void focus_prev();

    // ── Signals ──

    Signal<> on_close;
    Signal<SizeI> on_resized;
    Signal<PointI> on_moved;
    Signal<> on_focus_gained;
    Signal<> on_focus_lost;

    // ── Rendering ──

    void repaint_now();

    // Internal: called by Application to process SDL events for this window.
    void process_sdl_event(void* sdl_event);

    [[nodiscard]] uint32_t sdl_window_id()     const;
    [[nodiscard]] void*    native_handle()     const; // SDL_Window*
    [[nodiscard]] void*    native_gl_context() const; // SDL_GLContext
    [[nodiscard]] void*    native_gr_context() const; // GrDirectContext*

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    Widget*  focused_{nullptr};
    Widget*  hovered_{nullptr};
    Widget*  captured_{nullptr};

    // Shortcut table
    struct Shortcut { uint32_t id; Key key; KeyMods mods; std::function<void()> fn; };
    std::vector<Shortcut> shortcuts_;
    uint32_t next_shortcut_id_{1};

    // SDL cursor cache
    void* sdl_cursors_[10]{};  // one per CursorShape
    void  init_cursors();
    void  free_cursors();

    // Tab order helpers
    static void collect_tab_widgets(Widget& root, std::vector<Widget*>& out);
};

} // namespace swole
