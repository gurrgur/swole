#include "swole/widgets/list_box.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

ListBox::ListBox(Widget* parent) : Widget(parent), font_(theme::default_font()) {
    set_focus_policy(FocusPolicy::Click);
}

// ── item management ───────────────────────────────────────────────────────────

int ListBox::add_item(std::string_view text, void* user_data) {
    items_.push_back({std::string{text}, user_data});
    invalidate();
    return int(items_.size()) - 1;
}

void ListBox::insert_item(int index, std::string_view text, void* user_data) {
    items_.insert(items_.begin() + index, {std::string{text}, user_data});
    if (current_ >= index) ++current_;
    invalidate();
}

void ListBox::remove_item(int index) {
    items_.erase(items_.begin() + index);
    if (current_ == index)   current_ = -1;
    else if (current_ > index) --current_;
    invalidate();
}

void ListBox::clear() {
    items_.clear();
    current_ = anchor_ = -1;
    scroll_offset_ = 0;
    invalidate();
}

void ListBox::set_item_text(int index, std::string_view text) {
    items_[index].text = text;
    invalidate();
}

void ListBox::set_item_user_data(int index, void* data) {
    items_[index].user_data = data;
}

void ListBox::set_item_enabled(int index, bool enabled) {
    items_[index].enabled = enabled;
    invalidate();
}

// ── selection ─────────────────────────────────────────────────────────────────

void ListBox::set_selection_mode(SelectionMode m) {
    sel_mode_ = m;
    invalidate();
}

void ListBox::select(int index, bool selected) {
    if (index < 0 || index >= int(items_.size())) return;
    if (!items_[index].enabled) return;

    if (sel_mode_ == SelectionMode::Single) {
        // Deselect previous
        for (auto& it : items_) it.selected = false;
        if (selected) {
            items_[index].selected = true;
            current_ = index;
        } else {
            current_ = -1;
        }
    } else if (sel_mode_ == SelectionMode::Multi) {
        items_[index].selected = selected;
        if (selected) current_ = index;
    }
    on_selection_changed.emit(current_);
    invalidate();
}

void ListBox::select_all() {
    if (sel_mode_ != SelectionMode::Multi) return;
    for (auto& it : items_) if (it.enabled) it.selected = true;
    on_selection_changed.emit(current_);
    invalidate();
}

void ListBox::deselect_all() {
    for (auto& it : items_) it.selected = false;
    current_ = -1;
    on_selection_changed.emit(-1);
    invalidate();
}

bool ListBox::is_selected(int index) const {
    if (index < 0 || index >= int(items_.size())) return false;
    return items_[index].selected;
}

std::vector<int> ListBox::selected_indices() const {
    std::vector<int> out;
    for (int i = 0; i < int(items_.size()); ++i)
        if (items_[i].selected) out.push_back(i);
    return out;
}

// ── scrolling ─────────────────────────────────────────────────────────────────

void ListBox::ensure_visible(int index) {
    if (index < 0 || index >= int(items_.size())) return;
    int top    = index * item_height_;
    int bottom = top + item_height_;
    int view_h = size().h;
    if (top < scroll_offset_)
        scroll_offset_ = top;
    else if (bottom > scroll_offset_ + view_h)
        scroll_offset_ = bottom - view_h;
    invalidate();
}

void ListBox::scroll_to_top()    { scroll_offset_ = 0; invalidate(); }
void ListBox::scroll_to_bottom() {
    scroll_offset_ = std::max(0, int(items_.size()) * item_height_ - size().h);
    invalidate();
}

void ListBox::set_font(Font f) { font_ = std::move(f); invalidate(); }

WidgetSizeHint ListBox::size_hint() const {
    int rows = std::min(int(items_.size()), 8);
    return {
        .min_size       = {60, item_height_ * 2},
        .preferred_size = {200, rows * item_height_ + 2},
    };
}

// ── private helpers ───────────────────────────────────────────────────────────

int ListBox::index_at(PointI local_pos) const {
    int i = (local_pos.y + scroll_offset_) / item_height_;
    if (i < 0 || i >= int(items_.size())) return -1;
    return i;
}

RectI ListBox::item_rect(int index) const {
    return {0, index * item_height_ - scroll_offset_, size().w, item_height_};
}

// ── paint ─────────────────────────────────────────────────────────────────────

void ListBox::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::fill(theme::bg));
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::stroke(has_focus() ? theme::border_focus : theme::border));

    auto guard = canvas.scoped_save();
    canvas.clip_rect(r.inset(1.f));

    auto m     = font_.metrics();
    float pad  = 4.f;
    int first  = std::max(0, scroll_offset_ / item_height_);
    int last   = std::min(int(items_.size()),
                          (scroll_offset_ + size().h) / item_height_ + 1);

    for (int i = first; i < last; ++i) {
        const auto& item = items_[i];
        RectI ri = item_rect(i);
        RectF rf{ri};

        if (item.separator) {
            float cy = rf.y + rf.h * .5f;
            canvas.draw_line(rf.x + 4.f, cy, rf.right() - 4.f, cy,
                             Paint::stroke(theme::border));
            continue;
        }

        // Row background
        if (item.selected) {
            canvas.draw_rect(rf, Paint::fill(theme::sel_bg));
        } else if (alternate_rows_ && (i & 1)) {
            canvas.draw_rect(rf, Paint::fill(theme::row_alt));
        }

        // Label
        Color col = !item.enabled ? theme::text_disabled
                  : item.selected ? theme::sel_text
                                  : theme::text;
        canvas.draw_text(item.text,
                         rf.x + pad,
                         rf.y + (rf.h - m.ascent - m.descent) * .5f + m.ascent,
                         font_, Paint::fill(col));
    }
}

// ── events ────────────────────────────────────────────────────────────────────

void ListBox::on_mouse_press(const MouseEvent& e) {
    int idx = index_at(e.pos);
    if (idx < 0 || !items_[idx].enabled) return;

    if (sel_mode_ == SelectionMode::Multi && e.mods.ctrl) {
        items_[idx].selected = !items_[idx].selected;
        current_ = idx;
        anchor_  = idx;
    } else if (sel_mode_ == SelectionMode::Multi && e.mods.shift && anchor_ >= 0) {
        int lo = std::min(anchor_, idx), hi = std::max(anchor_, idx);
        for (int i = 0; i < int(items_.size()); ++i)
            items_[i].selected = (i >= lo && i <= hi);
        current_ = idx;
    } else {
        for (auto& it : items_) it.selected = false;
        items_[idx].selected = true;
        current_ = anchor_ = idx;
    }
    on_selection_changed.emit(current_);
    invalidate();
}

void ListBox::on_mouse_move(const MouseEvent& e) {
    // Hover highlight is implicit from paint; could track hovered_ here
    (void)e;
}

void ListBox::on_double_click(const MouseEvent& e) {
    int idx = index_at(e.pos);
    if (idx >= 0 && items_[idx].enabled)
        on_item_activated.emit(idx);
}

void ListBox::on_key_press(const KeyEvent& e) {
    if (items_.empty()) return;
    int next = current_;
    switch (e.key) {
    case Key::Up:
        next = (current_ <= 0) ? 0 : current_ - 1;
        break;
    case Key::Down:
        next = std::min(current_ + 1, int(items_.size()) - 1);
        break;
    case Key::Home:   next = 0; break;
    case Key::End:    next = int(items_.size()) - 1; break;
    case Key::Return: case Key::Space:
        if (current_ >= 0) on_item_activated.emit(current_);
        return;
    default: return;
    }
    if (next != current_) {
        select(next);
        ensure_visible(next);
    }
}

void ListBox::on_mouse_scroll(const MouseEvent& e) {
    scroll_offset_ = std::clamp(
        scroll_offset_ - int(e.wheel.y * float(item_height_)),
        0,
        std::max(0, int(items_.size()) * item_height_ - size().h));
    invalidate();
}

} // namespace swole
