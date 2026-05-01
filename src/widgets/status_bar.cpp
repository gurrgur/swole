#include "swole/widgets/status_bar.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "../theme.hpp"

#include <SDL3/SDL.h>
#include <algorithm>

namespace swole {

StatusBar::StatusBar(Widget* parent) : Widget(parent) {}

void StatusBar::show_message(std::string_view msg, int timeout_ms) {
    message_ = msg;
    message_until_ms_ = (timeout_ms > 0)
        ? SDL_GetTicks() + uint64_t(timeout_ms)
        : 0;
    invalidate();
}

void StatusBar::clear_message() {
    message_.clear();
    message_until_ms_ = 0;
    invalidate();
}

int StatusBar::add_section(std::string_view text, int min_width) {
    sections_.push_back({std::string{text}, min_width});
    invalidate();
    return int(sections_.size()) - 1;
}

void StatusBar::set_section_text(int index, std::string_view text) {
    if (index < 0 || index >= int(sections_.size())) return;
    sections_[index].text = text;
    invalidate();
}

void StatusBar::remove_section(int index) {
    if (index < 0 || index >= int(sections_.size())) return;
    sections_.erase(sections_.begin() + index);
    invalidate();
}

WidgetSizeHint StatusBar::size_hint() const {
    return {.min_size = {40, 22}, .preferred_size = {200, 24}};
}

void StatusBar::tick_message() {
    if (message_until_ms_ != 0 && SDL_GetTicks() >= message_until_ms_) {
        message_.clear();
        message_until_ms_ = 0;
    }
}

void StatusBar::on_paint(Canvas& canvas) {
    tick_message();

    RectF r{local_bounds()};

    // Background + top border
    canvas.draw_rect(r, Paint::fill(theme::surface));
    canvas.draw_line(r.x, r.y + .5f, r.right(), r.y + .5f, Paint::stroke(theme::border));

    Font font = theme::default_font(12.f);
    auto m    = font.metrics();
    float ty  = r.y + (r.h - m.ascent - m.descent) * .5f + m.ascent;

    // Temporary message on the left
    if (!message_.empty()) {
        auto g = canvas.scoped_save();
        // compute right edge: left of sections
        float right_limit = r.right();
        if (!sections_.empty()) {
            float sx = r.right();
            for (auto it = sections_.rbegin(); it != sections_.rend(); ++it) {
                sx -= float(std::max(it->min_width,
                                     int(font.measure_text_width(it->text)) + 16));
                sx -= 1.f; // separator
            }
            right_limit = sx - 4.f;
        }
        canvas.clip_rect({r.x + 4.f, r.y, right_limit - r.x - 4.f, r.h});
        canvas.draw_text(message_, r.x + 6.f, ty, font, Paint::fill(theme::text));
    }

    // Permanent sections on the right (right-to-left)
    float x = r.right();
    for (auto it = sections_.rbegin(); it != sections_.rend(); ++it) {
        float sw = float(std::max(it->min_width,
                                  int(font.measure_text_width(it->text)) + 16));
        x -= sw;
        // Section separator
        canvas.draw_line(x, r.y + 3.f, x, r.bottom() - 3.f, Paint::stroke(theme::border));
        // Section text
        {
            auto g = canvas.scoped_save();
            canvas.clip_rect({x, r.y, sw, r.h});
            canvas.draw_text(it->text, x + 6.f, ty, font, Paint::fill(theme::text_dim));
        }
    }
}

} // namespace swole
