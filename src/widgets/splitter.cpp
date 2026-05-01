#include "swole/widgets/splitter.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>
#include <numeric>

namespace swole {

Splitter::Splitter(Widget* parent, Orientation o)
    : Widget(parent), orientation_{o} {}

void Splitter::set_orientation(Orientation o) {
    orientation_ = o;
    do_layout();
    invalidate();
}

Widget* Splitter::add_panel(std::unique_ptr<Widget> panel) {
    Widget* raw = panel.get();
    add_child(std::move(panel));
    panels_.push_back(raw);
    // Give each panel an equal share to start with
    sizes_.assign(panels_.size(), 100);
    do_layout();
    return raw;
}

void Splitter::set_sizes(std::vector<int> sz) {
    if (sz.size() != panels_.size()) return;
    sizes_ = std::move(sz);
    do_layout();
}

std::vector<int> Splitter::sizes() const { return sizes_; }

// ── Layout ────────────────────────────────────────────────────────────────────

void Splitter::do_layout() {
    if (panels_.empty()) return;
    const int n = int(panels_.size());
    const bool horiz = (orientation_ == Orientation::Horizontal);
    const int total = horiz ? size().w : size().h;
    const int other = horiz ? size().h : size().w;
    const int handle_total = (n - 1) * kHandleW;
    const int available = std::max(0, total - handle_total);

    // Normalise sizes_ to fill `available` pixels
    int sum = 0;
    for (int s : sizes_) sum += std::max(1, s);
    std::vector<int> px(n);
    int used = 0;
    for (int i = 0; i < n - 1; ++i) {
        px[i] = std::max(0, int(float(sizes_[i]) / float(sum) * float(available)));
        used += px[i];
    }
    px[n-1] = std::max(0, available - used);

    int pos = 0;
    for (int i = 0; i < n; ++i) {
        if (horiz)
            panels_[i]->set_bounds({pos, 0, px[i], other});
        else
            panels_[i]->set_bounds({0, pos, other, px[i]});
        pos += px[i] + kHandleW;
    }
}

void Splitter::layout_children() {
    do_layout();
}

void Splitter::on_bounds_changed(RectI old) {
    Widget::on_bounds_changed(old);
    do_layout();
}

// ── Handle hit-testing ────────────────────────────────────────────────────────

int Splitter::handle_at(PointI p) const {
    const bool horiz = (orientation_ == Orientation::Horizontal);
    const int coord = horiz ? p.x : p.y;
    for (int i = 0; i + 1 < int(panels_.size()); ++i) {
        const auto& b = panels_[i]->bounds();
        int edge = horiz ? (b.x + b.w) : (b.y + b.h);
        if (coord >= edge && coord < edge + kHandleW)
            return i;
    }
    return -1;
}

// ── Painting ──────────────────────────────────────────────────────────────────

void Splitter::on_paint(Canvas& canvas) {
    const bool horiz = (orientation_ == Orientation::Horizontal);
    for (int i = 0; i + 1 < int(panels_.size()); ++i) {
        const auto& b = panels_[i]->bounds();
        int edge = horiz ? (b.x + b.w) : (b.y + b.h);
        RectF hr = horiz
            ? RectF{float(edge), 0.f, float(kHandleW), float(size().h)}
            : RectF{0.f, float(edge), float(size().w), float(kHandleW)};

        Color bg = (i == hovered_handle_) ? theme::hover_bg : theme::surface;
        canvas.draw_rect(hr, Paint::fill(bg));

        // Dotted grip marks in the centre
        float cx = hr.x + hr.w * .5f, cy = hr.y + hr.h * .5f;
        for (int d = -2; d <= 2; ++d) {
            float gx = horiz ? cx : cx + float(d) * 4.f;
            float gy = horiz ? cy + float(d) * 4.f : cy;
            canvas.draw_circle(gx, gy, 1.5f, Paint::fill(theme::border));
        }
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void Splitter::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    int h = handle_at(e.pos);
    if (h < 0) return;
    dragging_     = h;
    drag_origin_  = (orientation_ == Orientation::Horizontal) ? e.pos.x : e.pos.y;
    const auto& ba = panels_[h]->bounds();
    const auto& bb = panels_[h+1]->bounds();
    drag_a_ = (orientation_ == Orientation::Horizontal) ? ba.w : ba.h;
    drag_b_ = (orientation_ == Orientation::Horizontal) ? bb.w : bb.h;
}

void Splitter::on_mouse_release(const MouseEvent&) {
    if (dragging_ >= 0) {
        dragging_ = -1;
        on_moved.emit();
    }
}

void Splitter::on_mouse_move(const MouseEvent& e) {
    if (dragging_ >= 0) {
        int coord = (orientation_ == Orientation::Horizontal) ? e.pos.x : e.pos.y;
        int delta = coord - drag_origin_;
        int na = std::max(1, drag_a_ + delta);
        int nb = std::max(1, drag_b_ - delta);
        // Don't let the pair shrink below zero total
        if (na + nb != drag_a_ + drag_b_) {
            na = drag_a_ + drag_b_ - nb;
        }
        sizes_[dragging_]   = na;
        sizes_[dragging_+1] = nb;
        do_layout();
        invalidate();
        return;
    }

    int h = handle_at(e.pos);
    if (h != hovered_handle_) {
        hovered_handle_ = h;
        invalidate();
    }

    // Update cursor shape
    if (h >= 0)
        set_cursor(orientation_ == Orientation::Horizontal
                   ? CursorShape::SizeH : CursorShape::SizeV);
    else
        set_cursor(CursorShape::Arrow);
}

void Splitter::on_mouse_enter(const MouseEvent&) {}

void Splitter::on_mouse_leave(const MouseEvent&) {
    if (hovered_handle_ >= 0) {
        hovered_handle_ = -1;
        invalidate();
    }
    set_cursor(CursorShape::Arrow);
}

} // namespace swole
