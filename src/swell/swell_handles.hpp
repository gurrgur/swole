#pragma once

#include "swell_abi.hpp"

#include "swole/window/window.hpp"
#include "swole/window/widget.hpp"
#include "swole/render/canvas.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <optional>

namespace swole::swell {

inline constexpr uint32_t kHwndMagic  = 0x48574E44;
inline constexpr uint32_t kHmenuMagic = 0x484D454E;
inline constexpr uint32_t kHdcMagic   = 0x48444321;

using WndProc = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);

enum class HwndKind {
    TopLevel,
    Child,
    Control
};

struct WindowLongs {
    LONG_PTR user_data{};
    LONG_PTR id{};
    LONG_PTR wndproc{};
    LONG_PTR style{};
    LONG_PTR exstyle{};
};

} // namespace swole::swell

struct HWND__ {
    uint32_t magic{swole::swell::kHwndMagic};
    swole::swell::HwndKind kind{swole::swell::HwndKind::Child};

    HWND parent{};
    std::vector<HWND> children;

    std::unique_ptr<swole::Window> window;
    swole::Widget* widget{};

    std::string class_name;
    std::string text;

    RECT bounds{};
    bool visible{true};
    bool enabled{true};
    bool destroyed{false};

    HMENU menu{};

    swole::swell::WndProc wndproc{};
    swole::swell::WindowLongs longs;

    std::unordered_map<std::string, HANDLE> props;
};

struct HMENU__;

struct MenuItem {
    UINT id{};
    std::string text;
    bool enabled{true};
    bool checked{false};
    bool separator{false};
    std::unique_ptr<HMENU__> submenu;
};

struct HMENU__ {
    uint32_t magic{swole::swell::kHmenuMagic};
    std::vector<MenuItem> items;
    HWND destination{};
};

struct HDC__ {
    uint32_t magic{swole::swell::kHdcMagic};
    HWND owner{};
    swole::Canvas* canvas{};
    RECT clip{};

    HGDIOBJ selected_font{};
    HGDIOBJ selected_pen{};
    HGDIOBJ selected_brush{};
    COLORREF text_color{0};
    COLORREF bk_color{0xFFFFFF};
    int bk_mode{1};
    int stretch_mode{3};
    int stretch_blit_mode{3};

    POINT current_pos{};
};

namespace swole::swell {

inline HWND__* as_hwnd(HWND h) {
    auto* p = reinterpret_cast<HWND__*>(h);
    return p && p->magic == kHwndMagic && !p->destroyed ? p : nullptr;
}

inline HMENU__* as_hmenu(HMENU h) {
    auto* p = reinterpret_cast<HMENU__*>(h);
    return p && p->magic == kHmenuMagic ? p : nullptr;
}

inline HDC__* as_hdc(HDC h) {
    auto* p = reinterpret_cast<HDC__*>(h);
    return p && p->magic == kHdcMagic ? p : nullptr;
}

inline swole::Window* window_from_hwnd(HWND h) {
    for (auto* p = as_hwnd(h); p; p = as_hwnd(p->parent)) {
        if (p->window) return p->window.get();
    }
    return nullptr;
}

inline swole::Widget* widget_from_hwnd(HWND h) {
    auto* p = as_hwnd(h);
    if (!p) return nullptr;
    if (p->widget) return p->widget;
    if (p->window) return &p->window->root();
    return nullptr;
}

inline RectI to_swole_rect(const RECT& r) {
    return {int(r.left), int(r.top), int(r.right - r.left), int(r.bottom - r.top)};
}

inline RECT to_rect(RectI r) {
    return {r.x, r.y, r.x + r.w, r.y + r.h};
}

} // namespace swole::swell
