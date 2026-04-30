#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <string>
#include <vector>

namespace swole {

class ComboBox : public Widget {
public:
    explicit ComboBox(Widget* parent = nullptr);

    int  add_item(std::string_view text, void* user_data = nullptr);
    void remove_item(int index);
    void clear();

    [[nodiscard]] int  item_count()  const { return int(items_.size()); }
    [[nodiscard]] std::string_view item_text(int index) const;
    [[nodiscard]] void* item_user_data(int index) const;

    void set_current_index(int index);
    void set_current_text(std::string_view text);
    [[nodiscard]] int  current_index() const { return current_; }
    [[nodiscard]] std::string_view current_text() const;

    // Allow typing in the combo box
    void set_editable(bool v) { editable_ = v; invalidate(); }
    [[nodiscard]] bool is_editable() const { return editable_; }
    [[nodiscard]] std::string_view edit_text() const { return edit_text_; }

    void set_font(Font font);

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<int>         on_index_changed;
    Signal<std::string_view> on_text_changed;  // only when editable

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;

private:
    struct Item { std::string text; void* user_data{nullptr}; };
    std::vector<Item> items_;
    int    current_{-1};
    bool   editable_{false};
    bool   popup_open_{false};
    std::string edit_text_;
    Font   font_;
};

} // namespace swole
