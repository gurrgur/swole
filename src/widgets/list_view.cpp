#include "swole/widgets/list_view.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

static constexpr int kSortArrowW = 14;

ListView::ListView(Widget* parent) : Widget(parent), font_(theme::default_font()) {
    header_font_ = theme::default_font().bold();
    set_focus_policy(FocusPolicy::Click);
}

// ── columns ───────────────────────────────────────────────────────────────────

int ListView::add_column(ListViewColumn col) {
    columns_.push_back(std::move(col));
    // Make sure every existing item has a cell for the new column
    for (auto& it : items_)
        if (int(it.cells.size()) < int(columns_.size()))
            it.cells.resize(columns_.size());
    invalidate();
    return int(columns_.size()) - 1;
}

void ListView::remove_column(int col_index) {
    columns_.erase(columns_.begin() + col_index);
    for (auto& it : items_) {
        if (col_index < int(it.cells.size()))
            it.cells.erase(it.cells.begin() + col_index);
    }
    invalidate();
}

void ListView::set_column(int col_index, const ListViewColumn& col) {
    columns_[col_index] = col;
    invalidate();
}

// ── items ─────────────────────────────────────────────────────────────────────

int ListView::add_item(std::vector<std::string> cells, void* user_data) {
    cells.resize(columns_.size()); // pad to column count
    items_.push_back({std::move(cells), user_data});
    invalidate();
    return int(items_.size()) - 1;
}

void ListView::remove_item(int index) {
    items_.erase(items_.begin() + index);
    if (current_ == index)   current_ = -1;
    else if (current_ > index) --current_;
    invalidate();
}

void ListView::clear_items() {
    items_.clear();
    current_ = -1;
    scroll_offset_y_ = scroll_offset_x_ = 0;
    invalidate();
}

void ListView::set_cell_text(int row, int col, std::string_view text) {
    items_[row].cells[col] = text;
    invalidate();
}

// ── selection ─────────────────────────────────────────────────────────────────

void ListView::select_item(int index, bool selected) {
    if (index < 0 || index >= int(items_.size())) return;
    if (sel_mode_ == SelectionMode::Single) {
        for (auto& it : items_) it.selected = false;
        if (selected) items_[index].selected = true;
        current_ = selected ? index : -1;
    } else {
        items_[index].selected = selected;
        if (selected) current_ = index;
    }
    on_selection_changed.emit(current_);
    invalidate();
}

void ListView::select_all() {
    if (sel_mode_ != SelectionMode::Multi) return;
    for (auto& it : items_) it.selected = true;
    invalidate();
}

void ListView::deselect_all() {
    for (auto& it : items_) it.selected = false;
    current_ = -1;
    on_selection_changed.emit(-1);
    invalidate();
}

std::vector<int> ListView::selected_indices() const {
    std::vector<int> out;
    for (int i = 0; i < int(items_.size()); ++i)
        if (items_[i].selected) out.push_back(i);
    return out;
}

// ── sort ──────────────────────────────────────────────────────────────────────

void ListView::sort_by_column(int col, ListViewSortOrder order) {
    sort_col_   = col;
    sort_order_ = order;
    if (order == ListViewSortOrder::None) { invalidate(); return; }
    std::stable_sort(items_.begin(), items_.end(), [&](const auto& a, const auto& b) {
        std::string_view av = (col < int(a.cells.size())) ? a.cells[col] : std::string_view{};
        std::string_view bv = (col < int(b.cells.size())) ? b.cells[col] : std::string_view{};
        return order == ListViewSortOrder::Ascending ? av < bv : av > bv;
    });
    invalidate();
}

void ListView::set_sort_indicator(int col, ListViewSortOrder order) {
    sort_col_   = col;
    sort_order_ = order;
    invalidate();
}

// ── geometry helpers ──────────────────────────────────────────────────────────

RectI ListView::header_rect() const {
    return {0, 0, size().w, show_header_ ? header_height_ : 0};
}

RectI ListView::row_rect(int index) const {
    int y = (show_header_ ? header_height_ : 0) + index * row_height_ - scroll_offset_y_;
    return {-scroll_offset_x_, y, total_col_width(), row_height_};
}

RectI ListView::cell_rect(int row, int col) const {
    RectI rr = row_rect(row);
    int x = rr.x;
    for (int c = 0; c < col && c < int(columns_.size()); ++c)
        x += columns_[c].width;
    int w = (col < int(columns_.size())) ? columns_[col].width : 0;
    return {x, rr.y, w, row_height_};
}

int ListView::total_col_width() const {
    int w = 0;
    for (auto& c : columns_) w += c.width;
    return w;
}

int ListView::row_at(PointI p) const {
    int top_y = show_header_ ? header_height_ : 0;
    if (p.y < top_y) return -1;
    int idx = (p.y - top_y + scroll_offset_y_) / row_height_;
    if (idx < 0 || idx >= int(items_.size())) return -1;
    return idx;
}

int ListView::col_at_x(int x) const {
    int cx = -scroll_offset_x_;
    for (int c = 0; c < int(columns_.size()); ++c) {
        if (x >= cx && x < cx + columns_[c].width) return c;
        cx += columns_[c].width;
    }
    return -1;
}

void ListView::ensure_visible(int index) {
    if (index < 0 || index >= int(items_.size())) return;
    int top_y = show_header_ ? header_height_ : 0;
    int row_top = index * row_height_;
    int row_bot = row_top + row_height_;
    int view_h  = size().h - top_y;
    if (row_top < scroll_offset_y_)
        scroll_offset_y_ = row_top;
    else if (row_bot > scroll_offset_y_ + view_h)
        scroll_offset_y_ = row_bot - view_h;
    invalidate();
}

WidgetSizeHint ListView::size_hint() const {
    int rows = std::min(int(items_.size()), 8);
    int h = rows * row_height_ + (show_header_ ? header_height_ : 0) + 2;
    int w = std::max(200, total_col_width() + 2);
    return {.min_size = {60, 40}, .preferred_size = {w, h}};
}

void ListView::set_font(Font f)        { font_        = std::move(f); invalidate(); }
void ListView::set_header_font(Font f) { header_font_ = std::move(f); invalidate(); }

// ── paint ─────────────────────────────────────────────────────────────────────

void ListView::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    canvas.draw_rect(r, Paint::fill(theme::bg));
    canvas.draw_rect(r.inset(.5f), Paint::stroke(has_focus() ? theme::border_focus : theme::border));

    auto guard = canvas.scoped_save();
    canvas.clip_rect(r.inset(1.f));

    float pad = 4.f;

    // ── Header ────────────────────────────────────────────────────────────────
    if (show_header_) {
        RectF hr{header_rect()};
        canvas.draw_rect(hr, Paint::fill(theme::header_bg));
        canvas.draw_line(hr.x, hr.bottom() - .5f, hr.right(), hr.bottom() - .5f,
                         Paint::stroke(theme::border));

        int col_x = -scroll_offset_x_;
        for (int c = 0; c < int(columns_.size()); ++c) {
            const auto& col = columns_[c];
            RectF cr{float(col_x), 0.f, float(col.width), float(header_height_)};
            canvas.draw_text(col.title, cr.inset(pad, 0.f),
                             col.align, TextBaseline::Middle,
                             header_font_, Paint::fill(theme::text));

            // Sort indicator
            if (c == sort_col_ && sort_order_ != ListViewSortOrder::None) {
                float cx = cr.right() - float(kSortArrowW) * .5f;
                float cy = cr.y + cr.h * .5f;
                const float sz = 4.f;
                if (sort_order_ == ListViewSortOrder::Ascending) {
                    PointF pts[] = {{cx - sz, cy + sz*.4f},
                                    {cx, cy - sz*.6f},
                                    {cx + sz, cy + sz*.4f}};
                    canvas.draw_polygon(pts, true, Paint::fill(theme::accent));
                } else {
                    PointF pts[] = {{cx - sz, cy - sz*.4f},
                                    {cx, cy + sz*.6f},
                                    {cx + sz, cy - sz*.4f}};
                    canvas.draw_polygon(pts, true, Paint::fill(theme::accent));
                }
            }

            // Column separator
            if (c + 1 < int(columns_.size()))
                canvas.draw_line(cr.right() - .5f, cr.y + 3.f,
                                 cr.right() - .5f, cr.bottom() - 3.f,
                                 Paint::stroke(theme::border));
            col_x += col.width;
        }
    }

    // ── Rows ──────────────────────────────────────────────────────────────────
    int top_y  = show_header_ ? header_height_ : 0;
    int first  = std::max(0, scroll_offset_y_ / row_height_);
    int last   = std::min(int(items_.size()),
                         (scroll_offset_y_ + size().h - top_y) / row_height_ + 1);

    for (int i = first; i < last; ++i) {
        const auto& item = items_[i];
        RectF rr{row_rect(i)};

        // Row background
        if (item.selected) {
            canvas.draw_rect(rr, Paint::fill(theme::sel_bg));
        } else if (alternate_rows_ && (i & 1)) {
            canvas.draw_rect(rr, Paint::fill(theme::row_alt));
        }

        // Row bottom line
        canvas.draw_line(rr.x, rr.bottom() - .5f, rr.right(), rr.bottom() - .5f,
                         Paint::stroke(Color{235, 236, 238}));

        // Cells
        float cx = rr.x;
        for (int c = 0; c < int(columns_.size()) && c < int(item.cells.size()); ++c) {
            RectF cell{cx, rr.y, float(columns_[c].width), rr.h};
            Color col = !item.enabled  ? theme::text_disabled
                      : item.selected  ? theme::sel_text
                                       : theme::text;
            canvas.draw_text(item.cells[c], cell.inset(pad, 0.f),
                             columns_[c].align, TextBaseline::Middle,
                             font_, Paint::fill(col));
            cx += float(columns_[c].width);
        }
    }
}

// ── events ────────────────────────────────────────────────────────────────────

void ListView::on_mouse_press(const MouseEvent& e) {
    // Column resize?
    if (show_header_ && e.pos.y < header_height_) {
        int cx = -scroll_offset_x_;
        for (int c = 0; c < int(columns_.size()); ++c) {
            cx += columns_[c].width;
            if (std::abs(e.pos.x - cx) <= 4 && columns_[c].resizable) {
                resize_col_         = c;
                resize_start_x_     = e.pos.x;
                resize_col_start_w_ = columns_[c].width;
                return;
            }
        }
        // Header click → sort
        int col = col_at_x(e.pos.x);
        if (col >= 0 && columns_[col].sortable) {
            auto next = (sort_col_ == col && sort_order_ == ListViewSortOrder::Ascending)
                        ? ListViewSortOrder::Descending : ListViewSortOrder::Ascending;
            on_sort_requested.emit(col, next);
            sort_by_column(col, next);
        }
        return;
    }

    int idx = row_at(e.pos);
    if (idx < 0) { deselect_all(); return; }

    if (sel_mode_ == SelectionMode::Multi && e.mods.ctrl) {
        items_[idx].selected = !items_[idx].selected;
        current_ = idx;
    } else {
        for (auto& it : items_) it.selected = false;
        items_[idx].selected = true;
        current_ = idx;
    }
    on_selection_changed.emit(current_);
    invalidate();
}

void ListView::on_mouse_release(const MouseEvent& e) {
    if (e.button == MouseButton::Left) resize_col_ = -1;
}

void ListView::on_mouse_move(const MouseEvent& e) {
    if (resize_col_ >= 0) {
        int delta = e.pos.x - resize_start_x_;
        int new_w = std::max(columns_[resize_col_].min_width,
                             resize_col_start_w_ + delta);
        columns_[resize_col_].width = new_w;
        invalidate();
    }
}

void ListView::on_double_click(const MouseEvent& e) {
    int idx = row_at(e.pos);
    if (idx >= 0) on_item_activated.emit(idx);
}

void ListView::on_key_press(const KeyEvent& e) {
    if (items_.empty()) return;
    int next = current_;
    switch (e.key) {
    case Key::Up:     next = std::max(0, current_ - 1); break;
    case Key::Down:   next = std::min(int(items_.size()) - 1, current_ + 1); break;
    case Key::Home:   next = 0; break;
    case Key::End:    next = int(items_.size()) - 1; break;
    case Key::Return: if (current_ >= 0) on_item_activated.emit(current_); return;
    default: return;
    }
    if (next != current_) { select_item(next); ensure_visible(next); }
}

void ListView::on_mouse_scroll(const MouseEvent& e) {
    int max_y = std::max(0, int(items_.size()) * row_height_ - size().h +
                            (show_header_ ? header_height_ : 0));
    scroll_offset_y_ = std::clamp(
        scroll_offset_y_ - int(e.wheel.y * float(row_height_)), 0, max_y);
    invalidate();
}

} // namespace swole
