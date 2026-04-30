#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <functional>
#include <string>
#include <vector>

namespace swole {

enum class ListViewSortOrder { None, Ascending, Descending };

struct ListViewColumn {
    std::string title;
    int         width{100};
    int         min_width{40};
    bool        resizable{true};
    bool        sortable{true};
    TextAlign   align{TextAlign::Left};
};

struct ListViewItem {
    std::vector<std::string> cells; // one per column
    void* user_data{nullptr};
    bool  selected{false};
    bool  enabled{true};
};

// Multi-column report-view list (replaces ListView in Win32).
class ListView : public Widget {
public:
    explicit ListView(Widget* parent = nullptr);

    // Columns
    int  add_column(ListViewColumn col);
    void remove_column(int col_index);
    void set_column(int col_index, const ListViewColumn& col);
    [[nodiscard]] int column_count() const { return int(columns_.size()); }

    // Items
    int  add_item(std::vector<std::string> cells, void* user_data = nullptr);
    void remove_item(int index);
    void clear_items();
    void set_cell_text(int row, int col, std::string_view text);
    [[nodiscard]] const ListViewItem& item(int index) const { return items_[index]; }
    [[nodiscard]] int item_count() const { return int(items_.size()); }

    // Selection
    void set_selection_mode(SelectionMode m) { sel_mode_ = m; }
    void select_item(int index, bool selected = true);
    void select_all();
    void deselect_all();
    [[nodiscard]] int  current_index() const { return current_; }
    [[nodiscard]] std::vector<int> selected_indices() const;

    // Sorting
    void sort_by_column(int col, ListViewSortOrder order);
    void set_sort_indicator(int col, ListViewSortOrder order);

    // Appearance
    void set_show_header(bool v) { show_header_ = v; layout_children(); }
    void set_row_height(int h)   { row_height_ = h; invalidate(); }
    void set_alternate_row_colors(bool v) { alternate_rows_ = v; invalidate(); }
    void set_font(Font font);
    void set_header_font(Font font);

    void ensure_visible(int index);

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<int>           on_selection_changed;
    Signal<int>           on_item_activated;
    Signal<int, PointI>   on_context_menu;
    Signal<int, ListViewSortOrder> on_sort_requested;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_double_click(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;
    void on_mouse_scroll(const MouseEvent& e) override;

private:
    [[nodiscard]] RectI header_rect() const;
    [[nodiscard]] RectI row_rect(int index) const;
    [[nodiscard]] RectI cell_rect(int row, int col) const;
    [[nodiscard]] int   row_at(PointI p) const;
    [[nodiscard]] int   col_at_x(int x) const;

    std::vector<ListViewColumn> columns_;
    std::vector<ListViewItem>   items_;

    SelectionMode  sel_mode_{SelectionMode::Single};
    int            current_{-1};
    int            scroll_offset_y_{0};
    int            scroll_offset_x_{0};
    int            row_height_{22};
    int            header_height_{24};
    int            sort_col_{-1};
    ListViewSortOrder sort_order_{ListViewSortOrder::None};
    bool           show_header_{true};
    bool           alternate_rows_{true};
    Font           font_;
    Font           header_font_;

    // Column drag resize state
    int  resize_col_{-1};
    int  resize_start_x_{0};
    int  resize_col_start_w_{0};
};

} // namespace swole
