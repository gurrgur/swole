#include "swole/widgets/progress_bar.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <cstdio>

namespace swole {

ProgressBar::ProgressBar(Widget* parent, Orientation orientation)
    : Widget(parent), orientation_{orientation} {}

void ProgressBar::set_range(double min, double max) {
    min_ = min; max_ = max;
    value_ = std::clamp(value_, min_, max_);
    invalidate();
}

void ProgressBar::set_value(double v) {
    value_ = std::clamp(v, min_, max_);
    indeterminate_ = false;
    invalidate();
}

void ProgressBar::set_indeterminate(bool v) {
    indeterminate_ = v;
    invalidate();
}

WidgetSizeHint ProgressBar::size_hint() const {
    if (orientation_ == Orientation::Horizontal)
        return {.min_size = {60, 16}, .preferred_size = {200, 16}};
    else
        return {.min_size = {16, 60}, .preferred_size = {16, 200}};
}

void ProgressBar::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};

    // Track
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::fill(theme::track_bg));

    // Fill
    double t = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0;
    if (indeterminate_) {
        // Animated stripe — anim_phase_ advanced by caller / timer
        constexpr float stripe_w = 0.35f;
        float start = anim_phase_ - stripe_w;
        RectF fill;
        if (orientation_ == Orientation::Horizontal) {
            fill = {r.x + r.w * start, r.y, r.w * stripe_w, r.h};
        } else {
            fill = {r.x, r.y + r.h * start, r.w, r.h * stripe_w};
        }
        auto guard = canvas.scoped_save();
        canvas.clip_round_rect(r.inset(.5f), theme::radius, theme::radius);
        canvas.draw_rect(fill, Paint::fill(theme::accent));
    } else if (t > 0.0) {
        RectF fill;
        if (orientation_ == Orientation::Horizontal) {
            fill = {r.x, r.y, r.w * float(t), r.h};
        } else {
            float fh = r.h * float(t);
            fill = {r.x, r.bottom() - fh, r.w, fh};
        }
        auto guard = canvas.scoped_save();
        canvas.clip_round_rect(r.inset(.5f), theme::radius, theme::radius);
        canvas.draw_rect(fill, Paint::fill(theme::accent));
    }

    // Border
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::stroke(theme::border));

    // Optional text
    if (show_text_ && !indeterminate_) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), text_fmt_.c_str(), t * 100.0);
        Font font = theme::default_font(11.f);
        canvas.draw_text(buf, r, TextAlign::Center, TextBaseline::Middle,
                         font, Paint::fill(theme::text));
    }
}

} // namespace swole
