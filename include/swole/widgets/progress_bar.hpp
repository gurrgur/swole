#pragma once

#include "../window/widget.hpp"

namespace swole {

class ProgressBar : public Widget {
public:
    explicit ProgressBar(Widget* parent = nullptr,
                         Orientation orientation = Orientation::Horizontal);

    void set_range(double min, double max);
    void set_value(double v);
    void set_indeterminate(bool v);

    [[nodiscard]] double min()   const { return min_; }
    [[nodiscard]] double max()   const { return max_; }
    [[nodiscard]] double value() const { return value_; }
    [[nodiscard]] bool   is_indeterminate() const { return indeterminate_; }

    void set_show_text(bool v) { show_text_ = v; invalidate(); }
    void set_text_format(std::string_view fmt) { text_fmt_ = fmt; invalidate(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;

private:
    Orientation  orientation_;
    double       min_{0.0}, max_{1.0}, value_{0.0};
    bool         indeterminate_{false};
    bool         show_text_{false};
    std::string  text_fmt_{"%.0f%%"};
    float        anim_phase_{0.f}; // for indeterminate animation
};

} // namespace swole
