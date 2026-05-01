#pragma once

#include "../window/widget.hpp"
#include "../render/image.hpp"
#include "../core/event.hpp"
#include <memory>
#include <string>
#include <vector>

namespace swole {

struct ToolBarAction {
    std::string label;
    Image       icon;
    bool        checkable{false};
    bool        checked{false};
    bool        separator{false};  // if true, renders a divider; all other fields ignored
    bool        enabled{true};

    Signal<>    on_triggered;
    Signal<bool> on_toggled;
};

// Horizontal strip of icon/text buttons separated by optional dividers.
class ToolBar : public Widget {
public:
    explicit ToolBar(Widget* parent = nullptr);

    // Add a push action. Returns reference to the action for signal connection.
    ToolBarAction& add_action(std::string_view label, Image icon = {});
    // Add a checkable toggle action.
    ToolBarAction& add_toggle(std::string_view label, Image icon = {});
    // Add a visual separator.
    void add_separator();

    void remove_action(int index);

    [[nodiscard]] int              action_count() const { return int(actions_.size()); }
    [[nodiscard]] ToolBarAction&   action(int i)        { return *actions_[i]; }
    [[nodiscard]] const ToolBarAction& action(int i) const { return *actions_[i]; }

    void set_icon_size(SizeI sz)  { icon_size_ = sz; invalidate(); }
    void set_show_text(bool v)    { show_text_ = v;  invalidate(); }

    [[nodiscard]] SizeI icon_size()  const { return icon_size_; }
    [[nodiscard]] bool  show_text()  const { return show_text_; }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_release(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_mouse_leave(const MouseEvent& e) override;

private:
    static constexpr int kPadX    = 8;
    static constexpr int kPadY    = 4;
    static constexpr int kSepW    = 8;
    static constexpr int kGap     = 4;

    // Returns action index at point (ignoring separators), or -1.
    [[nodiscard]] int action_at(PointI p) const;
    // Left edge of action i's bounding box.
    [[nodiscard]] int action_x(int i) const;
    // Width of action i's bounding box.
    [[nodiscard]] int action_w(int i) const;

    std::vector<std::unique_ptr<ToolBarAction>> actions_;
    SizeI  icon_size_{16, 16};
    bool   show_text_{true};
    int    hovered_{-1};
    int    pressed_{-1};
};

} // namespace swole
