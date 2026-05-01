#pragma once

#include "layout.hpp"
#include <string>
#include <vector>

namespace swole {

// Two-column layout: fixed-width label column on the left,
// expanding field column on the right. Rows are laid out top-to-bottom
// with uniform row height determined by the tallest widget in each row.
class FormLayout : public Layout {
public:
    FormLayout() = default;

    // Add a row with an existing label widget and a field widget.
    void add_row(Widget* label, Widget* field);

    // Convenience: leave the label slot empty (field spans full width).
    void add_row(Widget* field);

    // Width of the label column in pixels. 0 = auto (widest label hint).
    void set_label_width(int px) { label_width_ = px; }
    [[nodiscard]] int label_width() const { return label_width_; }

    void apply(Widget& owner) override;
    SizeI min_size(const Widget& owner)       const override;
    SizeI preferred_size(const Widget& owner) const override;

private:
    struct Row { Widget* label; Widget* field; }; // label may be nullptr
    std::vector<Row> rows_;
    int label_width_{0}; // 0 = auto

    [[nodiscard]] int effective_label_width(const Widget& owner) const;
    [[nodiscard]] int row_min_h(const Row& r) const;
    [[nodiscard]] int row_pref_h(const Row& r) const;
};

} // namespace swole
