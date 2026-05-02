#include "swell_widget_host.hpp"

#include "swole/layout/layout.hpp"

namespace swole::swell {

HwndWidget::HwndWidget(HWND hwnd, swole::Widget* parent)
    : Widget(parent), hwnd_(hwnd) {
    set_focus_policy(FocusPolicy::Tab);
}

static LPARAM make_xy_lparam(int x, int y) {
    return (uint16_t(x) & 0xffff) | ((uint32_t(uint16_t(y)) & 0xffff) << 16);
}

void HwndWidget::on_paint(swole::Canvas& canvas) {
    HDC__ dc;
    dc.owner = hwnd_;
    dc.canvas = &canvas;
    dc.clip = to_rect(local_bounds());

    swell_send_message(hwnd_, 0x000F, 0, reinterpret_cast<LPARAM>(&dc));
}

void HwndWidget::on_resize(const swole::ResizeEvent& e) {
    auto* h = as_hwnd(hwnd_);
    if (h) h->bounds = to_rect(bounds());

    LPARAM lp = make_xy_lparam(e.new_size.w, e.new_size.h);
    swell_send_message(hwnd_, 0x0005, 0, lp);
}

void HwndWidget::on_mouse_press(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0201, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_mouse_release(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0202, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_mouse_move(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0200, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_mouse_enter(const swole::MouseEvent&) {
    swell_send_message(hwnd_, 0x02A0, 0, 0);
}

void HwndWidget::on_mouse_leave(const swole::MouseEvent&) {
    swell_send_message(hwnd_, 0x02A3, 0, 0);
}

void HwndWidget::on_mouse_scroll(const swole::MouseEvent& e) {
    WPARAM wp = WPARAM(int(e.wheel.y * 120));
    swell_send_message(hwnd_, 0x020A, wp, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_key_press(const swole::KeyEvent& e) {
    swell_send_message(hwnd_, 0x0100, WPARAM(e.key), 0);
}

void HwndWidget::on_key_release(const swole::KeyEvent& e) {
    swell_send_message(hwnd_, 0x0101, WPARAM(e.key), 0);
}

void HwndWidget::on_text_input(const swole::TextInputEvent& e) {
    swell_send_message(hwnd_, 0x0102, 0, reinterpret_cast<LPARAM>(e.text.c_str()));
}

void HwndWidget::on_focus_gain(const swole::FocusEvent&) {
    swell_send_message(hwnd_, 0x0007, 0, 0);
}

void HwndWidget::on_focus_loss(const swole::FocusEvent&) {
    swell_send_message(hwnd_, 0x0008, 0, 0);
}

WidgetSizeHint HwndWidget::size_hint() const {
    auto* h = as_hwnd(hwnd_);
    if (!h) return Widget::size_hint();

    WidgetSizeHint hint;
    hint.preferred_size = {
        int(h->bounds.right - h->bounds.left),
        int(h->bounds.bottom - h->bounds.top)
    };
    hint.h_policy = SizePolicy::Fixed;
    hint.v_policy = SizePolicy::Fixed;
    return hint;
}

LRESULT swell_def_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    (void)hwnd;
    (void)msg;
    (void)wp;
    (void)lp;
    return 0;
}

LRESULT swell_send_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* h = as_hwnd(hwnd);
    if (!h) return 0;

    if (h->wndproc)
        return h->wndproc(hwnd, msg, wp, lp);

    if (h->longs.wndproc)
        return reinterpret_cast<WndProc>(h->longs.wndproc)(hwnd, msg, wp, lp);

    return swell_def_window_proc(hwnd, msg, wp, lp);
}

} // namespace swole::swell
