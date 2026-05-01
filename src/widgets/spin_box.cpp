#include "swole/widgets/spin_box.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "../theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace swole {

SpinBox::SpinBox(Widget* parent) : Widget(parent) {
    set_focus_policy(FocusPolicy::Click);
}

void SpinBox::set_range(double mn, double mx) {
    min_ = mn; max_ = mx;
    set_value(value_);
}

void SpinBox::set_value(double v) {
    double clamped = clamp(v);
    if (clamped == value_) return;
    value_ = clamped;
    if (editing_) edit_text_ = format_value(value_);
    invalidate();
    on_value_changed.emit(value_);
}

double SpinBox::clamp(double v) const {
    return std::clamp(v, min_, max_);
}

std::string SpinBox::format_value(double v) const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals_, v);
    return buf;
}

RectI SpinBox::up_rect() const {
    int h = size().h;
    return {size().w - kBtnW, 0, kBtnW, h/2};
}

RectI SpinBox::dn_rect() const {
    int h = size().h;
    return {size().w - kBtnW, h/2, kBtnW, h - h/2};
}

WidgetSizeHint SpinBox::size_hint() const {
    Font font = theme::default_font();
    std::string sample = prefix_ + format_value(max_) + suffix_;
    float tw = font.measure_text_width(sample);
    return {
        .min_size       = {80,  24},
        .preferred_size = {int(tw) + kBtnW + 16, 28},
    };
}

void SpinBox::on_paint(Canvas& canvas) {
    bool focused = has_focus();
    RectF r{local_bounds()};
    theme::draw_widget_bg(canvas, r, focused);

    Font  font = theme::default_font();
    auto  m    = font.metrics();
    float text_y = r.h*.5f - (m.ascent+m.descent)*.5f + m.ascent;

    // Value text (or edit buffer)
    std::string display = editing_ ? prefix_ + edit_text_ + suffix_
                                   : prefix_ + format_value(value_) + suffix_;
    {
        auto g = canvas.scoped_save();
        canvas.clip_rect({r.x+2.f, r.y+2.f, r.w - float(kBtnW) - 4.f, r.h-4.f});
        canvas.draw_text(display, r.x+6.f, text_y, font, Paint::fill(theme::text));

        // Cursor
        if (editing_ && focused) {
            std::string before = prefix_ + edit_text_.substr(0, cursor_);
            float cx = r.x + 6.f + font.measure_text_width(before);
            canvas.draw_line(cx, text_y - m.ascent, cx, text_y + m.descent,
                             Paint::stroke(theme::text, 1.f));
        }
    }

    // Divider before buttons
    float bx = r.right() - float(kBtnW);
    canvas.draw_line(bx, r.y+2.f, bx, r.bottom()-2.f, Paint::stroke(theme::border));

    // Arrow buttons
    RectI ur = up_rect(), dr = dn_rect();
    if (hovered_up_ || pressed_up_)
        canvas.draw_rect({float(ur.x), float(ur.y), float(ur.w), float(ur.h)},
                         Paint::fill(pressed_up_ ? theme::sel_bg : theme::hover_bg));
    if (hovered_dn_ || pressed_dn_)
        canvas.draw_rect({float(dr.x), float(dr.y), float(dr.w), float(dr.h)},
                         Paint::fill(pressed_dn_ ? theme::sel_bg : theme::hover_bg));

    // Up arrow ▲
    {
        float ax = float(ur.x) + float(ur.w)*.5f;
        float ay = float(ur.y) + float(ur.h)*.5f;
        const float s = 3.5f;
        PointF pts[]{{ax-s, ay+s*.6f},{ax+s, ay+s*.6f},{ax, ay-s*.8f}};
        canvas.draw_polygon(pts, true, Paint::fill(theme::text_dim));
    }
    // Down arrow ▼
    {
        float ax = float(dr.x) + float(dr.w)*.5f;
        float ay = float(dr.y) + float(dr.h)*.5f;
        const float s = 3.5f;
        PointF pts[]{{ax-s, ay-s*.6f},{ax+s, ay-s*.6f},{ax, ay+s*.8f}};
        canvas.draw_polygon(pts, true, Paint::fill(theme::text_dim));
    }
}

void SpinBox::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    PointF p{float(e.pos.x), float(e.pos.y)};
    RectF ur{float(up_rect().x), float(up_rect().y),
             float(up_rect().w), float(up_rect().h)};
    RectF dr{float(dn_rect().x), float(dn_rect().y),
             float(dn_rect().w), float(dn_rect().h)};
    if (ur.contains(p)) { pressed_up_ = true; step_by(+1); }
    else if (dr.contains(p)) { pressed_dn_ = true; step_by(-1); }
    else {
        // Click in text area: start editing
        editing_   = true;
        edit_text_ = format_value(value_);
        cursor_    = int(edit_text_.size());
    }
    invalidate();
}

void SpinBox::on_mouse_release(const MouseEvent&) {
    pressed_up_ = pressed_dn_ = false;
    invalidate();
}

void SpinBox::on_mouse_move(const MouseEvent& e) {
    PointF p{float(e.pos.x), float(e.pos.y)};
    RectF ur{float(up_rect().x), float(up_rect().y), float(up_rect().w), float(up_rect().h)};
    RectF dr{float(dn_rect().x), float(dn_rect().y), float(dn_rect().w), float(dn_rect().h)};
    bool nu = ur.contains(p), nd = dr.contains(p);
    if (nu != hovered_up_ || nd != hovered_dn_) {
        hovered_up_ = nu; hovered_dn_ = nd; invalidate();
    }
}

void SpinBox::on_mouse_leave(const MouseEvent&) {
    if (hovered_up_ || hovered_dn_) {
        hovered_up_ = hovered_dn_ = false; invalidate();
    }
}

void SpinBox::on_key_press(const KeyEvent& e) {
    if (!editing_) {
        if (e.key == Key::Up)   { step_by(+1); return; }
        if (e.key == Key::Down) { step_by(-1); return; }
    }
    switch (e.key) {
    case Key::Up:
        step_by(+1); return;
    case Key::Down:
        step_by(-1); return;
    case Key::Return:
    case Key::Tab:
        commit_text(); return;
    case Key::Escape:
        editing_   = false;
        edit_text_ = format_value(value_);
        invalidate(); return;
    case Key::Left:
        if (cursor_ > 0) { --cursor_; invalidate(); } return;
    case Key::Right:
        if (cursor_ < int(edit_text_.size())) { ++cursor_; invalidate(); } return;
    case Key::Home:
        cursor_ = 0; invalidate(); return;
    case Key::End:
        cursor_ = int(edit_text_.size()); invalidate(); return;
    case Key::Backspace:
        if (editing_ && cursor_ > 0) {
            edit_text_.erase(cursor_-1, 1); --cursor_; invalidate();
        }
        return;
    case Key::Delete:
        if (editing_ && cursor_ < int(edit_text_.size())) {
            edit_text_.erase(cursor_, 1); invalidate();
        }
        return;
    default: break;
    }
}

void SpinBox::on_text_input(const TextInputEvent& e) {
    editing_ = true;
    // Accept digits, decimal point, minus
    for (char c : e.text) {
        if (std::isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E') {
            edit_text_.insert(cursor_, 1, c);
            ++cursor_;
        }
    }
    invalidate();
}

void SpinBox::on_focus_gain(const FocusEvent& e) {
    Widget::on_focus_gain(e);
    editing_   = true;
    edit_text_ = format_value(value_);
    cursor_    = int(edit_text_.size());
}

void SpinBox::on_focus_loss(const FocusEvent& e) {
    commit_text();
    Widget::on_focus_loss(e);
}

void SpinBox::step_by(int dir) {
    set_value(clamp(value_ + dir * step_));
}

void SpinBox::commit_text() {
    if (!editing_) return;
    try {
        double v = std::stod(edit_text_);
        set_value(v);
    } catch (...) {}
    edit_text_ = format_value(value_);
    editing_   = false;
    invalidate();
}

} // namespace swole
