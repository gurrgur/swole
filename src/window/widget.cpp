#include "swole/window/widget.hpp"
#include "swole/layout/layout.hpp"

namespace swole {

Widget::Widget(Widget* parent) : parent_{parent} {}

Window* Widget::window() const {
    const Widget* w = this;
    while (w) {
        if (w->owner_window_) return w->owner_window_;
        w = w->parent_;
    }
    return nullptr;
}

Widget::~Widget() = default;

void Widget::set_pos(PointI pos) {
    if (pos_ == pos) return;
    RectI old = bounds();
    pos_ = pos;
    on_bounds_changed(old);
}

void Widget::set_size(SizeI sz) {
    if (size_ == sz) return;
    RectI old = bounds();
    size_ = sz;
    on_bounds_changed(old);
    layout_children();
}

void Widget::set_bounds(RectI r) {
    if (pos_.x == r.x && pos_.y == r.y && size_.w == r.w && size_.h == r.h) return;
    RectI old = bounds();
    pos_  = {r.x, r.y};
    size_ = {r.w, r.h};
    on_bounds_changed(old);
    layout_children();
}

void Widget::move(int dx, int dy) {
    set_pos({pos_.x + dx, pos_.y + dy});
}

PointI Widget::map_to_root(PointI p) const {
    const Widget* w = this;
    while (w->parent_) {
        p = p + w->pos_;
        w = w->parent_;
    }
    return p;
}

PointI Widget::map_from_root(PointI p) const {
    return p - map_to_root({0, 0});
}

void Widget::set_visible(bool v) {
    if (visible_ == v) return;
    visible_ = v;
    invalidate();
}

void Widget::set_enabled(bool e) {
    if (enabled_ == e) return;
    enabled_ = e;
    invalidate();
}

bool Widget::is_visible_to_root() const {
    const Widget* w = this;
    while (w) {
        if (!w->visible_) return false;
        w = w->parent_;
    }
    return true;
}

void Widget::request_focus() {
    // TODO: delegate to Window
    focused_ = true;
}

void Widget::clear_focus() {
    focused_ = false;
}

void Widget::set_cursor(CursorShape shape) {
    cursor_shape_ = shape;
    // TODO: update SDL cursor when this widget is hovered
}

void Widget::set_layout(std::unique_ptr<Layout> layout) {
    layout_ = std::move(layout);
    layout_children();
}

void Widget::layout_children() {
    if (layout_)
        layout_->apply(*this);
}

WidgetSizeHint Widget::size_hint() const {
    return {.preferred_size = size_};
}

Widget* Widget::add_child(std::unique_ptr<Widget> child) {
    child->parent_ = this;
    Widget* raw = child.get();
    children_.push_back(std::move(child));
    return raw;
}

std::unique_ptr<Widget> Widget::remove_child(Widget* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            (*it)->parent_ = nullptr;
            auto out = std::move(*it);
            children_.erase(it);
            return out;
        }
    }
    return nullptr;
}

Widget* Widget::hit_test(PointI local_pos) {
    if (!is_visible() || !is_enabled()) return nullptr;
    if (!local_bounds().contains(local_pos)) return nullptr;

    // Check children last-to-first (top-most drawn child checked first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = it->get();
        PointI child_local = local_pos - child->pos_;
        Widget* hit = child->hit_test(child_local);
        if (hit) return hit;
    }
    return this;
}

void Widget::invalidate(RectI region) {
    // TODO: propagate dirty rect up to the owning Window
    (void)region;
}

void Widget::on_paint(Canvas& canvas) {
    if (cb_paint) cb_paint(canvas);
}

void Widget::on_resize(const ResizeEvent& e) { (void)e; }
void Widget::on_move(const MoveEvent& e)     { (void)e; }

void Widget::on_mouse_press(const MouseEvent& e) {
    if (cb_mouse_press) cb_mouse_press(e);
}
void Widget::on_mouse_release(const MouseEvent& e) {
    if (cb_mouse_release) cb_mouse_release(e);
}
void Widget::on_mouse_move(const MouseEvent& e) {
    if (cb_mouse_move) cb_mouse_move(e);
}
void Widget::on_mouse_enter(const MouseEvent& e) { (void)e; }
void Widget::on_mouse_leave(const MouseEvent& e) { (void)e; }
void Widget::on_mouse_scroll(const MouseEvent& e) { (void)e; }
void Widget::on_double_click(const MouseEvent& e) { (void)e; }

void Widget::on_key_press(const KeyEvent& e) {
    if (cb_key_press) cb_key_press(e);
}
void Widget::on_key_release(const KeyEvent& e) { (void)e; }
void Widget::on_text_input(const TextInputEvent& e) { (void)e; }

void Widget::on_focus_gain(const FocusEvent& e) {
    focused_ = true;
    if (cb_focus_gain) cb_focus_gain(e);
    invalidate();
}
void Widget::on_focus_loss(const FocusEvent& e) {
    focused_ = false;
    if (cb_focus_loss) cb_focus_loss(e);
    invalidate();
}

void Widget::on_bounds_changed(RectI old_bounds) {
    if (old_bounds.size() != size_) {
        ResizeEvent e{old_bounds.size(), size_};
        on_resize(e);
    }
    if (old_bounds.origin() != pos_) {
        MoveEvent e{old_bounds.origin(), pos_};
        on_move(e);
    }
    invalidate();
}

void Widget::dispatch_paint(Canvas& canvas) {
    if (!visible_) return;
    auto guard = canvas.scoped_save();
    canvas.translate(float(pos_.x), float(pos_.y));
    canvas.clip_rect(RectF(local_bounds()));
    on_paint(canvas);
    for (auto& child : children_)
        child->dispatch_paint(canvas);
}

} // namespace swole
