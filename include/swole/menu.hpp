#pragma once

#include "core/types.hpp"
#include "core/event.hpp"
#include "window/widget.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace swole {

class Window;

enum class MenuItemKind { Action, Separator, SubMenu };

// Leaf item representing an action, a separator, or a submenu.
class MenuItem {
public:
    explicit MenuItem(std::string_view label);

    // Factory helpers
    static MenuItem action(std::string_view label, std::function<void()> callback = {});
    static MenuItem separator();
    static MenuItem submenu(std::string_view label, std::unique_ptr<class Menu> sub);

    void set_enabled(bool v) { enabled_ = v; }
    void set_checked(bool v) { checked_ = v; }
    void set_shortcut(Key key, KeyMods mods = {});
    void set_icon(/* Image icon */);

    [[nodiscard]] bool        is_enabled()   const { return enabled_; }
    [[nodiscard]] bool        is_checked()   const { return checked_; }
    [[nodiscard]] MenuItemKind kind()        const { return kind_; }
    [[nodiscard]] std::string_view label()   const { return label_; }
    [[nodiscard]] Menu*        submenu()     const { return submenu_.get(); }

    void trigger() { if (on_triggered) on_triggered(); }

    std::function<void()> on_triggered;

private:
    std::string   label_;
    MenuItemKind  kind_{MenuItemKind::Action};
    bool          enabled_{true};
    bool          checked_{false};
    Key           shortcut_key_{Key::Unknown};
    KeyMods       shortcut_mods_{};
    std::unique_ptr<Menu> submenu_;
};

// Ordered collection of MenuItems.
class Menu {
public:
    Menu() = default;

    MenuItem& add_action(std::string_view label, std::function<void()> fn = {});
    MenuItem& add_separator();
    Menu&     add_submenu(std::string_view label, std::unique_ptr<Menu> sub);

    [[nodiscard]] int item_count() const { return int(items_.size()); }
    [[nodiscard]] MenuItem& item(int i) { return *items_[i]; }

    // Show as a floating popup at global screen coordinates.
    // Returns the triggered MenuItem* or nullptr if dismissed.
    MenuItem* exec(Window& parent, PointI screen_pos);

private:
    std::vector<std::unique_ptr<MenuItem>> items_;
};

// Menu bar pinned to the top of a window.
class MenuBar : public Widget {
public:
    explicit MenuBar(Widget* parent = nullptr);

    Menu& add_menu(std::string_view title);
    void  remove_menu(int index);
    [[nodiscard]] int menu_count() const { return int(menus_.size()); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;

private:
    struct Entry { std::string title; std::unique_ptr<Menu> menu; };
    std::vector<Entry> menus_;
    int open_index_{-1};
};

} // namespace swole
