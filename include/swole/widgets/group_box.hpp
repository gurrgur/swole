#pragma once

#include "../window/widget.hpp"
#include <string>

namespace swole {

// A labeled frame that groups related controls.
// Children are laid out inside the inner content area (below the title).
class GroupBox : public Widget {
public:
    explicit GroupBox(Widget* parent = nullptr, std::string_view title = {});

    void set_title(std::string_view t);
    [[nodiscard]] std::string_view title() const { return title_; }

    // Inset from the outer bounds to the content area.
    [[nodiscard]] InsetsI content_insets() const;

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;
    void layout_children() override;

private:
    std::string title_;
};

} // namespace swole
