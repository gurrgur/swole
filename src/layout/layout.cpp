#include "swole/layout/layout.hpp"
#include "swole/window/widget.hpp"

#include <algorithm>

namespace swole {

SizeI Layout::min_size(const Widget& owner) const {
    SizeI result{margin_.h_total(), margin_.v_total()};
    for (auto& child : owner.children()) {
        if (!child || !child->is_visible()) continue;
        auto hint = child->size_hint();
        result.w = std::max(result.w, hint.min_size.w + margin_.h_total());
        result.h = std::max(result.h, hint.min_size.h + margin_.v_total());
    }
    return result;
}

SizeI Layout::preferred_size(const Widget& owner) const {
    SizeI result{margin_.h_total(), margin_.v_total()};
    for (auto& child : owner.children()) {
        if (!child || !child->is_visible()) continue;
        auto hint = child->size_hint();
        result.w = std::max(result.w, hint.preferred_size.w + margin_.h_total());
        result.h = std::max(result.h, hint.preferred_size.h + margin_.v_total());
    }
    return result;
}

} // namespace swole
