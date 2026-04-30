#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <functional>
#include <string>

namespace swole {

enum class ButtonVariant { Push, Toggle, Check, Radio };

class Button : public Widget {
public:
    explicit Button(Widget* parent = nullptr, std::string_view label = {});

    void set_label(std::string_view text);
    [[nodiscard]] std::string_view label() const { return label_; }

    // Variant
    void set_variant(ButtonVariant v);
    [[nodiscard]] ButtonVariant variant() const { return variant_; }

    // Checked state (Toggle / Check / Radio)
    void set_checked(bool v);
    [[nodiscard]] bool is_checked() const { return checked_; }

    // Radio group: auto-uncheck siblings in the same parent with variant==Radio
    void set_radio_group(int group_id) { radio_group_ = group_id; }

    void set_font(Font font);

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    // Signals
    Signal<> on_clicked;
    Signal<bool> on_toggled;   // for Check/Toggle variants

protected:
    void on_paint(Canvas& canvas) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_release(const MouseEvent& e) override;
    void on_mouse_enter(const MouseEvent& e) override;
    void on_mouse_leave(const MouseEvent& e) override;
    void on_key_press(const KeyEvent& e) override;

private:
    std::string    label_;
    ButtonVariant  variant_{ButtonVariant::Push};
    bool           checked_{false};
    bool           pressed_{false};
    bool           hovered_{false};
    int            radio_group_{0};
    Font           font_;
};

} // namespace swole
