#pragma once

#include "swell_handles.hpp"

namespace swole::swell {

class HwndWidget final : public swole::Widget {
public:
    explicit HwndWidget(HWND hwnd, swole::Widget* parent = nullptr);

    HWND hwnd() const { return hwnd_; }

    void on_paint(swole::Canvas& canvas) override;
    void on_resize(const swole::ResizeEvent& e) override;
    void on_mouse_press(const swole::MouseEvent& e) override;
    void on_mouse_release(const swole::MouseEvent& e) override;
    void on_mouse_move(const swole::MouseEvent& e) override;
    void on_mouse_enter(const swole::MouseEvent& e) override;
    void on_mouse_leave(const swole::MouseEvent& e) override;
    void on_mouse_scroll(const swole::MouseEvent& e) override;
    void on_key_press(const swole::KeyEvent& e) override;
    void on_key_release(const swole::KeyEvent& e) override;
    void on_text_input(const swole::TextInputEvent& e) override;
    void on_focus_gain(const swole::FocusEvent& e) override;
    void on_focus_loss(const swole::FocusEvent& e) override;

    WidgetSizeHint size_hint() const override;

private:
    HWND hwnd_{};
};

LRESULT swell_send_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT swell_def_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

} // namespace swole::swell
