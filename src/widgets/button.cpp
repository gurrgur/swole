#include "swole/widgets/button.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"

namespace swole {

Button::Button(Widget* parent, std::string_view label)
    : Widget(parent), label_{label}, font_("", 13.f) {}

void Button::set_label(std::string_view text) {
    label_ = text;
    invalidate();
}

void Button::set_variant(ButtonVariant v) {
    variant_ = v;
    invalidate();
}

void Button::set_checked(bool v) {
    if (checked_ == v) return;
    checked_ = v;
    on_toggled.emit(checked_);
    invalidate();
}

void Button::set_font(Font f) {
    font_ = std::move(f);
    invalidate();
}

WidgetSizeHint Button::size_hint() const {
    SizeF text_sz = font_.measure_text(label_);
    int w = int(text_sz.w) + 24;
    int h = int(font_.metrics().line_height()) + 12;
    return {.preferred_size = {std::max(w, 64), std::max(h, 24)}};
}

void Button::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};

    // Background
    Color bg;
    if (!is_enabled())
        bg = Color{210, 210, 210};
    else if (pressed_)
        bg = Color{150, 180, 220};
    else if (hovered_)
        bg = Color{220, 230, 245};
    else if (checked_ && variant_ != ButtonVariant::Push)
        bg = Color{180, 210, 240};
    else
        bg = Color{230, 230, 230};

    canvas.draw_round_rect(r.inset(0.5f), 4.f, 4.f, Paint::fill(bg));
    canvas.draw_round_rect(r.inset(0.5f), 4.f, 4.f,
                           Paint::stroke(Color{160, 160, 160}, 1.f));

    // Check / radio indicator
    if (variant_ == ButtonVariant::Check || variant_ == ButtonVariant::Toggle) {
        RectF box{r.x + 4.f, r.y + (r.h - 14.f) * 0.5f, 14.f, 14.f};
        canvas.draw_round_rect(box, 2.f, 2.f,
                               Paint::fill(Color::white()));
        canvas.draw_round_rect(box, 2.f, 2.f,
                               Paint::stroke(Color{120, 120, 120}, 1.f));
        if (checked_) {
            Paint tick = Paint::stroke(Color{30, 120, 200}, 2.f);
            tick.set_stroke_cap(StrokeCap::Round);
            canvas.draw_line(box.x + 2.5f, box.y + box.h * 0.55f,
                             box.x + box.w * 0.42f, box.bottom() - 2.5f, tick);
            canvas.draw_line(box.x + box.w * 0.42f, box.bottom() - 2.5f,
                             box.right() - 2.f, box.y + 2.5f, tick);
        }
    } else if (variant_ == ButtonVariant::Radio) {
        float cx = r.x + 4.f + 7.f, cy = r.y + r.h * 0.5f;
        canvas.draw_circle(cx, cy, 7.f, Paint::fill(Color::white()));
        canvas.draw_circle(cx, cy, 7.f, Paint::stroke(Color{120, 120, 120}, 1.f));
        if (checked_)
            canvas.draw_circle(cx, cy, 3.5f, Paint::fill(Color{30, 120, 200}));
    }

    // Label
    Color text_color = is_enabled() ? Color::black() : Color{130, 130, 130};
    if (variant_ == ButtonVariant::Push) {
        canvas.draw_text(label_, r, TextAlign::Center, TextBaseline::Middle,
                         font_, Paint::fill(text_color));
    } else {
        RectF text_rect{r.x + 22.f, r.y, r.w - 22.f, r.h};
        canvas.draw_text(label_, text_rect, TextAlign::Left, TextBaseline::Middle,
                         font_, Paint::fill(text_color));
    }
}

void Button::on_mouse_press(const MouseEvent& e) {
    if (!is_enabled() || e.button != MouseButton::Left) return;
    pressed_ = true;
    invalidate();
}

void Button::on_mouse_release(const MouseEvent& e) {
    if (!is_enabled() || e.button != MouseButton::Left) return;
    bool was_pressed = pressed_;
    pressed_ = false;
    if (was_pressed && local_bounds().contains(e.pos)) {
        if (variant_ == ButtonVariant::Check || variant_ == ButtonVariant::Toggle)
            set_checked(!checked_);
        else if (variant_ == ButtonVariant::Radio)
            set_checked(true);
        on_clicked.emit();
    }
    invalidate();
}

void Button::on_mouse_enter(const MouseEvent&) {
    hovered_ = true; invalidate();
}
void Button::on_mouse_leave(const MouseEvent&) {
    hovered_ = false; pressed_ = false; invalidate();
}

void Button::on_key_press(const KeyEvent& e) {
    if (e.key == Key::Space || e.key == Key::Return) {
        on_mouse_press({.button = MouseButton::Left});
        on_mouse_release({.pos = {0, 0}, .button = MouseButton::Left});
    }
}

} // namespace swole
