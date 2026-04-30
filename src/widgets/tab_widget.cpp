#include "swole/widgets/tab_widget.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

namespace swole {

TabWidget::TabWidget(Widget* parent) : Widget(parent) {}

int TabWidget::add_tab(std::string_view title, std::unique_ptr<Widget> page) {
    int idx = int(tabs_.size());
    Widget* raw = add_child(std::move(page));
    raw->set_visible(false);
    tabs_.push_back({std::string{title}, raw});

    if (current_ < 0) {
        current_ = 0;
        raw->set_visible(true);
    }
    layout_children();
    return idx;
}

void TabWidget::remove_tab(int index) {
    if (index < 0 || index >= int(tabs_.size())) return;
    remove_child(tabs_[index].page);
    tabs_.erase(tabs_.begin() + index);
    if (current_ >= int(tabs_.size())) current_ = int(tabs_.size()) - 1;
    layout_children();
    invalidate();
}

void TabWidget::set_current_index(int index) {
    if (index == current_ || index < 0 || index >= int(tabs_.size())) return;
    if (current_ >= 0) tabs_[current_].page->set_visible(false);
    current_ = index;
    tabs_[current_].page->set_visible(true);
    on_current_changed.emit(current_);
    layout_children();
    invalidate();
}

Widget* TabWidget::current_page() const {
    if (current_ < 0 || current_ >= int(tabs_.size())) return nullptr;
    return tabs_[current_].page;
}

void TabWidget::set_tab_title(int index, std::string_view title) {
    tabs_[index].title = title;
    invalidate();
}

std::string_view TabWidget::tab_title(int index) const {
    return tabs_[index].title;
}

// ── geometry ──────────────────────────────────────────────────────────────────

RectI TabWidget::tab_bar_rect() const {
    return {0, 0, size().w, tab_bar_height_};
}

RectI TabWidget::page_rect() const {
    return {0, tab_bar_height_, size().w, size().h - tab_bar_height_};
}

RectI TabWidget::tab_rect(int index) const {
    Font font = theme::default_font();
    int x = 0;
    for (int i = 0; i < index; ++i) {
        int w = int(font.measure_text_width(tabs_[i].title)) + 24;
        x += w;
    }
    int w = int(font.measure_text_width(tabs_[index].title)) + 24;
    return {x, 0, w, tab_bar_height_};
}

WidgetSizeHint TabWidget::size_hint() const {
    SizeI page_hint{200, 150};
    if (current_ >= 0 && tabs_[current_].page) {
        auto h = tabs_[current_].page->size_hint();
        page_hint = h.preferred_size;
    }
    return {.preferred_size = {page_hint.w, page_hint.h + tab_bar_height_}};
}

void TabWidget::layout_children() {
    RectI pr = page_rect();
    for (auto& tab : tabs_)
        if (tab.page) tab.page->set_bounds(pr);
}

// ── paint ─────────────────────────────────────────────────────────────────────

void TabWidget::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    Font  font = theme::default_font();
    auto  m    = font.metrics();

    // Tab bar background
    canvas.draw_rect(RectF{tab_bar_rect()}, Paint::fill(theme::surface));

    // Bottom border of entire bar
    float bar_bottom = float(tab_bar_height_);
    canvas.draw_line(0.f, bar_bottom, r.w, bar_bottom,
                     Paint::stroke(theme::border));

    // Draw each tab
    for (int i = 0; i < int(tabs_.size()); ++i) {
        RectF tr{tab_rect(i)};
        bool  active = (i == current_);

        if (active) {
            // Active tab: white fill, connected to page area (no bottom border)
            canvas.draw_rect({tr.x, tr.y, tr.w, tr.h + 1.f},
                             Paint::fill(theme::bg));
            canvas.draw_line(tr.x, tr.y, tr.x, tr.bottom(),
                             Paint::stroke(theme::border));
            canvas.draw_line(tr.right(), tr.y, tr.right(), tr.bottom(),
                             Paint::stroke(theme::border));
            canvas.draw_line(tr.x, tr.y, tr.right(), tr.y,
                             Paint::stroke(theme::border));
        } else {
            canvas.draw_rect(tr.inset(1.f, 0.f),
                             Paint::fill(theme::header_bg));
        }

        Color text_col = active ? theme::text : theme::text_dim;
        canvas.draw_text(tabs_[i].title, tr,
                         TextAlign::Center, TextBaseline::Middle,
                         font, Paint::fill(text_col));
    }

    // Page area border (left, right, bottom sides)
    RectF pr{page_rect()};
    canvas.draw_line(pr.x, pr.y, pr.x, pr.bottom(),
                     Paint::stroke(theme::border));
    canvas.draw_line(pr.right(), pr.y, pr.right(), pr.bottom(),
                     Paint::stroke(theme::border));
    canvas.draw_line(pr.x, pr.bottom(), pr.right(), pr.bottom(),
                     Paint::stroke(theme::border));
}

// ── events ────────────────────────────────────────────────────────────────────

void TabWidget::on_mouse_press(const MouseEvent& e) {
    if (!tab_bar_rect().contains(e.pos)) return;
    for (int i = 0; i < int(tabs_.size()); ++i) {
        if (tab_rect(i).contains(e.pos)) {
            set_current_index(i);
            break;
        }
    }
}

} // namespace swole
