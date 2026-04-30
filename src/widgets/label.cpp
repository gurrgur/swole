#include "swole/widgets/label.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"

namespace swole {

Label::Label(Widget* parent, std::string_view text)
    : Widget(parent), text_{text}, font_("", 13.f) {}

void Label::set_text(std::string_view text) {
    text_ = text;
    invalidate();
}

void Label::set_font(Font f) {
    font_ = std::move(f);
    invalidate();
}

WidgetSizeHint Label::size_hint() const {
    SizeF sz = font_.measure_text(text_);
    return {.preferred_size = {int(sz.w) + 4, int(sz.h) + 4}};
}

void Label::on_paint(Canvas& canvas) {
    if (text_.empty()) return;
    RectF r{local_bounds()};
    canvas.draw_text(text_, r, align_, TextBaseline::Middle, font_, Paint::fill(color_));
}

} // namespace swole
