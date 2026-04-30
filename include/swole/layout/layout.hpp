#pragma once

#include "../core/types.hpp"
#include <memory>
#include <vector>

namespace swole {

class Widget;

// Base class for layout managers. A layout is owned by a widget and is
// invoked whenever the widget's bounds change.
class Layout {
public:
    virtual ~Layout() = default;

    // Called by the owning widget. Repositions / resizes all children.
    virtual void apply(Widget& owner) = 0;

    // Compute the minimum / preferred size this layout needs.
    virtual SizeI min_size(const Widget& owner) const;
    virtual SizeI preferred_size(const Widget& owner) const;

    void set_spacing(int px)        { spacing_ = px; }
    void set_margin(InsetsI margin) { margin_  = margin; }
    void set_margin(int all)        { margin_  = InsetsI{all}; }

    [[nodiscard]] int     spacing() const { return spacing_; }
    [[nodiscard]] InsetsI margin()  const { return margin_; }

protected:
    int     spacing_{4};
    InsetsI margin_{4};
};

} // namespace swole
