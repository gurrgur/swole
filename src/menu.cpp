#include "swole/menu.hpp"
#include "swole/core/application.hpp"
#include "swole/window/window.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "theme.hpp"

#include <SDL3/SDL.h>

// Skia GPU — only in this translation unit.
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"

#include <algorithm>

namespace swole {

// ── MenuItem ──────────────────────────────────────────────────────────────────

MenuItem::MenuItem(std::string_view label) : label_{label} {}

MenuItem MenuItem::action(std::string_view label, std::function<void()> fn) {
    MenuItem m{label};
    m.on_triggered = std::move(fn);
    return m;
}
MenuItem MenuItem::separator() {
    MenuItem m{""};
    m.kind_ = MenuItemKind::Separator;
    return m;
}
MenuItem MenuItem::submenu(std::string_view label, std::unique_ptr<Menu> sub) {
    MenuItem m{label};
    m.kind_    = MenuItemKind::SubMenu;
    m.submenu_ = std::move(sub);
    return m;
}
void MenuItem::set_shortcut(Key key, KeyMods mods) {
    shortcut_key_  = key;
    shortcut_mods_ = mods;
}

// ── Menu ──────────────────────────────────────────────────────────────────────

MenuItem& Menu::add_action(std::string_view label, std::function<void()> fn) {
    items_.push_back(std::make_unique<MenuItem>(MenuItem::action(label, std::move(fn))));
    return *items_.back();
}
MenuItem& Menu::add_separator() {
    items_.push_back(std::make_unique<MenuItem>(MenuItem::separator()));
    return *items_.back();
}
Menu& Menu::add_submenu(std::string_view label, std::unique_ptr<Menu> sub) {
    items_.push_back(std::make_unique<MenuItem>(MenuItem::submenu(label, std::move(sub))));
    return *this;
}

// ── Popup geometry & rendering ────────────────────────────────────────────────

namespace {

static constexpr int   kItemH  = 26;
static constexpr int   kSepH   =  8;
static constexpr int   kPadX   = 14;
static constexpr int   kMinW   = 160;
static constexpr int   kArrowW = 16;
static constexpr float kRadius = 6.f;

struct PopupGeometry {
    const Menu* menu;
    int         hovered{-1};
    int         width{kMinW};
    int         height{8};

    void measure() {
        Font font = theme::default_font();
        for (auto& it : menu->items_) {
            if (it->kind() == MenuItemKind::Separator) { height += kSepH; continue; }
            int tw = int(font.measure_text_width(it->label())) + kPadX * 2 + 20;
            if (it->kind() == MenuItemKind::SubMenu) tw += kArrowW;
            width  = std::max(width, tw);
            height += kItemH;
        }
        width  += 2;
        height += 8;
    }

    int item_y(int index) const {
        int y = 5;
        for (int i = 0; i < index && i < int(menu->items_.size()); ++i)
            y += (menu->items_[i]->kind() == MenuItemKind::Separator) ? kSepH : kItemH;
        return y;
    }

    int item_at(int ly) const {
        int y = 5;
        for (int i = 0; i < int(menu->items_.size()); ++i) {
            bool sep = (menu->items_[i]->kind() == MenuItemKind::Separator);
            int h = sep ? kSepH : kItemH;
            if (!sep && ly >= y && ly < y + h) return i;
            y += h;
        }
        return -1;
    }

    void render(Canvas& canvas) {
        RectF r{0.f, 0.f, float(width), float(height)};
        canvas.draw_round_rect(r.inset(.5f), kRadius, kRadius, Paint::fill(theme::bg));
        canvas.draw_round_rect(r.inset(.5f), kRadius, kRadius, Paint::stroke(theme::border));

        Font font = theme::default_font();
        auto m    = font.metrics();

        for (int i = 0; i < int(menu->items_.size()); ++i) {
            const MenuItem& item = *menu->items_[i];
            int iy = item_y(i);

            if (item.kind() == MenuItemKind::Separator) {
                float cy = float(iy) + kSepH * .5f;
                canvas.draw_line(float(kPadX * .5f), cy, float(width - kPadX / 2), cy,
                                 Paint::stroke(theme::border));
                continue;
            }

            RectF ir{1.f, float(iy), float(width) - 2.f, float(kItemH)};
            if (i == hovered && item.is_enabled())
                canvas.draw_round_rect(ir.inset(2.f, 1.f),
                                       theme::radius_small, theme::radius_small,
                                       Paint::fill(theme::sel_bg));

            Color col = !item.is_enabled() ? theme::text_disabled
                      : (i == hovered)     ? theme::sel_text
                                           : theme::text;

            float by = ir.y + (ir.h - m.ascent - m.descent) * .5f + m.ascent;
            canvas.draw_text(item.label(), float(kPadX), by, font, Paint::fill(col));

            // Checkmark
            if (item.is_checked()) {
                auto cp = Paint::stroke(col, 1.5f);
                cp.set_stroke_cap(StrokeCap::Round);
                float cx = 7.f, cy = ir.y + ir.h * .5f;
                canvas.draw_line(cx - 2.f, cy, cx, cy + 3.f, cp);
                canvas.draw_line(cx, cy + 3.f, cx + 4.f, cy - 3.f, cp);
            }

            // Submenu arrow
            if (item.kind() == MenuItemKind::SubMenu) {
                float ax = float(width) - float(kArrowW) * .5f;
                float ay = ir.y + ir.h * .5f;
                const float s = 4.f;
                PointF pts[] = {{ax - s*.4f, ay - s},
                                {ax + s*.6f, ay},
                                {ax - s*.4f, ay + s}};
                canvas.draw_polygon(pts, true, Paint::fill(col));
            }
        }
    }
};

// ── Popup window ──────────────────────────────────────────────────────────────

// Creates a borderless SDL3 popup window that shares the parent's GL context
// so it can reuse the same GrDirectContext for GPU-accelerated rendering.
MenuItem* run_popup(const Menu* menu, PointI screen_pos, Window& parent_window) {
    if (!menu || menu->item_count() == 0) return nullptr;

    PopupGeometry geo{menu};
    geo.measure();

    auto* parent_sdl = static_cast<SDL_Window*>(parent_window.native_handle());
    auto* parent_gl  = static_cast<SDL_GLContext>(parent_window.native_gl_context());
    auto* gr         = static_cast<GrDirectContext*>(parent_window.native_gr_context());

    // Clamp to screen bounds
    SDL_Rect display_bounds{};
    SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(parent_sdl), &display_bounds);
    int px = std::clamp(screen_pos.x, display_bounds.x,
                        display_bounds.x + display_bounds.w - geo.width);
    int py = std::clamp(screen_pos.y, display_bounds.y,
                        display_bounds.y + display_bounds.h - geo.height);

    // Share the parent's GL context so we can reuse GrDirectContext.
    SDL_GL_MakeCurrent(parent_sdl, parent_gl);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    SDL_Window* popup_win = SDL_CreatePopupWindow(
        parent_sdl, px, py, geo.width, geo.height,
        SDL_WINDOW_POPUP_MENU | SDL_WINDOW_OPENGL);
    if (!popup_win) return nullptr;

    SDL_GLContext popup_gl = SDL_GL_CreateContext(popup_win);
    if (!popup_gl) { SDL_DestroyWindow(popup_win); return nullptr; }

    SDL_GL_MakeCurrent(popup_win, popup_gl);

    // Create a Skia surface wrapping the popup's default framebuffer.
    auto mk_surface = [&]() -> sk_sp<SkSurface> {
        GrGLFramebufferInfo fb_info{};
        fb_info.fFBOID  = 0;
        fb_info.fFormat = 0x8058; // GL_RGBA8
        auto target = GrBackendRenderTarget(geo.width, geo.height, 0, 8, fb_info);
        SkSurfaceProps props;
        return SkSurfaces::WrapBackendRenderTarget(
            gr, target, kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), &props);
    };
    auto sk_surface = mk_surface();
    if (!sk_surface) {
        SDL_GL_DestroyContext(popup_gl);
        SDL_DestroyWindow(popup_win);
        return nullptr;
    }

    auto repaint = [&] {
        SDL_GL_MakeCurrent(popup_win, popup_gl);
        SkCanvas* sk_c = sk_surface->getCanvas();
        sk_c->clear(SK_ColorTRANSPARENT);
        Canvas canvas;
        canvas.bind(sk_c);
        geo.render(canvas);
        gr->flushAndSubmit();
        SDL_GL_SwapWindow(popup_win);
    };
    SDL_ShowWindow(popup_win);
    repaint();

    uint32_t popup_id = SDL_GetWindowID(popup_win);
    MenuItem* result  = nullptr;
    bool      done    = false;

    SDL_Event ev;
    while (!done) {
        SDL_WaitEventTimeout(&ev, 16);
        do {
            switch (ev.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (ev.button.windowID != popup_id) { done = true; break; }
                {
                    int idx = geo.item_at(int(ev.button.y));
                    if (idx >= 0) {
                        auto& item = *menu->items_[idx];
                        if (item.is_enabled() && item.kind() == MenuItemKind::Action) {
                            result = &item;
                            done   = true;
                        }
                    }
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (ev.motion.windowID == popup_id) {
                    int nh = geo.item_at(int(ev.motion.y));
                    if (nh != geo.hovered) { geo.hovered = nh; repaint(); }
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                switch (ev.key.key) {
                case SDLK_ESCAPE: done = true; break;
                case SDLK_UP:
                    { int n = geo.hovered - 1;
                      while (n >= 0 && menu->items_[n]->kind() == MenuItemKind::Separator) --n;
                      if (n >= 0) { geo.hovered = n; repaint(); } }
                    break;
                case SDLK_DOWN:
                    { int n = geo.hovered + 1;
                      while (n < int(menu->items_.size()) &&
                             menu->items_[n]->kind() == MenuItemKind::Separator) ++n;
                      if (n < int(menu->items_.size())) { geo.hovered = n; repaint(); } }
                    break;
                case SDLK_RETURN: case SDLK_SPACE:
                    if (geo.hovered >= 0) {
                        auto& item = *menu->items_[geo.hovered];
                        if (item.is_enabled() && item.kind() == MenuItemKind::Action) {
                            result = &item; done = true;
                        }
                    }
                    break;
                default: break;
                }
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (ev.window.windowID == popup_id) done = true;
                break;
            case SDL_EVENT_QUIT:
                done = true;
                break;
            default:
                // Forward to main windows so they keep repainting.
                for (Window* w : Application::instance().windows())
                    w->process_sdl_event(&ev);
                break;
            }
        } while (!done && SDL_PollEvent(&ev));
    }

    sk_surface.reset();
    gr->resetContext();
    SDL_GL_MakeCurrent(parent_sdl, parent_gl);
    SDL_GL_DestroyContext(popup_gl);
    SDL_DestroyWindow(popup_win);

    if (result) result->trigger();
    return result;
}

} // namespace

// ── Menu::exec ────────────────────────────────────────────────────────────────

MenuItem* Menu::exec(Window& parent, PointI screen_pos) {
    return run_popup(this, screen_pos, parent);
}

// ── MenuBar ───────────────────────────────────────────────────────────────────

MenuBar::MenuBar(Widget* parent) : Widget(parent) {
    set_focus_policy(FocusPolicy::Click);
}

Menu& MenuBar::add_menu(std::string_view title) {
    menus_.push_back({std::string{title}, std::make_unique<Menu>()});
    invalidate();
    return *menus_.back().menu;
}

void MenuBar::remove_menu(int index) {
    menus_.erase(menus_.begin() + index);
    invalidate();
}

WidgetSizeHint MenuBar::size_hint() const {
    return {.min_size = {40, 28}, .preferred_size = {200, 28}};
}

void MenuBar::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    canvas.draw_rect(r, Paint::fill(theme::surface));
    canvas.draw_line(r.x, r.bottom() - .5f, r.right(), r.bottom() - .5f,
                     Paint::stroke(theme::border));

    Font  font = theme::default_font();
    auto  m    = font.metrics();
    float base_y = (r.h - m.ascent - m.descent) * .5f + m.ascent;
    float x = 4.f;

    for (int i = 0; i < int(menus_.size()); ++i) {
        float tw = font.measure_text_width(menus_[i].title) + 16.f;
        bool  active = (i == open_index_);
        if (active)
            canvas.draw_round_rect({x - 2.f, 2.f, tw, r.h - 4.f},
                                   theme::radius_small, theme::radius_small,
                                   Paint::fill(theme::sel_bg));
        canvas.draw_text(menus_[i].title, x + 6.f, base_y, font,
                         Paint::fill(active ? theme::sel_text : theme::text));
        x += tw;
    }
}

void MenuBar::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;

    Font  font = theme::default_font();
    float x = 4.f;
    for (int i = 0; i < int(menus_.size()); ++i) {
        float tw = font.measure_text_width(menus_[i].title) + 16.f;
        if (float(e.pos.x) >= x && float(e.pos.x) < x + tw) {
            open_index_ = i;
            invalidate();

            // Convert widget-local bar bottom to screen position.
            Window* win = window();
            if (!win) { open_index_ = -1; return; }

            PointI bar_local_bottom{int(x), size().h};
            PointI root_pos = map_to_root(bar_local_bottom);
            PointI screen_pos{0, 0};
            SDL_GetWindowPosition(
                static_cast<SDL_Window*>(win->native_handle()),
                &screen_pos.x, &screen_pos.y);
            screen_pos.x += root_pos.x;
            screen_pos.y += root_pos.y;

            menus_[i].menu->exec(*win, screen_pos);
            open_index_ = -1;
            invalidate();
            return;
        }
        x += tw;
    }
}

} // namespace swole
