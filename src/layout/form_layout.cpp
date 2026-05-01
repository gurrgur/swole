#include "swole/layout/form_layout.hpp"
#include "swole/window/widget.hpp"

#include <algorithm>
#include <numeric>

namespace swole {

void FormLayout::add_row(Widget* label, Widget* field) {
    rows_.push_back({label, field});
}

void FormLayout::add_row(Widget* field) {
    rows_.push_back({nullptr, field});
}

int FormLayout::effective_label_width(const Widget& /*owner*/) const {
    if (label_width_ > 0) return label_width_;
    int w = 0;
    for (auto& r : rows_) {
        if (r.label)
            w = std::max(w, r.label->size_hint().preferred_size.w);
    }
    return std::max(w, 60);
}

int FormLayout::row_min_h(const Row& r) const {
    int h = 0;
    if (r.label) h = std::max(h, r.label->size_hint().min_size.h);
    if (r.field) h = std::max(h, r.field->size_hint().min_size.h);
    return std::max(h, 4);
}

int FormLayout::row_pref_h(const Row& r) const {
    int h = 0;
    if (r.label) h = std::max(h, r.label->size_hint().preferred_size.h);
    if (r.field) h = std::max(h, r.field->size_hint().preferred_size.h);
    return std::max(h, 4);
}

void FormLayout::apply(Widget& owner) {
    const InsetsI m  = margin_;
    const int     sp = spacing_;
    int lw = effective_label_width(owner);

    int field_x = m.left + lw + sp;
    int field_w = std::max(0, owner.size().w - field_x - m.right);

    int y = m.top;
    for (auto& r : rows_) {
        int rh = row_pref_h(r);

        if (r.label) {
            int lh = r.label->size_hint().preferred_size.h;
            // Vertically centre label within the row
            int ly = y + std::max(0, rh - lh) / 2;
            r.label->set_bounds({m.left, ly, lw, lh});
        }
        if (r.field) {
            r.field->set_bounds({field_x, y, field_w, rh});
        }
        y += rh + sp;
    }
}

SizeI FormLayout::min_size(const Widget& owner) const {
    int lw = effective_label_width(owner);
    int w  = margin_.left + lw + spacing_ + 40 + margin_.right;
    int h  = margin_.top + margin_.bottom;
    for (auto& r : rows_) h += row_min_h(r) + spacing_;
    if (!rows_.empty()) h -= spacing_;
    return {w, h};
}

SizeI FormLayout::preferred_size(const Widget& owner) const {
    int lw  = effective_label_width(owner);
    int fw  = 0;
    for (auto& r : rows_)
        if (r.field) fw = std::max(fw, r.field->size_hint().preferred_size.w);

    int w = margin_.left + lw + spacing_ + fw + margin_.right;
    int h = margin_.top + margin_.bottom;
    for (auto& r : rows_) h += row_pref_h(r) + spacing_;
    if (!rows_.empty()) h -= spacing_;
    return {w, h};
}

} // namespace swole
