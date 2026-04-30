#pragma once

#include "layout.hpp"
#include "../window/widget.hpp"

namespace swole {

// Item descriptor for BoxLayout. Allows inserting stretches or fixed spacers
// alongside real widgets.
struct BoxItem {
    enum class Kind { Widget, Stretch, Spacer };

    Kind    kind{Kind::Widget};
    Widget* widget{nullptr};
    int     stretch{1};   // relative weight for Kind::Widget and Kind::Stretch
    int     fixed_size{0}; // for Kind::Spacer
};

// Lays out children in a single row (Horizontal) or column (Vertical).
// Stretch factors distribute remaining space proportionally.
class BoxLayout : public Layout {
public:
    explicit BoxLayout(Orientation orientation);
    static BoxLayout h() { return BoxLayout(Orientation::Horizontal); }
    static BoxLayout v() { return BoxLayout(Orientation::Vertical); }

    // Add the next child widget (must already be a child of the owner widget).
    BoxLayout& add_widget(Widget* w, int stretch = 0);

    // Insert an invisible stretchable gap.
    BoxLayout& add_stretch(int stretch = 1);

    // Insert a fixed-size gap.
    BoxLayout& add_spacer(int px);

    void apply(Widget& owner) override;
    SizeI min_size(const Widget& owner) const override;
    SizeI preferred_size(const Widget& owner) const override;

private:
    Orientation orientation_;
    std::vector<BoxItem> items_;
};

} // namespace swole
