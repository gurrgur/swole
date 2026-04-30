#pragma once

#include "../window/widget.hpp"
#include <string>
#include <vector>

namespace swole {

class TabWidget : public Widget {
public:
    explicit TabWidget(Widget* parent = nullptr);

    // Add tab, taking ownership of the page widget.
    int  add_tab(std::string_view title, std::unique_ptr<Widget> page);
    void remove_tab(int index);

    void set_current_index(int index);
    [[nodiscard]] int  current_index() const { return current_; }
    [[nodiscard]] Widget* current_page() const;

    void set_tab_title(int index, std::string_view title);
    [[nodiscard]] std::string_view tab_title(int index) const;

    [[nodiscard]] int tab_count() const { return int(tabs_.size()); }

    void layout_children() override;
    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<int> on_current_changed;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;

private:
    [[nodiscard]] RectI tab_bar_rect() const;
    [[nodiscard]] RectI tab_rect(int index) const;
    [[nodiscard]] RectI page_rect() const;

    // page is a non-owning view into children_ (Widget owns via unique_ptr there)
    struct Tab { std::string title; Widget* page{nullptr}; };
    std::vector<Tab> tabs_;
    int  current_{-1};
    int  tab_bar_height_{28};
};

} // namespace swole
