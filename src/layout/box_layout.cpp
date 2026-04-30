#include "swole/layout/box_layout.hpp"
#include "swole/window/widget.hpp"

#include <numeric>

namespace swole {

BoxLayout::BoxLayout(Orientation orientation) : orientation_{orientation} {}

BoxLayout& BoxLayout::add_widget(Widget* w, int stretch) {
    items_.push_back({BoxItem::Kind::Widget, w, stretch, 0});
    return *this;
}
BoxLayout& BoxLayout::add_stretch(int stretch) {
    items_.push_back({BoxItem::Kind::Stretch, nullptr, stretch, 0});
    return *this;
}
BoxLayout& BoxLayout::add_spacer(int px) {
    items_.push_back({BoxItem::Kind::Spacer, nullptr, 0, px});
    return *this;
}

void BoxLayout::apply(Widget& owner) {
    const bool horiz = (orientation_ == Orientation::Horizontal);
    const RectI bounds = owner.local_bounds();

    int avail = (horiz ? bounds.w : bounds.h)
                - margin_.h_total() * (horiz ? 1 : 0)
                - margin_.v_total() * (horiz ? 0 : 1);

    // Sum fixed sizes and count total stretch weight
    int fixed_total = 0;
    int stretch_sum = 0;
    int gaps        = 0;

    for (auto& item : items_) {
        switch (item.kind) {
        case BoxItem::Kind::Widget:
            if (item.widget && item.widget->is_visible()) {
                auto hint = item.widget->size_hint();
                int  pref = horiz ? hint.preferred_size.w : hint.preferred_size.h;
                if (item.stretch == 0) fixed_total += pref;
                else                   stretch_sum  += item.stretch;
                ++gaps;
            }
            break;
        case BoxItem::Kind::Stretch:
            stretch_sum += item.stretch;
            ++gaps;
            break;
        case BoxItem::Kind::Spacer:
            fixed_total += item.fixed_size;
            ++gaps;
            break;
        }
    }

    int spacing_total = spacing_ * std::max(0, gaps - 1);
    int flex_space    = std::max(0, avail - fixed_total - spacing_total);
    int unit          = stretch_sum > 0 ? flex_space / stretch_sum : 0;
    int remainder     = stretch_sum > 0 ? flex_space % stretch_sum : 0;

    int pos     = horiz ? (bounds.x + margin_.left) : (bounds.y + margin_.top);
    int cross   = horiz ? (bounds.y + margin_.top)  : (bounds.x + margin_.left);
    int cross_sz = horiz ? (bounds.h - margin_.v_total()) : (bounds.w - margin_.h_total());

    bool first = true;
    for (auto& item : items_) {
        if (!first) pos += spacing_;

        switch (item.kind) {
        case BoxItem::Kind::Widget: {
            if (!item.widget || !item.widget->is_visible()) continue;
            auto hint = item.widget->size_hint();
            int pref  = horiz ? hint.preferred_size.w : hint.preferred_size.h;
            int sz    = (item.stretch > 0)
                        ? unit * item.stretch + (remainder > 0 ? (remainder--, 1) : 0)
                        : pref;
            sz = std::clamp(sz,
                            horiz ? hint.min_size.w : hint.min_size.h,
                            horiz ? hint.max_size.w : hint.max_size.h);

            RectI r;
            if (horiz) r = {pos, cross, sz, cross_sz};
            else        r = {cross, pos, cross_sz, sz};
            item.widget->set_bounds(r);
            pos += sz;
            break;
        }
        case BoxItem::Kind::Stretch:
            pos += unit * item.stretch + (remainder > 0 ? (remainder--, 1) : 0);
            break;
        case BoxItem::Kind::Spacer:
            pos += item.fixed_size;
            break;
        }
        first = false;
    }
}

SizeI BoxLayout::min_size(const Widget& owner) const {
    const bool horiz = (orientation_ == Orientation::Horizontal);
    int main_sz = 0, cross_sz = 0;
    int gaps = 0;
    for (auto& item : items_) {
        if (item.kind == BoxItem::Kind::Widget && item.widget) {
            auto hint = item.widget->size_hint();
            main_sz  += horiz ? hint.min_size.w : hint.min_size.h;
            cross_sz  = std::max(cross_sz, horiz ? hint.min_size.h : hint.min_size.w);
            ++gaps;
        } else if (item.kind == BoxItem::Kind::Spacer) {
            main_sz += item.fixed_size; ++gaps;
        }
    }
    main_sz += spacing_ * std::max(0, gaps - 1);
    if (horiz) return {main_sz + margin_.h_total(), cross_sz + margin_.v_total()};
    else       return {cross_sz + margin_.h_total(), main_sz + margin_.v_total()};
}

SizeI BoxLayout::preferred_size(const Widget& owner) const {
    const bool horiz = (orientation_ == Orientation::Horizontal);
    int main_sz = 0, cross_sz = 0;
    int gaps = 0;
    for (auto& item : items_) {
        if (item.kind == BoxItem::Kind::Widget && item.widget) {
            auto hint = item.widget->size_hint();
            main_sz  += horiz ? hint.preferred_size.w : hint.preferred_size.h;
            cross_sz  = std::max(cross_sz, horiz ? hint.preferred_size.h : hint.preferred_size.w);
            ++gaps;
        } else if (item.kind == BoxItem::Kind::Spacer) {
            main_sz += item.fixed_size; ++gaps;
        } else if (item.kind == BoxItem::Kind::Stretch) {
            ++gaps;
        }
    }
    main_sz += spacing_ * std::max(0, gaps - 1);
    if (horiz) return {main_sz + margin_.h_total(), cross_sz + margin_.v_total()};
    else       return {cross_sz + margin_.h_total(), main_sz + margin_.v_total()};
}

} // namespace swole
