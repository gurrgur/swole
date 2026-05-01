#include "swole/window/window.hpp"

#include <SDL3/SDL.h>
// Skia GPU surface
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"

#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "../theme.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace swole {

struct Window::Impl {
    SDL_Window*  sdl_window{nullptr};
    SDL_GLContext gl_ctx{nullptr};
    sk_sp<GrDirectContext> gr_ctx;
    sk_sp<SkSurface>       sk_surface;

    std::unique_ptr<Widget> root;
    WindowConfig            config;
    Canvas                  canvas;

    bool closed{false};

    // ── Tooltip state ──
    std::string   tooltip_text;
    PointI        tooltip_pos{-1, -1};  // screen-local position where tooltip is shown
    Uint64        tooltip_arm_ms{0};    // SDL_GetTicks() when hover started (0 = not armed)
    bool          tooltip_visible{false};
    static constexpr Uint64 kTooltipDelayMs = 600;

    void arm_tooltip(const std::string& text, PointI pos) {
        if (tooltip_text == text && tooltip_arm_ms != 0) return;
        tooltip_text    = text;
        tooltip_pos     = pos;
        tooltip_arm_ms  = SDL_GetTicks();
        tooltip_visible = false;
    }

    void disarm_tooltip() {
        tooltip_arm_ms  = 0;
        tooltip_visible = false;
        tooltip_text.clear();
    }

    void tick_tooltip(Canvas& c) {
        if (!tooltip_visible && tooltip_arm_ms != 0) {
            if (SDL_GetTicks() - tooltip_arm_ms >= kTooltipDelayMs)
                tooltip_visible = true;
        }
        if (!tooltip_visible || tooltip_text.empty()) return;
        draw_tooltip(c);
    }

    void draw_tooltip(Canvas& c) {
        Font  font = theme::default_font(12.f);
        auto  m    = font.metrics();
        float tw   = font.measure_text_width(tooltip_text);
        float fh   = m.ascent + m.descent;
        const float padx = 6.f, pady = 4.f;
        float w = tw + padx*2, h = fh + pady*2;

        // Clamp inside window
        int sw = 0, sh = 0;
        SDL_GetWindowSize(sdl_window, &sw, &sh);
        float x = float(tooltip_pos.x) + 12.f;
        float y = float(tooltip_pos.y) + 20.f;
        if (x + w > float(sw)) x = float(sw) - w - 2.f;
        if (y + h > float(sh)) y = float(tooltip_pos.y) - h - 4.f;

        RectF r{x, y, w, h};
        c.draw_round_rect(r.inset(.5f), 3.f, 3.f, Paint::fill({255,255,220,245}));
        c.draw_round_rect(r.inset(.5f), 3.f, 3.f, Paint::stroke({160,160,100}));
        c.draw_text(tooltip_text, x+padx, y+pady+m.ascent, font, Paint::fill(theme::text));
    }

    bool init_skia() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        gl_ctx = SDL_GL_CreateContext(sdl_window);
        if (!gl_ctx) return false;

        SDL_GL_MakeCurrent(sdl_window, gl_ctx);
        SDL_GL_SetSwapInterval(1); // VSync on

        auto interface = GrGLMakeNativeInterface();
        if (!interface) return false;

        gr_ctx = GrDirectContexts::MakeGL(std::move(interface));
        if (!gr_ctx) return false;

        return create_surface();
    }

    bool create_surface() {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(sdl_window, &w, &h);

        GrGLFramebufferInfo fb_info{};
        fb_info.fFBOID  = 0; // default framebuffer
        fb_info.fFormat = 0x8058; // GL_RGBA8

        auto target = GrBackendRenderTarget(w, h, 0, 8, fb_info);

        SkSurfaceProps props;
        sk_surface = SkSurfaces::WrapBackendRenderTarget(
            gr_ctx.get(), target, kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), &props);

        return sk_surface != nullptr;
    }

    void resize_surface() {
        if (gr_ctx) {
            gr_ctx->resetContext();
            create_surface();
        }
    }

    void paint_frame() {
        if (!sk_surface || !sdl_window) return;

        SDL_GL_MakeCurrent(sdl_window, gl_ctx);

        SkCanvas* sk_canvas = sk_surface->getCanvas();
        canvas.bind(sk_canvas);

        root->dispatch_paint(canvas);
        tick_tooltip(canvas);

        gr_ctx->flushAndSubmit();
        SDL_GL_SwapWindow(sdl_window);
    }
};

Window::Window(WindowConfig cfg) : impl_{std::make_unique<Impl>()} {
    impl_->config = cfg;

    uint32_t sdl_flags = SDL_WINDOW_OPENGL;
    if (has_flag(cfg.flags, WindowFlags::Resizable))   sdl_flags |= SDL_WINDOW_RESIZABLE;
    if (has_flag(cfg.flags, WindowFlags::Borderless))  sdl_flags |= SDL_WINDOW_BORDERLESS;
    if (has_flag(cfg.flags, WindowFlags::AlwaysOnTop)) sdl_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (has_flag(cfg.flags, WindowFlags::HighDPI))     sdl_flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (has_flag(cfg.flags, WindowFlags::FullScreen))  sdl_flags |= SDL_WINDOW_FULLSCREEN;

    impl_->sdl_window = SDL_CreateWindow(
        cfg.title.c_str(),
        cfg.size.w, cfg.size.h,
        sdl_flags);

    assert(impl_->sdl_window && "SDL_CreateWindow failed");

    if (cfg.position.x >= 0 && cfg.position.y >= 0)
        SDL_SetWindowPosition(impl_->sdl_window, cfg.position.x, cfg.position.y);

    if (cfg.min_width > 0 || cfg.min_height > 0)
        SDL_SetWindowMinimumSize(impl_->sdl_window, cfg.min_width, cfg.min_height);
    if (cfg.max_width > 0 || cfg.max_height > 0)
        SDL_SetWindowMaximumSize(impl_->sdl_window, cfg.max_width, cfg.max_height);

    impl_->init_skia();

    impl_->root = std::make_unique<Widget>();
    impl_->root->set_bounds({0, 0, cfg.size.w, cfg.size.h});

    Application::instance().register_window(this);
    impl_->root->set_owner_window(this);
}

Window::~Window() {
    free_cursors();
    Application::instance().unregister_window(this);
    if (impl_->gl_ctx)     SDL_GL_DestroyContext(impl_->gl_ctx);
    if (impl_->sdl_window) SDL_DestroyWindow(impl_->sdl_window);
}

void Window::show() {
    SDL_ShowWindow(impl_->sdl_window);
    impl_->paint_frame();
}

void Window::hide() { SDL_HideWindow(impl_->sdl_window); }

void Window::close() {
    on_close.emit();
    impl_->closed = true;
    SDL_HideWindow(impl_->sdl_window);
}

void Window::raise() { SDL_RaiseWindow(impl_->sdl_window); }

bool Window::is_visible() const {
    if (!impl_->sdl_window) return false;
    return (SDL_GetWindowFlags(impl_->sdl_window) & SDL_WINDOW_HIDDEN) == 0;
}

bool Window::is_closed() const { return impl_->closed; }

void Window::set_title(std::string_view title) {
    SDL_SetWindowTitle(impl_->sdl_window, std::string{title}.c_str());
}

void Window::set_size(SizeI sz) {
    SDL_SetWindowSize(impl_->sdl_window, sz.w, sz.h);
    impl_->root->set_size(sz);
    impl_->resize_surface();
}

void Window::set_position(PointI pos) {
    SDL_SetWindowPosition(impl_->sdl_window, pos.x, pos.y);
}

void Window::set_min_size(SizeI sz) {
    SDL_SetWindowMinimumSize(impl_->sdl_window, sz.w, sz.h);
}

void Window::set_max_size(SizeI sz) {
    SDL_SetWindowMaximumSize(impl_->sdl_window, sz.w, sz.h);
}

void Window::center_on_screen() {
    SDL_SetWindowPosition(impl_->sdl_window,
                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void Window::enter_fullscreen() {
    SDL_SetWindowFullscreen(impl_->sdl_window, true);
}

void Window::exit_fullscreen() {
    SDL_SetWindowFullscreen(impl_->sdl_window, false);
}

std::string Window::title() const {
    return SDL_GetWindowTitle(impl_->sdl_window);
}

SizeI Window::size() const {
    int w = 0, h = 0;
    SDL_GetWindowSize(impl_->sdl_window, &w, &h);
    return {w, h};
}

PointI Window::position() const {
    int x = 0, y = 0;
    SDL_GetWindowPosition(impl_->sdl_window, &x, &y);
    return {x, y};
}

float Window::dpi_scale() const {
    int logical = 0, physical = 0;
    SDL_GetWindowSize(impl_->sdl_window, &logical, nullptr);
    SDL_GetWindowSizeInPixels(impl_->sdl_window, &physical, nullptr);
    return logical > 0 ? float(physical) / float(logical) : 1.f;
}

Widget& Window::root()              { return *impl_->root; }
const Widget& Window::root() const  { return *impl_->root; }

void Window::set_layout(std::unique_ptr<Layout> layout) {
    impl_->root->set_layout(std::move(layout));
}

void Window::set_focused_widget(Widget* w) {
    if (focused_ == w) return;
    if (focused_) {
        bool accepted = false;
        FocusEvent e{false, w};
        focused_->on_focus_loss(e);
    }
    focused_ = w;
    if (focused_) {
        FocusEvent e{true, nullptr};
        focused_->on_focus_gain(e);
    }
}

void Window::repaint_now() {
    impl_->paint_frame();
}

uint32_t Window::sdl_window_id() const {
    return SDL_GetWindowID(impl_->sdl_window);
}

void* Window::native_handle()     const { return impl_->sdl_window; }
void* Window::native_gl_context() const { return impl_->gl_ctx; }
void* Window::native_gr_context() const { return impl_->gr_ctx.get(); }

// ── Shortcuts ─────────────────────────────────────────────────────────────────

uint32_t Window::add_shortcut(Key key, KeyMods mods, std::function<void()> fn) {
    uint32_t id = next_shortcut_id_++;
    shortcuts_.push_back({id, key, mods, std::move(fn)});
    return id;
}

void Window::remove_shortcut(uint32_t id) {
    std::erase_if(shortcuts_, [id](const Shortcut& s){ return s.id == id; });
}

// ── Mouse capture ─────────────────────────────────────────────────────────────

void Window::capture_mouse(Widget* w) {
    captured_ = w;
    SDL_SetWindowMouseGrab(static_cast<SDL_Window*>(impl_->sdl_window), SDL_TRUE);
}

void Window::release_capture() {
    captured_ = nullptr;
    SDL_SetWindowMouseGrab(static_cast<SDL_Window*>(impl_->sdl_window), SDL_FALSE);
}

// ── Cursor ────────────────────────────────────────────────────────────────────

void Window::init_cursors() {
    static const SDL_SystemCursor kMap[] = {
        SDL_SYSTEM_CURSOR_DEFAULT,     // Arrow
        SDL_SYSTEM_CURSOR_TEXT,        // IBeam
        SDL_SYSTEM_CURSOR_WAIT,        // Wait
        SDL_SYSTEM_CURSOR_CROSSHAIR,   // Crosshair
        SDL_SYSTEM_CURSOR_POINTER,     // Hand
        SDL_SYSTEM_CURSOR_EW_RESIZE,   // SizeH
        SDL_SYSTEM_CURSOR_NS_RESIZE,   // SizeV
        SDL_SYSTEM_CURSOR_MOVE,        // SizeAll
        SDL_SYSTEM_CURSOR_NOT_ALLOWED, // Forbidden
        SDL_SYSTEM_CURSOR_DEFAULT,     // Blank (no native blank; hide separately)
    };
    for (int i = 0; i < 10; ++i)
        sdl_cursors_[i] = SDL_CreateSystemCursor(kMap[i]);
}

void Window::free_cursors() {
    for (int i = 0; i < 10; ++i) {
        if (sdl_cursors_[i]) SDL_DestroyCursor(static_cast<SDL_Cursor*>(sdl_cursors_[i]));
        sdl_cursors_[i] = nullptr;
    }
}

void Window::apply_cursor(CursorShape shape) {
    int idx = int(shape);
    if (idx < 0 || idx >= 10) return;
    if (!sdl_cursors_[0]) init_cursors();
    SDL_SetCursor(static_cast<SDL_Cursor*>(sdl_cursors_[idx]));
    if (shape == CursorShape::Blank) SDL_HideCursor();
    else SDL_ShowCursor();
}

// ── Tab navigation ────────────────────────────────────────────────────────────

void Window::collect_tab_widgets(Widget& w, std::vector<Widget*>& out) {
    if (!w.is_visible() || !w.is_enabled()) return;
    if (w.focus_policy() == FocusPolicy::Tab || w.focus_policy() == FocusPolicy::Strong)
        out.push_back(&w);
    for (auto& child : w.children())
        collect_tab_widgets(*child, out);
}

void Window::focus_next() {
    std::vector<Widget*> order;
    collect_tab_widgets(*impl_->root, order);
    if (order.empty()) return;
    auto it = std::find(order.begin(), order.end(), focused_);
    if (it == order.end() || std::next(it) == order.end())
        set_focused_widget(order.front());
    else
        set_focused_widget(*std::next(it));
}

void Window::focus_prev() {
    std::vector<Widget*> order;
    collect_tab_widgets(*impl_->root, order);
    if (order.empty()) return;
    auto it = std::find(order.begin(), order.end(), focused_);
    if (it == order.end() || it == order.begin())
        set_focused_widget(order.back());
    else
        set_focused_widget(*std::prev(it));
}

// ── Event dispatch ────────────────────────────────────────────────────────────

void Window::process_sdl_event(void* raw_event) {
    if (!raw_event) return;
    auto& ev = *static_cast<SDL_Event*>(raw_event);

    auto this_id = sdl_window_id();

    switch (ev.type) {
    case SDL_EVENT_WINDOW_RESIZED:
        if (ev.window.windowID != this_id) break;
        {
            SizeI new_size{ev.window.data1, ev.window.data2};
            impl_->root->set_size(new_size);
            impl_->resize_surface();
            on_resized.emit(new_size);
        }
        break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (ev.window.windowID != this_id) break;
        close();
        break;

    case SDL_EVENT_WINDOW_EXPOSED:
        if (ev.window.windowID != this_id) break;
        impl_->paint_frame();
        break;

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        if (ev.window.windowID != this_id) break;
        on_focus_gained.emit();
        break;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        if (ev.window.windowID != this_id) break;
        on_focus_lost.emit();
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (ev.button.windowID != this_id) break;
        {
            MouseEvent me;
            me.pos = {int(ev.button.x), int(ev.button.y)};
            {
                int wx = 0, wy = 0;
                SDL_GetWindowPosition(impl_->sdl_window, &wx, &wy);
                me.global_pos = {me.pos.x + wx, me.pos.y + wy};
            }
            me.button      = MouseButton(ev.button.button);
            me.click_count = ev.button.clicks;

            Widget* target = captured_
                ? captured_
                : impl_->root->hit_test(me.pos);
            if (!target) target = impl_->root.get();

            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                set_focused_widget(target->focus_policy() != FocusPolicy::None ? target : nullptr);
                if (ev.button.clicks == 2) target->on_double_click(me);
                else                       target->on_mouse_press(me);
            } else {
                target->on_mouse_release(me);
            }
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (ev.motion.windowID != this_id) break;
        {
            MouseEvent me;
            me.pos   = {int(ev.motion.x), int(ev.motion.y)};
            me.delta = {ev.motion.xrel, ev.motion.yrel};
            {
                int wx = 0, wy = 0;
                SDL_GetWindowPosition(impl_->sdl_window, &wx, &wy);
                me.global_pos = {me.pos.x + wx, me.pos.y + wy};
            }

            Widget* target = captured_
                ? captured_
                : impl_->root->hit_test(me.pos);
            if (!target) target = impl_->root.get();

            if (target != hovered_) {
                if (hovered_) hovered_->on_mouse_leave(me);
                hovered_ = target;
                if (hovered_) {
                    hovered_->on_mouse_enter(me);
                    apply_cursor(hovered_->cursor());
                }
            }
            target->on_mouse_move(me);

            // Tooltip
            std::string tip = target ? std::string{target->tooltip()} : std::string{};
            if (tip.empty()) {
                impl_->disarm_tooltip();
            } else {
                PointI cur{int(ev.motion.x), int(ev.motion.y)};
                PointI prev = impl_->tooltip_pos;
                int dx = cur.x - prev.x, dy = cur.y - prev.y;
                if ((dx*dx + dy*dy) > 16 || impl_->tooltip_text != tip)
                    impl_->arm_tooltip(tip, cur);
            }
        }
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        if (ev.wheel.windowID != this_id) break;
        {
            float mx = 0, my = 0;
            SDL_GetMouseState(&mx, &my);
            MouseEvent me;
            me.pos   = {int(mx), int(my)};
            me.wheel = {ev.wheel.x, ev.wheel.y};

            Widget* target = captured_
                ? captured_
                : impl_->root->hit_test(me.pos);
            if (target) target->on_mouse_scroll(me);
        }
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if (ev.key.windowID != this_id) break;
        {
            KeyEvent ke;
            ke.key       = Key(ev.key.key);
            ke.scancode  = ev.key.scancode;
            ke.is_repeat = ev.key.repeat;
            ke.mods.shift = (ev.key.mod & SDL_KMOD_SHIFT) != 0;
            ke.mods.ctrl  = (ev.key.mod & SDL_KMOD_CTRL)  != 0;
            ke.mods.alt   = (ev.key.mod & SDL_KMOD_ALT)   != 0;
            ke.mods.meta  = (ev.key.mod & SDL_KMOD_GUI)   != 0;

            if (ev.type == SDL_EVENT_KEY_DOWN) {
                // Tab navigation
                if (ev.key.key == SDLK_TAB && !ke.mods.ctrl && !ke.mods.alt) {
                    if (ke.mods.shift) focus_prev(); else focus_next();
                    break;
                }
                // Check window shortcuts before dispatching to focused widget
                for (auto& sc : shortcuts_) {
                    if (sc.key == ke.key && sc.mods == ke.mods) {
                        sc.fn();
                        goto done_key;
                    }
                }
                if (focused_) focused_->on_key_press(ke);
            } else {
                if (focused_) focused_->on_key_release(ke);
            }
            done_key:;
        }
        break;

    case SDL_EVENT_TEXT_INPUT:
        if (ev.text.windowID != this_id) break;
        if (focused_) {
            TextInputEvent te{ev.text.text};
            focused_->on_text_input(te);
        }
        break;

    default:
        break;
    }
}

} // namespace swole
