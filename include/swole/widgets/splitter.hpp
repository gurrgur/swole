#pragma once

#include "../window/widget.hpp"
#include "../core/types.hpp"
#include <vector>

namespace swole {

// Lays out two or more panels separated by draggable handles.
class Splitter : public Widget {
public:
    explicit Splitter(Widget* parent = nullptr,
                      Orientation orientation = Orientation::Horizontal);

    void set_orientation(Orientation o);
    [[nodiscard]] Orientation orientation() const { return orientation_; }

    // Add a panel; the Splitter takes ownership via its widget tree.
    Widget* add_panel(std::unique_ptr<Widget> panel);

    template <typename T, typename... Args>
    T* emplace_panel(Args&&... args) {
        return static_cast<T*>(
            add_panel(std::make_unique<T>(this, std::forward<Args>(args)...)));
    }

    // Override the current pixel sizes of all panels.
    void set_sizes(std::vector<int> sizes);
    [[nodiscard]] std::vector<int> sizes() const;

    Signal<> on_moved;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_release(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_mouse_enter(const MouseEvent& e) override;
    void on_mouse_leave(const MouseEvent& e) override;
    void layout_children() override;
    void on_bounds_changed(RectI old) override;

private:
    static constexpr int kHandleW = 5;

    // Returns index of the handle (gap between panels i and i+1) under p, or -1.
    [[nodiscard]] int handle_at(PointI p) const;
    void do_layout();

    Orientation       orientation_;
    std::vector<Widget*> panels_;   // non-owning; owned by widget children_ list
    std::vector<int>     sizes_;    // pixel size of each panel along the split axis

    int  dragging_{-1};
    int  drag_origin_{0};   // cursor coord at drag start
    int  drag_a_{0};        // panel[dragging_] size at drag start
    int  drag_b_{0};        // panel[dragging_+1] size at drag start

    int  hovered_handle_{-1};
};

} // namespace swole
