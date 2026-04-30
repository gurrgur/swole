#include "swole/widgets/slider.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>
#include <cmath>

namespace swole {

Slider::Slider(Widget* parent, Orientation orientation)
    : Widget(parent), orientation_{orientation} {
    set_focus_policy(FocusPolicy::Click);
}

void Slider::set_range(double min, double max) {
    min_   = min;
    max_   = max;
    value_ = std::clamp(value_, min_, max_);
    invalidate();
}

void Slider::set_value(double v) {
    double clamped = std::clamp(v, min_, max_);
    if (clamped == value_) return;
    value_ = clamped;
    on_value_changed.emit(value_);
    invalidate();
}

WidgetSizeHint Slider::size_hint() const {
    if (orientation_ == Orientation::Horizontal)
        return {.min_size = {80, 20}, .preferred_size = {200, 20}};
    else
        return {.min_size = {20, 80}, .preferred_size = {20, 200}};
}

RectF Slider::track_rect() const {
    const float tw = 4.f;
    RectF r{local_bounds()};
    if (orientation_ == Orientation::Horizontal) {
        float cy = r.y + r.h * .5f;
        return {r.x + 8.f, cy - tw * .5f, r.w - 16.f, tw};
    } else {
        float cx = r.x + r.w * .5f;
        return {cx - tw * .5f, r.y + 8.f, tw, r.h - 16.f};
    }
}

RectF Slider::thumb_rect() const {
    const float th = 16.f;
    RectF tr = track_rect();
    double t = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0;
    if (orientation_ == Orientation::Horizontal) {
        float cx = tr.x + float(t) * tr.w;
        return {cx - th * .5f, tr.y + tr.h * .5f - th * .5f, th, th};
    } else {
        float cy = tr.bottom() - float(t) * tr.h; // invert so up = more
        return {tr.x + tr.w * .5f - th * .5f, cy - th * .5f, th, th};
    }
}

double Slider::value_at(PointF pos) const {
    RectF tr = track_rect();
    double t = 0.0;
    if (orientation_ == Orientation::Horizontal)
        t = (tr.w > 0) ? double(pos.x - tr.x) / double(tr.w) : 0.0;
    else
        t = (tr.h > 0) ? 1.0 - double(pos.y - tr.y) / double(tr.h) : 0.0;
    t = std::clamp(t, 0.0, 1.0);
    double raw = min_ + t * (max_ - min_);
    if (snap_) raw = std::round(raw / step_) * step_;
    return std::clamp(raw, min_, max_);
}

void Slider::on_paint(Canvas& canvas) {
    RectF tr = track_rect();
    RectF th = thumb_rect();

    // Track background
    canvas.draw_round_rect(tr, 2.f, 2.f, Paint::fill(theme::track_bg));

    // Filled portion
    double t = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0;
    if (orientation_ == Orientation::Horizontal) {
        RectF filled{tr.x, tr.y, tr.w * float(t), tr.h};
        if (filled.w > 0)
            canvas.draw_round_rect(filled, 2.f, 2.f, Paint::fill(theme::accent));
    } else {
        float fh = tr.h * float(t);
        RectF filled{tr.x, tr.bottom() - fh, tr.w, fh};
        if (filled.h > 0)
            canvas.draw_round_rect(filled, 2.f, 2.f, Paint::fill(theme::accent));
    }

    // Thumb
    Color thumb_col = dragging_     ? theme::accent_dark
                    : has_focus()   ? theme::accent
                                    : Color{255, 255, 255};
    canvas.draw_oval(th, Paint::fill(thumb_col));
    canvas.draw_oval(th, Paint::stroke(dragging_ ? theme::accent_dark : theme::border));
}

void Slider::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    dragging_ = true;
    on_slide_started.emit(value_);
    set_value(value_at(PointF{float(e.pos.x), float(e.pos.y)}));
}

void Slider::on_mouse_release(const MouseEvent& e) {
    if (e.button != MouseButton::Left || !dragging_) return;
    dragging_ = false;
    set_value(value_at(PointF{float(e.pos.x), float(e.pos.y)}));
    on_slide_finished.emit(value_);
}

void Slider::on_mouse_move(const MouseEvent& e) {
    if (!dragging_) return;
    set_value(value_at(PointF{float(e.pos.x), float(e.pos.y)}));
}

void Slider::on_key_press(const KeyEvent& e) {
    switch (e.key) {
    case Key::Left: case Key::Down: set_value(value_ - step_);      break;
    case Key::Right: case Key::Up:  set_value(value_ + step_);      break;
    case Key::PageDown:             set_value(value_ - page_step_); break;
    case Key::PageUp:               set_value(value_ + page_step_); break;
    case Key::Home:                 set_value(min_);                break;
    case Key::End:                  set_value(max_);                break;
    default: break;
    }
}

} // namespace swole
