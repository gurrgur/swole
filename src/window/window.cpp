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

#include <cassert>
#include <unordered_map>

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
}

Window::~Window() {
    if (impl_->gl_ctx)    SDL_GL_DestroyContext(impl_->gl_ctx);
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

void Window::process_sdl_event(void* raw_event) {
    if (!raw_event) return;
    auto& ev = *static_cast<SDL_Event*>(raw_event);

    // Only handle events for this window
    auto this_id = sdl_window_id();

    switch (ev.type) {
    case SDL_EVENT_WINDOW_RESIZED:
        if (ev.window.windowID != this_id) break;
        {
            SizeI new_size{ev.window.data1, ev.window.data2};
            impl_->root->set_size(new_size);
            impl_->resize_surface();
            on_resized.emit(new_size);
            impl_->paint_frame();
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

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (ev.button.windowID != this_id) break;
        {
            MouseEvent me;
            me.pos        = {int(ev.button.x), int(ev.button.y)};
            me.global_pos = me.pos; // TODO: convert
            me.button     = MouseButton(ev.button.button);
            me.click_count = ev.button.clicks;

            Widget* target = impl_->root->hit_test(me.pos);
            if (!target) target = impl_->root.get();

            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                set_focused_widget(target->focus_policy() != FocusPolicy::None ? target : nullptr);
                if (ev.button.clicks == 2)
                    target->on_double_click(me);
                else
                    target->on_mouse_press(me);
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

            Widget* target = impl_->root->hit_test(me.pos);
            if (!target) target = impl_->root.get();

            if (target != hovered_) {
                if (hovered_) hovered_->on_mouse_leave(me);
                hovered_ = target;
                if (hovered_) hovered_->on_mouse_enter(me);
            }
            target->on_mouse_move(me);
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

            Widget* target = impl_->root->hit_test(me.pos);
            if (target) target->on_mouse_scroll(me);
        }
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if (ev.key.windowID != this_id) break;
        if (focused_) {
            KeyEvent ke;
            ke.key       = Key(ev.key.key);
            ke.scancode  = ev.key.scancode;
            ke.is_repeat = ev.key.repeat;
            ke.mods.shift = (ev.key.mod & SDL_KMOD_SHIFT) != 0;
            ke.mods.ctrl  = (ev.key.mod & SDL_KMOD_CTRL)  != 0;
            ke.mods.alt   = (ev.key.mod & SDL_KMOD_ALT)   != 0;
            ke.mods.meta  = (ev.key.mod & SDL_KMOD_GUI)   != 0;

            if (ev.type == SDL_EVENT_KEY_DOWN)
                focused_->on_key_press(ke);
            else
                focused_->on_key_release(ke);
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
