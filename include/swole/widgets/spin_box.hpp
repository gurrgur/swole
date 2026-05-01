#pragma once

#include "../window/widget.hpp"
#include "../core/event.hpp"
#include <string>

namespace swole {

// Numeric value editor with increment / decrement arrow buttons.
class SpinBox : public Widget {
public:
    explicit SpinBox(Widget* parent = nullptr);

    void set_range(double min, double max);
    void set_step(double step)          { step_ = step; }
    void set_value(double v);
    void set_decimals(int d)            { decimals_ = d; invalidate(); }
    void set_prefix(std::string_view p) { prefix_ = p;  invalidate(); }
    void set_suffix(std::string_view s) { suffix_ = s;  invalidate(); }

    [[nodiscard]] double      value()    const { return value_; }
    [[nodiscard]] double      minimum()  const { return min_; }
    [[nodiscard]] double      maximum()  const { return max_; }
    [[nodiscard]] double      step()     const { return step_; }
    [[nodiscard]] int         decimals() const { return decimals_; }
    [[nodiscard]] std::string prefix()   const { return prefix_; }
    [[nodiscard]] std::string suffix()   const { return suffix_; }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<double> on_value_changed;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_release(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_mouse_leave(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;
    void on_text_input(const TextInputEvent& e) override;
    void on_focus_gain(const FocusEvent& e) override;
    void on_focus_loss(const FocusEvent& e) override;

private:
    static constexpr int kBtnW = 20;

    void step_by(int dir);
    void commit_text();
    [[nodiscard]] double clamp(double v) const;
    [[nodiscard]] std::string format_value(double v) const;
    // Returns {up_rect, dn_rect} in local coords
    [[nodiscard]] RectI up_rect() const;
    [[nodiscard]] RectI dn_rect() const;

    double value_{0.}, min_{0.}, max_{100.}, step_{1.};
    int    decimals_{0};
    std::string prefix_, suffix_;

    // Inline text editing state
    bool        editing_{false};
    std::string edit_text_;
    int         cursor_{0};

    bool hovered_up_{false}, hovered_dn_{false};
    bool pressed_up_{false}, pressed_dn_{false};
};

} // namespace swole
