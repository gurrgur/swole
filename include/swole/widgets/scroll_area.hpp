#pragma once

#include "../window/widget.hpp"

namespace swole {

enum class ScrollPolicy { Auto, Always, Never };

// Container that clips its content widget and provides scroll bars.
class ScrollArea : public Widget {
public:
    explicit ScrollArea(Widget* parent = nullptr);

    // Set the widget to scroll. Takes ownership.
    void set_content(std::unique_ptr<Widget> content);
    [[nodiscard]] Widget* content() const { return content_; }

    void set_h_scroll_policy(ScrollPolicy p) { h_policy_ = p; update_bars(); }
    void set_v_scroll_policy(ScrollPolicy p) { v_policy_ = p; update_bars(); }

    void scroll_to(PointI pos);
    void scroll_by(int dx, int dy);
    [[nodiscard]] PointI scroll_pos() const { return scroll_pos_; }

    // True if the scroll area should resize content to fill the viewport
    void set_widget_resizable(bool v) { resizable_ = v; layout_children(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;
    void layout_children() override;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_scroll(const MouseEvent& e) override;

private:
    void update_bars();
    void clamp_scroll();
    [[nodiscard]] RectI viewport_rect() const;
    [[nodiscard]] bool  needs_vbar() const;
    [[nodiscard]] bool  needs_hbar() const;

    Widget*      content_{nullptr};
    PointI       scroll_pos_{0, 0};
    ScrollPolicy h_policy_{ScrollPolicy::Auto};
    ScrollPolicy v_policy_{ScrollPolicy::Auto};
    bool         resizable_{false};

    int bar_size_{12};
};

} // namespace swole
