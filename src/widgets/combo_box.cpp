#include "swole/widgets/combo_box.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

namespace swole {

static constexpr int kArrowW = 20;

ComboBox::ComboBox(Widget* parent) : Widget(parent), font_(theme::default_font()) {
    set_focus_policy(FocusPolicy::Click);
}

int ComboBox::add_item(std::string_view text, void* user_data) {
    items_.push_back({std::string{text}, user_data});
    if (current_ < 0) { current_ = 0; }
    invalidate();
    return int(items_.size()) - 1;
}

void ComboBox::remove_item(int index) {
    items_.erase(items_.begin() + index);
    if (current_ >= int(items_.size())) current_ = int(items_.size()) - 1;
    invalidate();
}

void ComboBox::clear() {
    items_.clear();
    current_ = -1;
    edit_text_.clear();
    invalidate();
}

std::string_view ComboBox::item_text(int index) const {
    return items_[index].text;
}

void* ComboBox::item_user_data(int index) const {
    return items_[index].user_data;
}

void ComboBox::set_current_index(int index) {
    if (index == current_) return;
    current_ = (index >= 0 && index < int(items_.size())) ? index : -1;
    if (current_ >= 0) edit_text_ = items_[current_].text;
    on_index_changed.emit(current_);
    invalidate();
}

void ComboBox::set_current_text(std::string_view text) {
    for (int i = 0; i < int(items_.size()); ++i) {
        if (items_[i].text == text) { set_current_index(i); return; }
    }
    // Not found — if editable, accept it as typed text
    if (editable_) {
        edit_text_ = text;
        current_   = -1;
        on_text_changed.emit(edit_text_);
        invalidate();
    }
}

std::string_view ComboBox::current_text() const {
    if (editable_) return edit_text_;
    if (current_ >= 0 && current_ < int(items_.size())) return items_[current_].text;
    return {};
}

void ComboBox::set_font(Font f) { font_ = std::move(f); invalidate(); }

WidgetSizeHint ComboBox::size_hint() const {
    auto m = font_.metrics();
    int h = int(m.line_height()) + 10;
    return {.min_size = {60, h}, .preferred_size = {160, h}};
}

// ── popup (simple inline overlay) ────────────────────────────────────────────

static void draw_chevron(Canvas& canvas, RectF arrow_area) {
    float cx = arrow_area.x + arrow_area.w * .5f;
    float cy = arrow_area.y + arrow_area.h * .5f;
    const float sz = 4.f;
    PointF pts[] = {{cx - sz, cy - sz * .4f},
                    {cx,      cy + sz * .6f},
                    {cx + sz, cy - sz * .4f}};
    auto p = Paint::stroke(theme::text_dim, 1.5f);
    p.set_stroke_cap(StrokeCap::Round);
    p.set_stroke_join(StrokeJoin::Round);
    canvas.draw_polygon(pts, false, p);
}

void ComboBox::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    theme::draw_widget_bg(canvas, r, has_focus() && !popup_open_);

    // Arrow strip
    RectF arrow{r.right() - float(kArrowW), r.y, float(kArrowW), r.h};
    canvas.draw_line(arrow.x, r.y + 3.f, arrow.x, r.bottom() - 3.f,
                     Paint::stroke(theme::border));
    draw_chevron(canvas, arrow);

    // Text area (clipped)
    auto guard = canvas.scoped_save();
    canvas.clip_rect({r.x + 1.f, r.y + 1.f, r.w - float(kArrowW) - 2.f, r.h - 2.f});

    std::string_view txt = current_text();
    if (!txt.empty()) {
        auto m = font_.metrics();
        float base_y = (r.h - m.ascent - m.descent) * .5f + m.ascent;
        canvas.draw_text(txt, r.x + 6.f, base_y, font_, Paint::fill(theme::text));
    }

    // Inline dropdown (drawn in parent space via canvas save/translate)
    if (popup_open_) {
        float inh = float(font_.metrics().line_height() + 6.f);
        float pop_h = float(items_.size()) * inh + 2.f;
        RectF pop{r.x, r.bottom() - 1.f, r.w, pop_h};

        // Draw on top (no z-order yet — just paints over siblings below)
        canvas.draw_round_rect(pop, theme::radius_small, theme::radius_small,
                               Paint::fill(theme::bg));
        canvas.draw_round_rect(pop, theme::radius_small, theme::radius_small,
                               Paint::stroke(theme::border));

        for (int i = 0; i < int(items_.size()); ++i) {
            RectF ir{pop.x + 1.f, pop.y + 1.f + float(i) * inh, pop.w - 2.f, inh};
            if (i == current_) {
                canvas.draw_rect(ir.inset(.5f), Paint::fill(theme::sel_bg));
            }
            auto pm = font_.metrics();
            canvas.draw_text(items_[i].text,
                             ir.x + 6.f,
                             ir.y + (ir.h - pm.ascent - pm.descent) * .5f + pm.ascent,
                             font_,
                             Paint::fill(i == current_ ? theme::sel_text : theme::text));
        }
    }
}

void ComboBox::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    if (popup_open_) {
        // Check if click is on a popup item
        float inh = float(font_.metrics().line_height() + 6.f);
        float pop_y = float(size().h) - 1.f;
        int idx = int((float(e.pos.y) - pop_y) / inh);
        if (idx >= 0 && idx < int(items_.size())) {
            set_current_index(idx);
        }
        popup_open_ = false;
        invalidate();
        return;
    }
    popup_open_ = true;
    invalidate();
}

void ComboBox::on_key_press(const KeyEvent& e) {
    switch (e.key) {
    case Key::Up:
        if (current_ > 0) set_current_index(current_ - 1);
        break;
    case Key::Down:
        if (current_ + 1 < int(items_.size())) set_current_index(current_ + 1);
        break;
    case Key::Return: case Key::Escape:
        popup_open_ = false;
        invalidate();
        break;
    default: break;
    }
}

} // namespace swole
