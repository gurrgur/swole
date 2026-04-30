#pragma once

#include "../core/event.hpp"
#include "../core/types.hpp"
#include "../render/canvas.hpp"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace swole {

class Layout;

enum class SizePolicy {
    Fixed,      // stays exactly at preferred size
    Minimum,    // can grow, not shrink below preferred
    Maximum,    // can shrink, not grow past preferred
    Preferred,  // preferred is hint; can grow or shrink
    Expanding,  // greedily takes as much space as available
    Ignored,    // preferred size is ignored
};

struct WidgetSizeHint {
    SizeI    min_size;
    SizeI    max_size{32767, 32767};
    SizeI    preferred_size;
    SizePolicy h_policy{SizePolicy::Preferred};
    SizePolicy v_policy{SizePolicy::Preferred};
};

enum class FocusPolicy { None, Click, Tab, Strong };
enum class CursorShape { Arrow, IBeam, Wait, Crosshair, Hand, SizeH, SizeV, SizeAll, Forbidden, Blank };

// ── Widget ───────────────────────────────────────────────────────────────────

// Base class for every UI element. The widget tree is owned by unique_ptrs;
// parent->children, Window->root. Lifetimes are strictly tree-shaped.
class Widget {
public:
    explicit Widget(Widget* parent = nullptr);
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // ── Identity / debug ──

    void set_name(std::string_view name) { name_ = name; }
    [[nodiscard]] std::string_view name() const { return name_; }

    // ── Geometry ──

    // Position in parent coordinates.
    void  set_pos(PointI pos);
    void  set_size(SizeI sz);
    void  set_bounds(RectI r);
    void  move(int dx, int dy);

    [[nodiscard]] PointI pos()    const { return pos_; }
    [[nodiscard]] SizeI  size()   const { return size_; }
    [[nodiscard]] RectI  bounds() const { return {pos_.x, pos_.y, size_.w, size_.h}; }
    // Bounds in own coordinate space (origin at 0,0)
    [[nodiscard]] RectI  local_bounds() const { return {0, 0, size_.w, size_.h}; }

    // Convert between coordinate spaces
    [[nodiscard]] PointI map_to_parent(PointI p) const { return p + pos_; }
    [[nodiscard]] PointI map_from_parent(PointI p) const { return p - pos_; }
    [[nodiscard]] PointI map_to_root(PointI p) const;
    [[nodiscard]] PointI map_from_root(PointI p) const;

    // ── Visibility / state ──

    void  set_visible(bool v);
    void  show() { set_visible(true); }
    void  hide() { set_visible(false); }
    void  set_enabled(bool e);

    [[nodiscard]] bool is_visible() const { return visible_; }
    [[nodiscard]] bool is_enabled() const { return enabled_; }
    [[nodiscard]] bool is_visible_to_root() const;

    // ── Focus ──

    void set_focus_policy(FocusPolicy p) { focus_policy_ = p; }
    [[nodiscard]] FocusPolicy focus_policy() const { return focus_policy_; }

    void request_focus();
    void clear_focus();
    [[nodiscard]] bool has_focus() const { return focused_; }

    // ── Cursor ──

    void set_cursor(CursorShape shape);
    [[nodiscard]] CursorShape cursor() const { return cursor_shape_; }

    // ── Tooltip ──

    void set_tooltip(std::string_view tip) { tooltip_ = tip; }
    [[nodiscard]] std::string_view tooltip() const { return tooltip_; }

    // ── Layout ──

    void set_layout(std::unique_ptr<Layout> layout);
    [[nodiscard]] Layout* layout() const { return layout_.get(); }

    // Called by layout managers; also triggers invalidation.
    virtual void layout_children();

    [[nodiscard]] virtual WidgetSizeHint size_hint() const;

    void  set_min_size(SizeI s) { min_size_ = s; }
    void  set_max_size(SizeI s) { max_size_ = s; }
    [[nodiscard]] SizeI min_size() const { return min_size_; }
    [[nodiscard]] SizeI max_size() const { return max_size_; }

    // ── Margins / padding ──

    void set_contents_margins(InsetsI m) { margins_ = m; }
    [[nodiscard]] InsetsI contents_margins() const { return margins_; }

    // ── Tree ──

    [[nodiscard]] Widget*              parent()   const { return parent_; }
    [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }

    // Adds child, transferring ownership. Returns raw pointer for convenience.
    Widget* add_child(std::unique_ptr<Widget> child);

    // Convenience: construct and add in one step.
    template <typename T, typename... Args>
    T* emplace_child(Args&&... args) {
        auto child = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* raw = child.get();
        add_child(std::move(child));
        return raw;
    }

    std::unique_ptr<Widget> remove_child(Widget* child);

    // Hit test: returns deepest enabled+visible child containing local point, or this.
    [[nodiscard]] virtual Widget* hit_test(PointI local_pos);

    // ── Painting ──

    // Mark region dirty (in local coordinates). Pass empty rect for whole widget.
    void invalidate(RectI region = {});
    void invalidate_all() { invalidate(local_bounds()); }

    // ── Event handlers (override in subclasses) ──

    virtual void on_paint(Canvas& canvas);
    virtual void on_resize(const ResizeEvent& e);
    virtual void on_move(const MoveEvent& e);

    virtual void on_mouse_press(const MouseEvent& e);
    virtual void on_mouse_release(const MouseEvent& e);
    virtual void on_mouse_move(const MouseEvent& e);
    virtual void on_mouse_enter(const MouseEvent& e);
    virtual void on_mouse_leave(const MouseEvent& e);
    virtual void on_mouse_scroll(const MouseEvent& e);
    virtual void on_double_click(const MouseEvent& e);

    virtual void on_key_press(const KeyEvent& e);
    virtual void on_key_release(const KeyEvent& e);
    virtual void on_text_input(const TextInputEvent& e);

    virtual void on_focus_gain(const FocusEvent& e);
    virtual void on_focus_loss(const FocusEvent& e);

    // ── Callback slots (alternative to subclassing) ──

    std::function<void(Canvas&)>          cb_paint;
    std::function<void(const MouseEvent&)> cb_mouse_press;
    std::function<void(const MouseEvent&)> cb_mouse_release;
    std::function<void(const MouseEvent&)> cb_mouse_move;
    std::function<void(const KeyEvent&)>   cb_key_press;
    std::function<void(const FocusEvent&)> cb_focus_gain;
    std::function<void(const FocusEvent&)> cb_focus_loss;

protected:
    // Dispatch to children, then self. Called by the window root.
    void dispatch_paint(Canvas& canvas);
    void dispatch_mouse_event(const MouseEvent& e, bool entering);

    // Called when bounds change; default re-runs layout.
    virtual void on_bounds_changed(RectI old_bounds);

private:
    friend class Window;

    Widget*     parent_{nullptr};
    std::vector<std::unique_ptr<Widget>> children_;

    PointI pos_{0, 0};
    SizeI  size_{0, 0};
    SizeI  min_size_{0, 0};
    SizeI  max_size_{32767, 32767};
    InsetsI margins_{0};

    std::string  name_;
    std::string  tooltip_;
    bool         visible_{true};
    bool         enabled_{true};
    bool         focused_{false};
    FocusPolicy  focus_policy_{FocusPolicy::None};
    CursorShape  cursor_shape_{CursorShape::Arrow};

    std::unique_ptr<Layout> layout_;
};

} // namespace swole
