#include "swole/widgets/scroll_area.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

ScrollArea::ScrollArea(Widget* parent) : Widget(parent) {}

void ScrollArea::set_content(std::unique_ptr<Widget> content) {
    if (content_) remove_child(content_);
    content_ = add_child(std::move(content));
    layout_children();
}

void ScrollArea::scroll_to(PointI pos) {
    scroll_pos_ = pos;
    clamp_scroll();
    if (content_) content_->set_pos({-scroll_pos_.x, -scroll_pos_.y});
    invalidate();
}

void ScrollArea::scroll_by(int dx, int dy) {
    scroll_to({scroll_pos_.x + dx, scroll_pos_.y + dy});
}

void ScrollArea::clamp_scroll() {
    if (!content_) { scroll_pos_ = {0, 0}; return; }
    SizeI content_sz = content_->size();
    RectI vp = viewport_rect();
    scroll_pos_.x = std::clamp(scroll_pos_.x, 0, std::max(0, content_sz.w - vp.w));
    scroll_pos_.y = std::clamp(scroll_pos_.y, 0, std::max(0, content_sz.h - vp.h));
}

RectI ScrollArea::viewport_rect() const {
    bool need_vbar = needs_vbar();
    bool need_hbar = needs_hbar();
    int w = size().w - (need_vbar ? bar_size_ : 0);
    int h = size().h - (need_hbar ? bar_size_ : 0);
    return {0, 0, w, h};
}

bool ScrollArea::needs_vbar() const {
    if (v_policy_ == ScrollPolicy::Never)  return false;
    if (v_policy_ == ScrollPolicy::Always) return true;
    if (!content_) return false;
    return content_->size().h > size().h;
}

bool ScrollArea::needs_hbar() const {
    if (h_policy_ == ScrollPolicy::Never)  return false;
    if (h_policy_ == ScrollPolicy::Always) return true;
    if (!content_) return false;
    return content_->size().w > size().w;
}

WidgetSizeHint ScrollArea::size_hint() const {
    if (content_) {
        auto h = content_->size_hint();
        return {.min_size = {}, .preferred_size = {h.preferred_size.w + bar_size_,
                                   h.preferred_size.h + bar_size_}};
    }
    return {.min_size = {}, .preferred_size = {120, 80}};
}

void ScrollArea::update_bars() {
    layout_children();
    invalidate();
}

void ScrollArea::layout_children() {
    if (!content_) return;
    RectI vp = viewport_rect();
    if (resizable_) {
        content_->set_bounds({-scroll_pos_.x, -scroll_pos_.y, vp.w, vp.h});
    } else {
        SizeI cs = content_->size_hint().preferred_size;
        content_->set_bounds({-scroll_pos_.x, -scroll_pos_.y,
                              std::max(cs.w, vp.w), std::max(cs.h, vp.h)});
    }
}

void ScrollArea::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    bool vbar = needs_vbar();
    bool hbar = needs_hbar();

    // Background
    canvas.draw_rect(r, Paint::fill(theme::bg));

    // Content is painted by Widget::dispatch_paint via the child

    // ── Vertical scrollbar ────────────────────────────────────────────────────
    if (vbar && content_) {
        int content_h = content_->size().h;
        int view_h    = viewport_rect().h;
        float track_h = float(size().h - (hbar ? bar_size_ : 0));
        float ratio   = (content_h > 0) ? float(view_h) / float(content_h) : 1.f;
        float thumb_h = std::max(20.f, track_h * ratio);
        float max_off = float(std::max(0, content_h - view_h));
        float t       = (max_off > 0) ? float(scroll_pos_.y) / max_off : 0.f;
        float thumb_y = t * (track_h - thumb_h);

        RectF track{r.right() - float(bar_size_), 0.f, float(bar_size_), track_h};
        RectF thumb{track.x + 2.f, thumb_y + 2.f, float(bar_size_) - 4.f, thumb_h - 4.f};

        canvas.draw_rect(track, Paint::fill(Color{245, 246, 248}));
        canvas.draw_round_rect(thumb, 3.f, 3.f, Paint::fill(theme::scrollbar));
    }

    // ── Horizontal scrollbar ─────────────────────────────────────────────────
    if (hbar && content_) {
        int content_w = content_->size().w;
        int view_w    = viewport_rect().w;
        float track_w = float(size().w - (vbar ? bar_size_ : 0));
        float ratio   = (content_w > 0) ? float(view_w) / float(content_w) : 1.f;
        float thumb_w = std::max(20.f, track_w * ratio);
        float max_off = float(std::max(0, content_w - view_w));
        float t       = (max_off > 0) ? float(scroll_pos_.x) / max_off : 0.f;
        float thumb_x = t * (track_w - thumb_w);

        RectF track{0.f, r.bottom() - float(bar_size_), track_w, float(bar_size_)};
        RectF thumb{thumb_x + 2.f, track.y + 2.f, thumb_w - 4.f, float(bar_size_) - 4.f};

        canvas.draw_rect(track, Paint::fill(Color{245, 246, 248}));
        canvas.draw_round_rect(thumb, 3.f, 3.f, Paint::fill(theme::scrollbar));
    }

    // Outer border
    canvas.draw_rect(r.inset(.5f), Paint::stroke(theme::border));
}

void ScrollArea::on_mouse_scroll(const MouseEvent& e) {
    int dx = int(-e.wheel.x * 40.f);
    int dy = int(-e.wheel.y * 40.f);
    scroll_by(dx, dy);
}

} // namespace swole
