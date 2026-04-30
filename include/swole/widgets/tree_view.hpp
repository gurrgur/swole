#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace swole {

struct TreeItem {
    std::string  text;
    void*        user_data{nullptr};
    bool         expanded{false};
    bool         enabled{true};

    // Children are owned here; the tree owns the root items.
    std::vector<std::unique_ptr<TreeItem>> children;

    // Parent back-pointer (raw, non-owning).
    TreeItem* parent{nullptr};

    TreeItem* add_child(std::string_view t, void* data = nullptr);
};

class TreeView : public Widget {
public:
    explicit TreeView(Widget* parent = nullptr);

    // Root-level items
    TreeItem* add_root_item(std::string_view text, void* user_data = nullptr);
    void      remove_item(TreeItem* item);
    void      clear();

    [[nodiscard]] int root_item_count() const { return int(roots_.size()); }

    // Selection
    void     select(TreeItem* item);
    void     deselect_all();
    [[nodiscard]] TreeItem* selected_item() const { return selected_; }

    // Expand / collapse
    void expand(TreeItem* item);
    void collapse(TreeItem* item);
    void expand_all();
    void collapse_all();
    void ensure_visible(TreeItem* item);

    // Appearance
    void set_font(Font font);
    void set_indent(int px) { indent_ = px; invalidate(); }
    void set_item_height(int h) { item_height_ = h; invalidate(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    Signal<TreeItem*> on_selection_changed;
    Signal<TreeItem*> on_item_activated;
    Signal<TreeItem*> on_item_expanded;
    Signal<TreeItem*> on_item_collapsed;
    Signal<TreeItem*, PointI> on_context_menu;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_double_click(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;
    void on_mouse_scroll(const MouseEvent& e) override;

private:
    struct FlatItem { TreeItem* item; int depth; };
    void  build_flat_list();
    [[nodiscard]] int  flat_index_at(PointI local_pos) const;

    std::vector<std::unique_ptr<TreeItem>> roots_;
    std::vector<FlatItem>  flat_;  // rebuilt after expand/collapse
    TreeItem*  selected_{nullptr};
    int        scroll_offset_{0};
    int        item_height_{20};
    int        indent_{16};
    Font       font_;
    bool       flat_dirty_{true};
};

} // namespace swole
