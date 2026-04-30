#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <string>

namespace swole {

class Label : public Widget {
public:
    explicit Label(Widget* parent = nullptr, std::string_view text = {});

    void set_text(std::string_view text);
    [[nodiscard]] std::string_view text() const { return text_; }

    void set_font(Font font);
    void set_color(Color c) { color_ = c; invalidate(); }
    void set_align(TextAlign align) { align_ = align; invalidate(); }
    void set_word_wrap(bool wrap) { word_wrap_ = wrap; invalidate(); }
    void set_elide(bool elide) { elide_ = elide; invalidate(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;

private:
    std::string text_;
    Font        font_;
    Color       color_{Color::black()};
    TextAlign   align_{TextAlign::Left};
    bool        word_wrap_{false};
    bool        elide_{true};
};

} // namespace swole
