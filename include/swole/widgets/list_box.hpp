#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <functional>
#include <string>
#include <vector>

namespace swole {

struct ListItem {
    std::string text;
    void*       user_data{nullptr};
    bool        enabled{true};
    bool        selected{false};
    bool        separator{false};
};

class ListBox : public Widget {
public:
    explicit ListBox(Widget* parent = nullptr);

    // Items
    int  add_item(std::string_view text, void* user_data = nullptr);
    void insert_item(int index, std::string_view text, void* user_data = nullptr);
    void remove_item(int index);
    void clear();

    [[nodiscard]] int   item_count() const { return int(items_.size()); }
    [[nodiscard]] const ListItem& item(int index) const { return items_[index]; }
    void  set_item_text(int index, std::string_view text);
    void  set_item_user_data(int index, void* data);
    void  set_item_enabled(int index, bool enabled);

    // Selection
    void set_selection_mode(SelectionMode m);
    void select(int index, bool selected = true);
    void select_all();
    void deselect_all();
    [[nodiscard]] int  current_index() const { return current_; }
    [[nodiscard]] bool is_selected(int index) const;
    [[nodiscard]] std::vector<int> selected_indices() const;

    // Scroll
    void ensure_visible(int index);
    void scroll_to_top();
    void scroll_to_bottom();

    // Appearance
    void set_item_height(int h) { item_height_ = h; invalidate(); }
    void set_font(Font font);
    void set_alternate_row_colors(bool v) { alternate_rows_ = v; invalidate(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    // Signals
    Signal<int>  on_selection_changed;  // index of newly selected item
    Signal<int>  on_item_activated;     // double-click or Enter
    Signal<int, PointI> on_context_menu; // right-click; index may be -1

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_double_click(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;
    void on_mouse_scroll(const MouseEvent& e) override;

private:
    [[nodiscard]] int index_at(PointI local_pos) const;
    [[nodiscard]] RectI item_rect(int index) const;

    std::vector<ListItem> items_;
    SelectionMode  sel_mode_{SelectionMode::Single};
    int  current_{-1};
    int  scroll_offset_{0};
    int  item_height_{20};
    bool alternate_rows_{false};
    Font font_;

    // Multi-select anchor for shift-click range selection
    int anchor_{-1};
};

} // namespace swole
