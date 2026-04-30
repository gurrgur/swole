#include "swole/layout/grid_layout.hpp"
#include "swole/window/widget.hpp"

#include <algorithm>
#include <numeric>

namespace swole {

GridLayout::GridLayout(int cols) : cols_{cols} {}

GridLayout& GridLayout::add_widget(Widget* w, int row, int col,
                                   int row_span, int col_span,
                                   int h_stretch, int v_stretch) {
    cells_.push_back({w, row, col, row_span, col_span, h_stretch, v_stretch});
    return *this;
}

void GridLayout::set_column_stretch(int col, int stretch) {
    if (col >= int(col_stretch_.size())) col_stretch_.resize(col + 1, 1);
    col_stretch_[col] = stretch;
}

void GridLayout::set_row_stretch(int row, int stretch) {
    if (row >= int(row_stretch_.size())) row_stretch_.resize(row + 1, 1);
    row_stretch_[row] = stretch;
}

void GridLayout::set_column_min_width(int col, int min_w) {
    if (col >= int(col_min_w_.size())) col_min_w_.resize(col + 1, 0);
    col_min_w_[col] = min_w;
}

void GridLayout::set_row_min_height(int row, int min_h) {
    if (row >= int(row_min_h_.size())) row_min_h_.resize(row + 1, 0);
    row_min_h_[row] = min_h;
}

// ── geometry resolution ───────────────────────────────────────────────────────

void GridLayout::apply(Widget& owner) {
    if (cells_.empty()) return;

    // Determine grid extents
    int num_rows = 0, num_cols = cols_;
    for (auto& c : cells_) {
        num_rows = std::max(num_rows, c.row + c.row_span);
        num_cols = std::max(num_cols, c.col + c.col_span);
    }
    if (num_rows == 0 || num_cols == 0) return;

    RectI bounds = owner.local_bounds();
    int avail_w = bounds.w - margin_.h_total() - spacing_ * (num_cols - 1);
    int avail_h = bounds.h - margin_.v_total() - spacing_ * (num_rows - 1);

    // Column widths
    std::vector<int> col_w(num_cols, 0);
    // Apply min widths
    for (int c = 0; c < num_cols; ++c) {
        if (c < int(col_min_w_.size())) col_w[c] = col_min_w_[c];
        else col_w[c] = 0;
    }
    // Preferred widths from single-cell widgets
    for (auto& cell : cells_) {
        if (!cell.widget || cell.col_span != 1) continue;
        auto hint = cell.widget->size_hint();
        col_w[cell.col] = std::max(col_w[cell.col], hint.preferred_size.w);
    }
    // Distribute remaining space by stretch
    int fixed_w = std::accumulate(col_w.begin(), col_w.end(), 0);
    int flex_w  = std::max(0, avail_w - fixed_w);
    int stretch_sum = 0;
    for (int c = 0; c < num_cols; ++c)
        stretch_sum += (c < int(col_stretch_.size()) ? col_stretch_[c] : 1);
    if (stretch_sum > 0) {
        int given = 0;
        for (int c = 0; c < num_cols; ++c) {
            int s = (c < int(col_stretch_.size()) ? col_stretch_[c] : 1);
            int extra = (c == num_cols - 1) ? flex_w - given
                                            : flex_w * s / stretch_sum;
            col_w[c] += extra;
            given     += extra;
        }
    }

    // Row heights
    std::vector<int> row_h(num_rows, 0);
    for (int r = 0; r < num_rows; ++r) {
        if (r < int(row_min_h_.size())) row_h[r] = row_min_h_[r];
    }
    for (auto& cell : cells_) {
        if (!cell.widget || cell.row_span != 1) continue;
        auto hint = cell.widget->size_hint();
        row_h[cell.row] = std::max(row_h[cell.row], hint.preferred_size.h);
    }
    int fixed_h = std::accumulate(row_h.begin(), row_h.end(), 0);
    int flex_h  = std::max(0, avail_h - fixed_h);
    int v_stretch_sum = 0;
    for (int r = 0; r < num_rows; ++r)
        v_stretch_sum += (r < int(row_stretch_.size()) ? row_stretch_[r] : 1);
    if (v_stretch_sum > 0) {
        int given = 0;
        for (int r = 0; r < num_rows; ++r) {
            int s = (r < int(row_stretch_.size()) ? row_stretch_[r] : 1);
            int extra = (r == num_rows - 1) ? flex_h - given
                                            : flex_h * s / v_stretch_sum;
            row_h[r] += extra;
            given     += extra;
        }
    }

    // Compute cumulative column X offsets and row Y offsets
    std::vector<int> col_x(num_cols), row_y(num_rows);
    col_x[0] = bounds.x + margin_.left;
    for (int c = 1; c < num_cols; ++c)
        col_x[c] = col_x[c - 1] + col_w[c - 1] + spacing_;
    row_y[0] = bounds.y + margin_.top;
    for (int r = 1; r < num_rows; ++r)
        row_y[r] = row_y[r - 1] + row_h[r - 1] + spacing_;

    // Place widgets
    for (auto& cell : cells_) {
        if (!cell.widget || !cell.widget->is_visible()) continue;
        int x = col_x[cell.col];
        int y = row_y[cell.row];
        int w = 0, h = 0;
        for (int c = cell.col; c < cell.col + cell.col_span && c < num_cols; ++c)
            w += col_w[c] + (c > cell.col ? spacing_ : 0);
        for (int r = cell.row; r < cell.row + cell.row_span && r < num_rows; ++r)
            h += row_h[r] + (r > cell.row ? spacing_ : 0);
        cell.widget->set_bounds({x, y, w, h});
    }
}

SizeI GridLayout::min_size(const Widget& /*owner*/) const {
    // Simple pass: sum of min-widths + spacing
    int num_rows = 0, num_cols = cols_;
    for (auto& c : cells_) {
        num_rows = std::max(num_rows, c.row + c.row_span);
        num_cols = std::max(num_cols, c.col + c.col_span);
    }
    std::vector<int> cw(num_cols, 0), rh(num_rows, 0);
    for (auto& cell : cells_) {
        if (!cell.widget) continue;
        auto hint = cell.widget->size_hint();
        if (cell.col_span == 1) cw[cell.col] = std::max(cw[cell.col], hint.min_size.w);
        if (cell.row_span == 1) rh[cell.row] = std::max(rh[cell.row], hint.min_size.h);
    }
    int w = std::accumulate(cw.begin(), cw.end(), 0) + spacing_ * (num_cols - 1) + margin_.h_total();
    int h = std::accumulate(rh.begin(), rh.end(), 0) + spacing_ * (num_rows - 1) + margin_.v_total();
    return {w, h};
}

SizeI GridLayout::preferred_size(const Widget& /*owner*/) const {
    int num_rows = 0, num_cols = cols_;
    for (auto& c : cells_) {
        num_rows = std::max(num_rows, c.row + c.row_span);
        num_cols = std::max(num_cols, c.col + c.col_span);
    }
    std::vector<int> cw(num_cols, 0), rh(num_rows, 0);
    for (auto& cell : cells_) {
        if (!cell.widget) continue;
        auto hint = cell.widget->size_hint();
        if (cell.col_span == 1) cw[cell.col] = std::max(cw[cell.col], hint.preferred_size.w);
        if (cell.row_span == 1) rh[cell.row] = std::max(rh[cell.row], hint.preferred_size.h);
    }
    int w = std::accumulate(cw.begin(), cw.end(), 0) + spacing_ * (num_cols - 1) + margin_.h_total();
    int h = std::accumulate(rh.begin(), rh.end(), 0) + spacing_ * (num_rows - 1) + margin_.v_total();
    return {w, h};
}

} // namespace swole
