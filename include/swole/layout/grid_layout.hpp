#pragma once

#include "layout.hpp"
#include "../window/widget.hpp"

namespace swole {

struct GridCell {
    Widget* widget{nullptr};
    int     row{0}, col{0};
    int     row_span{1}, col_span{1};
    int     h_stretch{0}, v_stretch{0};
};

// Lays out children in a grid. Rows and columns stretch proportionally to
// their assigned stretch factors (default 1 for all).
class GridLayout : public Layout {
public:
    explicit GridLayout(int cols);

    GridLayout& add_widget(Widget* w, int row, int col,
                           int row_span = 1, int col_span = 1,
                           int h_stretch = 0, int v_stretch = 0);

    void set_column_stretch(int col, int stretch);
    void set_row_stretch(int row, int stretch);
    void set_column_min_width(int col, int min_w);
    void set_row_min_height(int row, int min_h);

    void apply(Widget& owner) override;
    SizeI min_size(const Widget& owner) const override;
    SizeI preferred_size(const Widget& owner) const override;

private:
    int cols_;
    std::vector<GridCell> cells_;
    std::vector<int> col_stretch_;
    std::vector<int> row_stretch_;
    std::vector<int> col_min_w_;
    std::vector<int> row_min_h_;
};

} // namespace swole
