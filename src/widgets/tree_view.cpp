#include "swole/widgets/tree_view.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

// ── TreeItem ──────────────────────────────────────────────────────────────────

TreeItem* TreeItem::add_child(std::string_view t, void* data) {
    auto child = std::make_unique<TreeItem>();
    child->text      = t;
    child->user_data = data;
    child->parent    = this;
    TreeItem* raw = child.get();
    children.push_back(std::move(child));
    return raw;
}

// ── TreeView ──────────────────────────────────────────────────────────────────

TreeView::TreeView(Widget* parent) : Widget(parent), font_(theme::default_font()) {
    set_focus_policy(FocusPolicy::Click);
}

TreeItem* TreeView::add_root_item(std::string_view text, void* user_data) {
    auto item = std::make_unique<TreeItem>();
    item->text      = text;
    item->user_data = user_data;
    TreeItem* raw = item.get();
    roots_.push_back(std::move(item));
    flat_dirty_ = true;
    invalidate();
    return raw;
}

void TreeView::remove_item(TreeItem* item) {
    // Find in roots or parent's children
    if (!item->parent) {
        auto it = std::find_if(roots_.begin(), roots_.end(),
                               [item](const auto& p) { return p.get() == item; });
        if (it != roots_.end()) roots_.erase(it);
    } else {
        auto& sib = item->parent->children;
        auto  it  = std::find_if(sib.begin(), sib.end(),
                                 [item](const auto& p) { return p.get() == item; });
        if (it != sib.end()) sib.erase(it);
    }
    if (selected_ == item) selected_ = nullptr;
    flat_dirty_ = true;
    invalidate();
}

void TreeView::clear() {
    roots_.clear();
    selected_ = nullptr;
    scroll_offset_ = 0;
    flat_dirty_ = true;
    invalidate();
}

// ── selection ─────────────────────────────────────────────────────────────────

void TreeView::select(TreeItem* item) {
    if (selected_ == item) return;
    selected_ = item;
    on_selection_changed.emit(item);
    invalidate();
}

void TreeView::deselect_all() {
    selected_ = nullptr;
    on_selection_changed.emit(nullptr);
    invalidate();
}

// ── expand / collapse ─────────────────────────────────────────────────────────

void TreeView::expand(TreeItem* item) {
    if (!item || item->children.empty() || item->expanded) return;
    item->expanded = true;
    flat_dirty_ = true;
    on_item_expanded.emit(item);
    invalidate();
}

void TreeView::collapse(TreeItem* item) {
    if (!item || !item->expanded) return;
    item->expanded = false;
    flat_dirty_ = true;
    on_item_collapsed.emit(item);
    invalidate();
}

void TreeView::expand_all() {
    std::function<void(TreeItem*)> recurse = [&](TreeItem* it) {
        it->expanded = true;
        for (auto& c : it->children) recurse(c.get());
    };
    for (auto& r : roots_) recurse(r.get());
    flat_dirty_ = true;
    invalidate();
}

void TreeView::collapse_all() {
    std::function<void(TreeItem*)> recurse = [&](TreeItem* it) {
        it->expanded = false;
        for (auto& c : it->children) recurse(c.get());
    };
    for (auto& r : roots_) recurse(r.get());
    flat_dirty_ = true;
    invalidate();
}

void TreeView::ensure_visible(TreeItem* item) {
    if (!item) return;
    if (flat_dirty_) build_flat_list();
    for (int i = 0; i < int(flat_.size()); ++i) {
        if (flat_[i].item == item) {
            int top    = i * item_height_;
            int bottom = top + item_height_;
            if (top < scroll_offset_)
                scroll_offset_ = top;
            else if (bottom > scroll_offset_ + size().h)
                scroll_offset_ = bottom - size().h;
            invalidate();
            return;
        }
    }
}

void TreeView::set_font(Font f) { font_ = std::move(f); invalidate(); }

WidgetSizeHint TreeView::size_hint() const {
    return {.min_size = {80, 60}, .preferred_size = {200, 300}};
}

// ── flat list ─────────────────────────────────────────────────────────────────

void TreeView::build_flat_list() {
    flat_.clear();
    std::function<void(TreeItem*, int)> walk = [&](TreeItem* it, int depth) {
        flat_.push_back({it, depth});
        if (it->expanded)
            for (auto& c : it->children) walk(c.get(), depth + 1);
    };
    for (auto& r : roots_) walk(r.get(), 0);
    flat_dirty_ = false;
}

int TreeView::flat_index_at(PointI local_pos) const {
    int i = (local_pos.y + scroll_offset_) / item_height_;
    if (i < 0 || i >= int(flat_.size())) return -1;
    return i;
}

// ── paint ─────────────────────────────────────────────────────────────────────

void TreeView::on_paint(Canvas& canvas) {
    if (flat_dirty_) build_flat_list();

    RectF r{local_bounds()};
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::fill(theme::bg));
    canvas.draw_round_rect(r.inset(.5f), theme::radius, theme::radius,
                           Paint::stroke(has_focus() ? theme::border_focus : theme::border));

    auto guard = canvas.scoped_save();
    canvas.clip_rect(r.inset(1.f));

    auto m   = font_.metrics();
    float pad = 4.f;

    int first = std::max(0, scroll_offset_ / item_height_);
    int last  = std::min(int(flat_.size()),
                         (scroll_offset_ + size().h) / item_height_ + 1);

    for (int i = first; i < last; ++i) {
        TreeItem* item  = flat_[i].item;
        int       depth = flat_[i].depth;

        float iy = float(i * item_height_ - scroll_offset_);
        float ih = float(item_height_);
        float ix = float(depth * indent_) + pad;

        // Selection
        if (item == selected_)
            canvas.draw_rect({0.f, iy, r.w, ih}, Paint::fill(theme::sel_bg));

        // Expand / collapse triangle
        bool has_children = !item->children.empty();
        if (has_children) {
            float cx = ix + 6.f;
            float cy = iy + ih * .5f;
            const float sz = 5.f;
            if (item->expanded) {
                // Pointing down
                PointF pts[] = {{cx - sz, cy - sz * .5f},
                                {cx + sz, cy - sz * .5f},
                                {cx,      cy + sz * .6f}};
                canvas.draw_polygon(pts, true, Paint::fill(theme::text_dim));
            } else {
                // Pointing right
                PointF pts[] = {{cx - sz * .5f, cy - sz},
                                {cx - sz * .5f, cy + sz},
                                {cx + sz * .6f, cy}};
                canvas.draw_polygon(pts, true, Paint::fill(theme::text_dim));
            }
        }

        // Label
        float text_x = ix + float(indent_);
        float base_y = iy + (ih - m.ascent - m.descent) * .5f + m.ascent;
        Color col = !item->enabled  ? theme::text_disabled
                  : item == selected_ ? theme::sel_text
                                      : theme::text;
        canvas.draw_text(item->text, text_x, base_y, font_, Paint::fill(col));
    }
}

// ── events ────────────────────────────────────────────────────────────────────

void TreeView::on_mouse_press(const MouseEvent& e) {
    if (flat_dirty_) build_flat_list();
    int idx = flat_index_at(e.pos);
    if (idx < 0) return;

    TreeItem* item  = flat_[idx].item;
    int       depth = flat_[idx].depth;

    // Check if click is on the expand triangle area
    float ix = float(depth * indent_) + 4.f;
    if (!item->children.empty() && e.pos.x >= ix && e.pos.x < ix + float(indent_)) {
        item->expanded ? collapse(item) : expand(item);
        return;
    }

    select(item);
}

void TreeView::on_double_click(const MouseEvent& e) {
    if (flat_dirty_) build_flat_list();
    int idx = flat_index_at(e.pos);
    if (idx < 0) return;
    TreeItem* item = flat_[idx].item;
    if (!item->children.empty())
        item->expanded ? collapse(item) : expand(item);
    on_item_activated.emit(item);
}

void TreeView::on_key_press(const KeyEvent& e) {
    if (flat_dirty_) build_flat_list();
    if (flat_.empty()) return;

    // Find current selection index
    int cur = -1;
    for (int i = 0; i < int(flat_.size()); ++i)
        if (flat_[i].item == selected_) { cur = i; break; }

    switch (e.key) {
    case Key::Up:
        if (cur > 0) { select(flat_[cur - 1].item); ensure_visible(selected_); }
        break;
    case Key::Down:
        if (cur + 1 < int(flat_.size())) { select(flat_[cur + 1].item); ensure_visible(selected_); }
        else if (cur < 0 && !flat_.empty()) { select(flat_[0].item); }
        break;
    case Key::Left:
        if (selected_ && selected_->expanded) collapse(selected_);
        else if (selected_ && selected_->parent) { select(selected_->parent); ensure_visible(selected_); }
        break;
    case Key::Right:
        if (selected_ && !selected_->children.empty() && !selected_->expanded) expand(selected_);
        break;
    case Key::Return:
        if (selected_) on_item_activated.emit(selected_);
        break;
    default: break;
    }
}

void TreeView::on_mouse_scroll(const MouseEvent& e) {
    if (flat_dirty_) build_flat_list();
    int total_h = int(flat_.size()) * item_height_;
    scroll_offset_ = std::clamp(
        scroll_offset_ - int(e.wheel.y * float(item_height_)),
        0, std::max(0, total_h - size().h));
    invalidate();
}

} // namespace swole
