#pragma once

#include "../window/widget.hpp"

namespace swole {

class Slider : public Widget {
public:
    explicit Slider(Widget* parent = nullptr,
                    Orientation orientation = Orientation::Horizontal);

    void set_range(double min, double max);
    void set_value(double v);
    void set_step(double step) { step_ = step; }
    void set_page_step(double step) { page_step_ = step; }
    void set_snap_to_tick(bool v) { snap_ = v; }

    [[nodiscard]] double min()      const { return min_; }
    [[nodiscard]] double max()      const { return max_; }
    [[nodiscard]] double value()    const { return value_; }
    [[nodiscard]] double step()     const { return step_; }
    [[nodiscard]] Orientation orientation() const { return orientation_; }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<double> on_value_changed;
    Signal<double> on_slide_started;
    Signal<double> on_slide_finished;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_release(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;

private:
    [[nodiscard]] RectF  track_rect()  const;
    [[nodiscard]] RectF  thumb_rect()  const;
    [[nodiscard]] double value_at(PointF pos) const;

    Orientation orientation_;
    double min_{0.0}, max_{1.0}, value_{0.0};
    double step_{0.01}, page_step_{0.1};
    bool   snap_{false};
    bool   dragging_{false};
    float  drag_offset_{0.f};
};

} // namespace swole
