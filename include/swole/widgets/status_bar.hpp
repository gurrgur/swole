#pragma once

#include "../window/widget.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace swole {

// Thin horizontal bar (typically docked at the window bottom) for displaying
// transient status messages and permanent informational sections.
class StatusBar : public Widget {
public:
    explicit StatusBar(Widget* parent = nullptr);

    // Show a temporary message. Clears after timeout_ms (0 = indefinite).
    void show_message(std::string_view msg, int timeout_ms = 3000);
    void clear_message();

    // Permanent sections shown on the right, ordered left-to-right.
    // Returns the section index.
    int  add_section(std::string_view text, int min_width = 80);
    void set_section_text(int index, std::string_view text);
    void remove_section(int index);

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;

private:
    struct Section { std::string text; int min_width; };

    std::string          message_;
    uint64_t             message_until_ms_{0}; // SDL tick when message expires; 0=never

    std::vector<Section> sections_;

    void tick_message();
};

} // namespace swole
