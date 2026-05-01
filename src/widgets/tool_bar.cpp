#include "swole/widgets/tool_bar.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "../theme.hpp"

#include <algorithm>
#include <numeric>

namespace swole {

ToolBar::ToolBar(Widget* parent) : Widget(parent) {}

ToolBarAction& ToolBar::add_action(std::string_view label, Image icon) {
    auto& a = actions_.emplace_back(std::make_unique<ToolBarAction>());
    a->label = label;
    a->icon  = std::move(icon);
    invalidate();
    return *a;
}

ToolBarAction& ToolBar::add_toggle(std::string_view label, Image icon) {
    auto& a = actions_.emplace_back(std::make_unique<ToolBarAction>());
    a->label     = label;
    a->icon      = std::move(icon);
    a->checkable = true;
    invalidate();
    return *a;
}

void ToolBar::add_separator() {
    auto& a = actions_.emplace_back(std::make_unique<ToolBarAction>());
    a->separator = true;
    invalidate();
}

void ToolBar::remove_action(int index) {
    if (index < 0 || index >= int(actions_.size())) return;
    actions_.erase(actions_.begin() + index);
    hovered_ = pressed_ = -1;
    invalidate();
}

// ── Geometry helpers ──────────────────────────────────────────────────────────

int ToolBar::action_w(int i) const {
    const auto& a = *actions_[i];
    if (a.separator) return kSepW;
    Font font = theme::default_font(12.f);
    int w = kPadX * 2;
    if (a.icon.is_valid())  w += icon_size_.w + kGap;
    if (show_text_ && !a.label.empty())
        w += int(font.measure_text_width(a.label));
    return std::max(w, kPadX * 2 + 4);
}

int ToolBar::action_x(int i) const {
    int x = kPadX / 2;
    for (int j = 0; j < i; ++j)
        x += action_w(j);
    return x;
}

int ToolBar::action_at(PointI p) const {
    for (int i = 0; i < int(actions_.size()); ++i) {
        if (actions_[i]->separator) continue;
        int x0 = action_x(i);
        int x1 = x0 + action_w(i);
        if (p.x >= x0 && p.x < x1) return i;
    }
    return -1;
}

WidgetSizeHint ToolBar::size_hint() const {
    int total_w = kPadX;
    for (int i = 0; i < int(actions_.size()); ++i)
        total_w += action_w(i);
    int h = kPadY * 2 + icon_size_.h;
    if (show_text_) h = std::max(h, kPadY * 2 + 20);
    return {.min_size = {40, h}, .preferred_size = {total_w, h}};
}

// ── Painting ──────────────────────────────────────────────────────────────────

void ToolBar::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};

    // Background + bottom border
    canvas.draw_rect(r, Paint::fill(theme::surface));
    canvas.draw_line(r.x, r.bottom()-.5f, r.right(), r.bottom()-.5f,
                     Paint::stroke(theme::border));

    Font  font = theme::default_font(12.f);
    auto  m    = font.metrics();
    float h    = r.h;
    float fh   = m.ascent + m.descent;

    for (int i = 0; i < int(actions_.size()); ++i) {
        const auto& a = *actions_[i];
        int ax = action_x(i);
        int aw = action_w(i);

        if (a.separator) {
            float sx = float(ax) + float(kSepW) * .5f;
            canvas.draw_line(sx, float(kPadY), sx, h - float(kPadY),
                             Paint::stroke(theme::border));
            continue;
        }

        RectF item{float(ax), 1.f, float(aw), h - 2.f};

        // Background highlight
        if (!a.enabled) {
            // no highlight
        } else if (i == pressed_ || (a.checkable && a.checked)) {
            canvas.draw_round_rect(item.inset(1.f), theme::radius_small, theme::radius_small,
                                   Paint::fill(theme::sel_bg));
        } else if (i == hovered_) {
            canvas.draw_round_rect(item.inset(1.f), theme::radius_small, theme::radius_small,
                                   Paint::fill(theme::hover_bg));
        }

        Color col = a.enabled
            ? ((i == pressed_ || (a.checkable && a.checked)) ? theme::sel_text : theme::text)
            : theme::text_disabled;

        float cx = float(ax) + float(kPadX);
        float cy = h * .5f;

        // Icon
        if (a.icon.is_valid()) {
            float iy = cy - float(icon_size_.h) * .5f;
            canvas.draw_image(a.icon, {cx, iy, float(icon_size_.w), float(icon_size_.h)});
            cx += float(icon_size_.w) + float(kGap);
        }

        // Label
        if (show_text_ && !a.label.empty()) {
            canvas.draw_text(a.label, cx, cy - fh*.5f + m.ascent,
                             font, Paint::fill(col));
        }
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void ToolBar::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    int idx = action_at(e.pos);
    if (idx >= 0 && actions_[idx]->enabled) {
        pressed_ = idx;
        invalidate();
    }
}

void ToolBar::on_mouse_release(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    int idx = action_at(e.pos);
    if (idx >= 0 && idx == pressed_ && actions_[idx]->enabled) {
        auto& a = *actions_[idx];
        if (a.checkable) {
            a.checked = !a.checked;
            a.on_toggled.emit(a.checked);
        }
        a.on_triggered.emit();
    }
    pressed_ = -1;
    invalidate();
}

void ToolBar::on_mouse_move(const MouseEvent& e) {
    int idx = action_at(e.pos);
    if (idx != hovered_) { hovered_ = idx; invalidate(); }
}

void ToolBar::on_mouse_leave(const MouseEvent&) {
    if (hovered_ >= 0) { hovered_ = -1; invalidate(); }
    pressed_ = -1;
}

} // namespace swole
