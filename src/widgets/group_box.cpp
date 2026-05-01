#include "swole/widgets/group_box.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

static constexpr int kTitlePadX = 8;
static constexpr int kTitleGapX = 4;
static constexpr int kTopInset  = 20; // space above content for title bar
static constexpr int kSideInset =  8;
static constexpr int kBotInset  =  8;

GroupBox::GroupBox(Widget* parent, std::string_view title)
    : Widget(parent), title_{title} {}

void GroupBox::set_title(std::string_view t) {
    title_ = t;
    invalidate();
}

InsetsI GroupBox::content_insets() const {
    return {kTopInset, kSideInset, kBotInset, kSideInset};
}

WidgetSizeHint GroupBox::size_hint() const {
    InsetsI ins = content_insets();
    SizeI ch_min{0,0}, ch_pref{0,0};
    for (auto& child : children()) {
        auto hint = child->size_hint();
        ch_min.w  = std::max(ch_min.w,  hint.min_size.w);
        ch_min.h  = std::max(ch_min.h,  hint.min_size.h);
        ch_pref.w = std::max(ch_pref.w, hint.preferred_size.w);
        ch_pref.h = std::max(ch_pref.h, hint.preferred_size.h);
    }
    return {
        .min_size       = {ch_min.w  + ins.left + ins.right,
                           ch_min.h  + ins.top  + ins.bottom},
        .preferred_size = {ch_pref.w + ins.left + ins.right,
                           ch_pref.h + ins.top  + ins.bottom},
    };
}

void GroupBox::layout_children() {
    InsetsI ins = content_insets();
    RectI content{
        ins.left,
        ins.top,
        std::max(0, size().w - ins.left - ins.right),
        std::max(0, size().h - ins.top  - ins.bottom),
    };
    // Stretch every direct child to fill the content area.
    // If the user needs multiple children, they should embed a layout-bearing
    // container widget as the single child of the GroupBox.
    for (auto& child : children())
        child->set_bounds(content);
}

void GroupBox::on_paint(Canvas& canvas) {
    Font  font   = theme::default_font(12.f);
    auto  m      = font.metrics();
    float title_w = title_.empty() ? 0.f
                  : font.measure_text_width(title_) + float(kTitleGapX * 2);
    float fh   = m.ascent + m.descent;
    float frame_y = fh * .5f;         // vertical centre of frame stroke = centre of title text
    float W    = float(size().w);
    float H    = float(size().h);

    // Frame: four segments around the title gap
    Paint brd = Paint::stroke(theme::border);

    // Top-left segment (left of title)
    float gap_x0 = float(kTitlePadX);
    float gap_x1 = gap_x0 + title_w;

    canvas.draw_line(0.f, frame_y, gap_x0, frame_y, brd);
    if (!title_.empty())
        canvas.draw_line(gap_x1, frame_y, W, frame_y, brd);
    else
        canvas.draw_line(0.f, frame_y, W, frame_y, brd);
    canvas.draw_line(W, frame_y, W, H, brd);
    canvas.draw_line(W, H, 0.f, H, brd);
    canvas.draw_line(0.f, H, 0.f, frame_y, brd);

    // Title text
    if (!title_.empty()) {
        canvas.draw_text(title_,
                         gap_x0 + float(kTitleGapX),
                         frame_y - fh*.5f + m.ascent,
                         font, Paint::fill(theme::text_dim));
    }
}

} // namespace swole
