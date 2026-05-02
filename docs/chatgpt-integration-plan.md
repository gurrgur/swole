Your repo is already shaped well for this. The clean path is:

**Do not make `HWND` equal to `swole::Widget*`.**
Make `HWND` an opaque SWELL shell that owns or points at your modern C++ `swole::Window` / `swole::Widget`.

Why: your `Widget` tree is already owned by `unique_ptr`s and is strictly tree-shaped, with `Window -> root -> children`. It has modern geometry, focus, event, and paint APIs already. ([GitHub][1]) Your top-level `Window` is already SDL3 + Skia-backed and owns the root widget/rendering context. ([GitHub][2]) Meanwhile your SWELL layer currently defines `HWND`, `HMENU`, and `HDC` as opaque pointer types. ([GitHub][3]) That is exactly the opening you need.

The missing piece is a **handle bridge layer** under `src/swell`.

---

## Current state

You already have:

```cpp
using HWND  = struct HWND__*;
using HMENU = struct HMENU__*;
using HDC   = struct HDC__*;
```

in `swell_abi.hpp`. ([GitHub][3])

And you already have a large SWELL dispatch table, but most actual window/widget/menu/GDI functions are stubs: `GetDlgItem`, `ShowWindow`, `DestroyWindow`, `InvalidateRect`, `SendMessage`, `CreatePopupMenu`, `TrackPopupMenu`, `BeginPaint`, `EndPaint`, etc. are currently routed to `API_STUB`. ([GitHub][4])

So implement these by introducing three private structs:

```cpp
struct HWND__;
struct HMENU__;
struct HDC__;
```

backed by your C++ objects.

---

# 1. Add `src/swell/swell_handles.hpp`

```cpp
#pragma once

#include "swell_abi.hpp"

#include "swole/window/window.hpp"
#include "swole/window/widget.hpp"
#include "swole/menu.hpp"
#include "swole/render/canvas.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <optional>

namespace swole::swell {

inline constexpr uint32_t kHwndMagic  = 0x48574E44; // "HWND"
inline constexpr uint32_t kHmenuMagic = 0x484D454E; // "HMEN"
inline constexpr uint32_t kHdcMagic   = 0x48444321; // "HDC!"

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

struct HWND__ {
    uint32_t magic{kHwndMagic};
    HwndKind kind{HwndKind::Child};

    HWND parent{};
    std::vector<HWND> children;

    // Top-level HWND owns a real swole::Window.
    std::unique_ptr<swole::Window> window;

    // Child/control HWND owns a swole::Widget inserted into the parent tree.
    swole::Widget* widget{};

    std::string class_name;
    std::string text;

    RECT bounds{};
    bool visible{true};
    bool enabled{true};
    bool destroyed{false};

    HMENU menu{};

    WndProc wndproc{};
    WindowLongs longs;

    std::unordered_map<std::string, HANDLE> props;
};

struct MenuItem {
    UINT id{};
    std::string text;
    bool enabled{true};
    bool checked{false};
    std::unique_ptr<HMENU__> submenu;
};

struct HMENU__ {
    uint32_t magic{kHmenuMagic};
    std::vector<MenuItem> items;
    HWND destination{};
};

struct HDC__ {
    uint32_t magic{kHdcMagic};

    // For now, HDC is a lightweight drawing session around your Canvas.
    // Later this can grow selected font/brush/pen state.
    HWND owner{};
    swole::Canvas* canvas{};
    RECT clip{};
};

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
```

This keeps your ABI C-shaped while letting C++ stay clean.

---

# 2. Add a SWELL host widget

You need a widget that can receive typed `swole` events and translate them into SWELL/Win32-ish messages.

Add `src/swell/swell_widget_host.hpp`:

```cpp
#pragma once

#include "swell_handles.hpp"

namespace swole::swell {

class HwndWidget final : public swole::Widget {
public:
    explicit HwndWidget(HWND hwnd, swole::Widget* parent = nullptr)
        : Widget(parent), hwnd_(hwnd) {}

    HWND hwnd() const { return hwnd_; }

    void on_paint(swole::Canvas& canvas) override;
    void on_resize(const swole::ResizeEvent& e) override;
    void on_mouse_press(const swole::MouseEvent& e) override;
    void on_mouse_release(const swole::MouseEvent& e) override;
    void on_mouse_move(const swole::MouseEvent& e) override;
    void on_key_press(const swole::KeyEvent& e) override;
    void on_key_release(const swole::KeyEvent& e) override;

private:
    HWND hwnd_{};
};

LRESULT swell_send_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT swell_def_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

} // namespace swole::swell
```

Add `src/swell/swell_widget_host.cpp`:

```cpp
#include "swell_widget_host.hpp"

namespace swole::swell {

static LPARAM make_xy_lparam(int x, int y) {
    return (uint16_t(x) & 0xffff) | ((uint32_t(uint16_t(y)) & 0xffff) << 16);
}

void HwndWidget::on_paint(swole::Canvas& canvas) {
    HDC__ dc;
    dc.owner = hwnd_;
    dc.canvas = &canvas;
    dc.clip = to_rect(local_bounds());

    // SWELL/Win32-ish: WM_PAINT with an HDC-ish drawing context.
    swell_send_message(hwnd_, 0x000F /* WM_PAINT */, 0, reinterpret_cast<LPARAM>(&dc));
}

void HwndWidget::on_resize(const swole::ResizeEvent& e) {
    auto* h = as_hwnd(hwnd_);
    if (h) h->bounds = to_rect(bounds());

    LPARAM lp = make_xy_lparam(e.new_size.w, e.new_size.h);
    swell_send_message(hwnd_, 0x0005 /* WM_SIZE */, 0, lp);
}

void HwndWidget::on_mouse_press(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0201 /* WM_LBUTTONDOWN */, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_mouse_release(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0202 /* WM_LBUTTONUP */, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_mouse_move(const swole::MouseEvent& e) {
    swell_send_message(hwnd_, 0x0200 /* WM_MOUSEMOVE */, 0, make_xy_lparam(e.pos.x, e.pos.y));
}

void HwndWidget::on_key_press(const swole::KeyEvent& e) {
    swell_send_message(hwnd_, 0x0100 /* WM_KEYDOWN */, WPARAM(e.key), 0);
}

void HwndWidget::on_key_release(const swole::KeyEvent& e) {
    swell_send_message(hwnd_, 0x0101 /* WM_KEYUP */, WPARAM(e.key), 0);
}

} // namespace swole::swell
```

This is the main bridge: modern `Widget` event → crusty `HWND` message.

---

# 3. Add SWELL window creation/destruction

Create `src/swell/swell_window.cpp`:

```cpp
#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include "swole/window/window.hpp"
#include "swole/window/widget.hpp"

#include <algorithm>
#include <memory>

namespace swole::swell {

static void unlink_child(HWND parent, HWND child) {
    auto* p = as_hwnd(parent);
    if (!p) return;

    std::erase(p->children, child);
}

static void destroy_hwnd_tree(HWND hwnd) {
    auto* h = as_hwnd(hwnd);
    if (!h) return;

    auto children = h->children;
    for (HWND child : children)
        destroy_hwnd_tree(child);

    h->children.clear();

    if (h->parent)
        unlink_child(h->parent, hwnd);

    h->destroyed = true;
    h->magic = 0;

    delete h;
}

LRESULT swell_def_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    (void)wp;
    (void)lp;

    switch (msg) {
        case 0x0002: // WM_DESTROY
            return 0;

        case 0x000F: // WM_PAINT
            return 0;

        default:
            return 0;
    }
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

extern "C" {

HWND CreateWindowEx(
    DWORD exstyle,
    const char* class_name,
    const char* window_name,
    DWORD style,
    int x,
    int y,
    int w,
    int h,
    HWND parent,
    HMENU menu,
    HINSTANCE,
    LPVOID param
) {
    using namespace swole::swell;

    auto* hwnd = new HWND__{};
    hwnd->class_name = class_name ? class_name : "";
    hwnd->text = window_name ? window_name : "";
    hwnd->parent = parent;
    hwnd->menu = menu;
    hwnd->longs.style = style;
    hwnd->longs.exstyle = exstyle;
    hwnd->bounds = {x, y, x + w, y + h};

    if (!parent) {
        swole::WindowConfig cfg;
        cfg.title = hwnd->text;
        cfg.size = {w > 0 ? w : 800, h > 0 ? h : 600};
        cfg.position = {x, y};

        hwnd->kind = HwndKind::TopLevel;
        hwnd->window = std::make_unique<swole::Window>(cfg);
        hwnd->widget = &hwnd->window->root();
    } else {
        auto* ph = as_hwnd(parent);
        auto* parent_widget = widget_from_hwnd(parent);

        hwnd->kind = HwndKind::Child;

        auto child = std::make_unique<HwndWidget>(reinterpret_cast<HWND>(hwnd), parent_widget);
        child->set_bounds({x, y, w, h});

        hwnd->widget = child.get();

        if (parent_widget)
            parent_widget->add_child(std::move(child));

        if (ph)
            ph->children.push_back(reinterpret_cast<HWND>(hwnd));
    }

    // WM_CREATE
    swell_send_message(reinterpret_cast<HWND>(hwnd), 0x0001, 0, reinterpret_cast<LPARAM>(param));

    return reinterpret_cast<HWND>(hwnd);
}

BOOL DestroyWindow(HWND hwnd) {
    using namespace swole::swell;

    if (!as_hwnd(hwnd))
        return SWELL_FALSE;

    swell_send_message(hwnd, 0x0002 /* WM_DESTROY */, 0, 0);
    destroy_hwnd_tree(hwnd);
    return SWELL_TRUE;
}

BOOL ShowWindow(HWND hwnd, int cmd) {
    using namespace swole::swell;

    auto* h = as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    h->visible = cmd != 0;

    if (h->window) {
        if (h->visible) h->window->show();
        else h->window->hide();
    } else if (h->widget) {
        h->widget->set_visible(h->visible);
    }

    return SWELL_TRUE;
}

BOOL IsWindow(HWND hwnd) {
    return swole::swell::as_hwnd(hwnd) ? SWELL_TRUE : SWELL_FALSE;
}

BOOL IsWindowVisible(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h && h->visible ? SWELL_TRUE : SWELL_FALSE;
}

BOOL EnableWindow(HWND hwnd, BOOL enable) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    h->enabled = enable != 0;
    if (h->widget)
        h->widget->set_enabled(h->enabled);

    return SWELL_TRUE;
}

BOOL IsWindowEnabled(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h && h->enabled ? SWELL_TRUE : SWELL_FALSE;
}

}
```

Then change `swell_dispatch.cpp` from stubs to implemented entries:

```cpp
API_IMPL(CreateWindowEx),
API_IMPL(ShowWindow),
API_IMPL(DestroyWindow),
API_IMPL(IsWindow),
API_IMPL(IsWindowVisible),
API_IMPL(EnableWindow),
API_IMPL(IsWindowEnabled),
```

Your dispatch table currently has these as stubs. ([GitHub][4])

---

# 4. Implement basic geometry APIs

Add to `swell_window.cpp`:

```cpp
extern "C" {

BOOL GetClientRect(HWND hwnd, RECT* r) {
    auto* w = swole::swell::widget_from_hwnd(hwnd);
    if (!w || !r) return SWELL_FALSE;

    auto b = w->local_bounds();
    *r = {0, 0, b.w, b.h};
    return SWELL_TRUE;
}

BOOL GetWindowRect(HWND hwnd, RECT* r) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !r) return SWELL_FALSE;

    *r = h->bounds;
    return SWELL_TRUE;
}

BOOL SetWindowPos(
    HWND hwnd,
    HWND,
    int x,
    int y,
    int cx,
    int cy,
    UINT flags
) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    // Minimal SWP_NOMOVE / SWP_NOSIZE support.
    constexpr UINT SWP_NOSIZE = 0x0001;
    constexpr UINT SWP_NOMOVE = 0x0002;

    RECT old = h->bounds;

    if (!(flags & SWP_NOMOVE)) {
        h->bounds.left = x;
        h->bounds.top = y;
    }

    if (!(flags & SWP_NOSIZE)) {
        h->bounds.right = h->bounds.left + cx;
        h->bounds.bottom = h->bounds.top + cy;
    } else {
        int old_w = old.right - old.left;
        int old_h = old.bottom - old.top;
        h->bounds.right = h->bounds.left + old_w;
        h->bounds.bottom = h->bounds.top + old_h;
    }

    if (h->window) {
        if (!(flags & SWP_NOMOVE))
            h->window->set_position({h->bounds.left, h->bounds.top});
        if (!(flags & SWP_NOSIZE))
            h->window->set_size({h->bounds.right - h->bounds.left,
                                 h->bounds.bottom - h->bounds.top});
    } else if (h->widget) {
        h->widget->set_bounds(swole::swell::to_swole_rect(h->bounds));
    }

    return SWELL_TRUE;
}

BOOL MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint) {
    BOOL ok = SetWindowPos(hwnd, nullptr, x, y, w, h, 0);
    if (ok && repaint)
        InvalidateRect(hwnd, nullptr, SWELL_TRUE);
    return ok;
}

BOOL InvalidateRect(HWND hwnd, const RECT* r, BOOL) {
    auto* w = swole::swell::widget_from_hwnd(hwnd);
    if (!w) return SWELL_FALSE;

    if (r) {
        w->invalidate({
            int(r->left),
            int(r->top),
            int(r->right - r->left),
            int(r->bottom - r->top)
        });
    } else {
        w->invalidate_all();
    }

    return SWELL_TRUE;
}

BOOL UpdateWindow(HWND hwnd) {
    auto* win = swole::swell::window_from_hwnd(hwnd);
    if (!win) return SWELL_FALSE;

    win->repaint_now();
    return SWELL_TRUE;
}

}
```

Also update dispatch:

```cpp
API_IMPL(GetClientRect),
API_IMPL(GetWindowRect),
API_IMPL(SetWindowPos),
API_IMPL(MoveWindow),
API_IMPL(InvalidateRect),
API_IMPL(UpdateWindow),
```

One important repo-side TODO: `Widget::invalidate()` is currently empty. ([GitHub][5]) For the bridge to feel right, make it trigger the owning window’s repaint:

```cpp
void Widget::invalidate(RectI region) {
    (void)region;
    if (Window* w = window())
        w->repaint_now();
}
```

Later, replace that with coalesced dirty rects instead of immediate repaint.

---

# 5. Implement `SendMessage`, `PostMessage`, and `DefWindowProc`

Add `src/swell/swell_messages.cpp`:

```cpp
#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include <deque>
#include <mutex>

namespace {

struct QueuedMessage {
    HWND hwnd{};
    UINT msg{};
    WPARAM wp{};
    LPARAM lp{};
};

std::mutex g_queue_mutex;
std::deque<QueuedMessage> g_queue;

} // namespace

extern "C" {

LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return swole::swell::swell_def_window_proc(hwnd, msg, wp, lp);
}

LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return swole::swell::swell_send_message(hwnd, msg, wp, lp);
}

BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (!swole::swell::as_hwnd(hwnd))
        return SWELL_FALSE;

    std::lock_guard lock{g_queue_mutex};
    g_queue.push_back({hwnd, msg, wp, lp});
    return SWELL_TRUE;
}

void SWELL_MessageQueue_Flush(HWND hwnd) {
    std::deque<QueuedMessage> local;

    {
        std::lock_guard lock{g_queue_mutex};

        for (auto it = g_queue.begin(); it != g_queue.end();) {
            if (!hwnd || it->hwnd == hwnd) {
                local.push_back(*it);
                it = g_queue.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (const auto& m : local)
        swole::swell::swell_send_message(m.hwnd, m.msg, m.wp, m.lp);
}

void SWELL_MessageQueue_Clear(HWND hwnd) {
    std::lock_guard lock{g_queue_mutex};

    if (!hwnd) {
        g_queue.clear();
        return;
    }

    std::erase_if(g_queue, [hwnd](const QueuedMessage& m) {
        return m.hwnd == hwnd;
    });
}

}
```

Then in `SWELL_RunMessageLoop()`, call the queue flush instead of only sleeping:

```cpp
extern "C" void SWELL_MessageQueue_Flush(HWND hwnd);

void SWELL_RunMessageLoop() {
    SWELL_MessageQueue_Flush(nullptr);
    swole::Application::instance().poll_once(); // or whatever your app API exposes
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

Your current `SWELL_RunMessageLoop()` only sleeps because HWND/event dispatch does not exist yet. ([GitHub][6]) This is where the message bridge should hook in.

---

# 6. Implement `GetWindowLong` / `SetWindowLong`

SWELL/WDL code will absolutely depend on these.

```cpp
extern "C" {

LONG_PTR GetWindowLongPtr(HWND hwnd, int index) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return 0;

    switch (index) {
        case -21: return h->longs.user_data; // GWLP_USERDATA
        case -12: return h->longs.id;        // GWLP_ID
        case -4:  return h->longs.wndproc;   // GWLP_WNDPROC
        case -16: return h->longs.style;     // GWL_STYLE
        case -20: return h->longs.exstyle;   // GWL_EXSTYLE
        default:  return 0;
    }
}

LONG_PTR SetWindowLongPtr(HWND hwnd, int index, LONG_PTR value) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return 0;

    LONG_PTR old = 0;

    switch (index) {
        case -21:
            old = h->longs.user_data;
            h->longs.user_data = value;
            break;

        case -12:
            old = h->longs.id;
            h->longs.id = value;
            break;

        case -4:
            old = h->longs.wndproc;
            h->longs.wndproc = value;
            break;

        case -16:
            old = h->longs.style;
            h->longs.style = value;
            break;

        case -20:
            old = h->longs.exstyle;
            h->longs.exstyle = value;
            break;
    }

    return old;
}

LONG GetWindowLong(HWND hwnd, int index) {
    return LONG(GetWindowLongPtr(hwnd, index));
}

LONG SetWindowLong(HWND hwnd, int index, LONG value) {
    return LONG(SetWindowLongPtr(hwnd, index, value));
}

}
```

Update dispatch:

```cpp
API_IMPL(GetWindowLong),
API_IMPL(SetWindowLong),
API_IMPL(GetWindowLongPtr),
API_IMPL(SetWindowLongPtr),
```

Even if WDL mostly asks for `GetWindowLong`, you want the `Ptr` versions internally.

---

# 7. Implement child lookup / dialog control IDs

Your `HWND__::longs.id` gives you `GetDlgItem`.

```cpp
extern "C" {

HWND GetDlgItem(HWND parent, int id) {
    auto* p = swole::swell::as_hwnd(parent);
    if (!p) return nullptr;

    for (HWND child : p->children) {
        auto* c = swole::swell::as_hwnd(child);
        if (!c) continue;

        if (int(c->longs.id) == id)
            return child;

        if (HWND nested = GetDlgItem(child, id))
            return nested;
    }

    return nullptr;
}

BOOL IsChild(HWND parent, HWND child) {
    auto* p = swole::swell::as_hwnd(parent);
    if (!p || !child) return SWELL_FALSE;

    for (HWND c : p->children) {
        if (c == child)
            return SWELL_TRUE;
        if (IsChild(c, child))
            return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

}
```

Update dispatch:

```cpp
API_IMPL(GetDlgItem),
API_IMPL(IsChild),
```

---

# 8. Implement text APIs

```cpp
extern "C" {

void SetWindowText(HWND hwnd, const char* text) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return;

    h->text = text ? text : "";

    if (h->window)
        h->window->set_title(h->text);

    if (h->widget)
        h->widget->invalidate_all();
}

int GetWindowText(HWND hwnd, char* buf, int maxlen) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !buf || maxlen <= 0) return 0;

    int n = std::min<int>(maxlen - 1, h->text.size());
    std::memcpy(buf, h->text.data(), n);
    buf[n] = 0;
    return n;
}

int GetWindowTextLength(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h ? int(h->text.size()) : 0;
}

void SetDlgItemText(HWND hwnd, int id, const char* text) {
    SetWindowText(GetDlgItem(hwnd, id), text);
}

int GetDlgItemText(HWND hwnd, int id, char* buf, int maxlen) {
    return GetWindowText(GetDlgItem(hwnd, id), buf, maxlen);
}

}
```

Update dispatch:

```cpp
API_IMPL(SetWindowText),
API_IMPL(GetWindowText),
API_IMPL(GetWindowTextLength),
API_IMPL(SetDlgItemText),
API_IMPL(GetDlgItemText),
```

---

# 9. Menus: keep `HMENU` as a model, dispatch `WM_COMMAND`

Add `src/swell/swell_menu.cpp`:

```cpp
#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include <algorithm>

extern "C" {

HMENU CreatePopupMenu() {
    return reinterpret_cast<HMENU>(new swole::swell::HMENU__{});
}

HMENU CreatePopupMenuEx(const char*) {
    return CreatePopupMenu();
}

BOOL DestroyMenu(HMENU menu) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    m->magic = 0;
    delete m;
    return SWELL_TRUE;
}

int GetMenuItemCount(HMENU menu) {
    auto* m = swole::swell::as_hmenu(menu);
    return m ? int(m->items.size()) : -1;
}

UINT GetMenuItemID(HMENU menu, int pos) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m || pos < 0 || pos >= int(m->items.size()))
        return UINT(-1);

    return m->items[size_t(pos)].id;
}

BOOL SWELL_InsertMenu(
    HMENU menu,
    int pos,
    int flags,
    int id,
    const char* text
) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    swole::swell::MenuItem item;
    item.id = UINT(id);
    item.text = text ? text : "";

    if (pos < 0 || pos > int(m->items.size()))
        pos = int(m->items.size());

    m->items.insert(m->items.begin() + pos, std::move(item));
    return SWELL_TRUE;
}

BOOL EnableMenuItem(HMENU menu, UINT id, UINT flags) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    // Minimal: MF_GRAYED / MF_DISABLED means disabled.
    constexpr UINT MF_GRAYED = 0x0001;
    constexpr UINT MF_DISABLED = 0x0002;

    for (auto& item : m->items) {
        if (item.id == id) {
            item.enabled = !(flags & (MF_GRAYED | MF_DISABLED));
            return SWELL_TRUE;
        }
    }

    return SWELL_FALSE;
}

BOOL CheckMenuItem(HMENU menu, UINT id, UINT flags) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    constexpr UINT MF_CHECKED = 0x0008;

    for (auto& item : m->items) {
        if (item.id == id) {
            item.checked = (flags & MF_CHECKED) != 0;
            return SWELL_TRUE;
        }
    }

    return SWELL_FALSE;
}

BOOL SetMenu(HWND hwnd, HMENU menu) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    h->menu = menu;
    return SWELL_TRUE;
}

HMENU GetMenu(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h ? h->menu : nullptr;
}

void SWELL_SetMenuDestination(HMENU menu, HWND hwnd) {
    auto* m = swole::swell::as_hmenu(menu);
    if (m) m->destination = hwnd;
}

BOOL TrackPopupMenu(
    HMENU menu,
    UINT,
    int,
    int,
    int,
    HWND hwnd,
    const RECT*
) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    HWND dest = m->destination ? m->destination : hwnd;

    // Minimal non-UI implementation: choose first enabled item.
    // Replace later with a real swole::Menu popup widget.
    for (const auto& item : m->items) {
        if (item.enabled && item.id != 0) {
            swole::swell::swell_send_message(
                dest,
                0x0111 /* WM_COMMAND */,
                WPARAM(item.id),
                0
            );
            return SWELL_TRUE;
        }
    }

    return SWELL_FALSE;
}

}
```

This is intentionally minimal. It gives WDL/SWELL code the command routing it expects, while you later replace `TrackPopupMenu` with a real popup window built from your `Menu`/widget system.

---

# 10. `HDC`: start with paint-session wrapper, not full GDI

Your `Canvas` class is already the right high-level API and explicitly states it is valid only for the duration of paint. ([GitHub][7]) So make `HDC` the same: a short-lived drawing context.

Basic version:

```cpp
extern "C" {

HDC BeginPaint(HWND hwnd, void* paintstruct) {
    (void)paintstruct;

    // In this architecture WM_PAINT gets a temporary HDC via LPARAM.
    // If old code explicitly calls BeginPaint, create a temporary HDC
    // but it cannot draw until attached to a Canvas.
    auto* dc = new swole::swell::HDC__{};
    dc->owner = hwnd;
    return reinterpret_cast<HDC>(dc);
}

BOOL EndPaint(HWND, void*) {
    return SWELL_TRUE;
}

HDC GetWindowDC(HWND hwnd) {
    auto* dc = new swole::swell::HDC__{};
    dc->owner = hwnd;
    return reinterpret_cast<HDC>(dc);
}

int ReleaseDC(HWND, HDC hdc) {
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc) return 0;

    dc->magic = 0;
    delete dc;
    return 1;
}

}
```

For actual GDI drawing functions, map them to `Canvas` if `dc->canvas` exists:

```cpp
extern "C" {

void SWELL_FillRect(HDC hdc, const RECT* r, HGDIOBJ brush) {
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc || !dc->canvas || !r) return;

    // Later: decode brush.
    swole::Paint p = swole::Paint::fill({220, 220, 220, 255});

    dc->canvas->draw_rect(
        swole::RectF{
            float(r->left),
            float(r->top),
            float(r->right - r->left),
            float(r->bottom - r->top)
        },
        p
    );
}

}
```

The key rule: **never let widgets store `HDC`**. Let `HDC` be a transient paint adapter over `Canvas`.

---

# 11. Add first-class native handle access to `Widget`

Right now `Window` exposes native SDL/GL handles, but `Widget` does not expose the SWELL handle. ([GitHub][2]) Add a small optional field to `Widget` only for bridge ownership.

In `include/swole/window/widget.hpp`:

```cpp
public:
    void* native_handle() const { return native_handle_; }
    void set_native_handle(void* h) { native_handle_ = h; }

private:
    void* native_handle_{};
```

When creating `HwndWidget`:

```cpp
child->set_native_handle(hwnd);
```

For root widgets:

```cpp
hwnd->window->root().set_native_handle(hwnd);
```

This is useful for debugging, hit-testing, and reverse lookup.

---

# 12. The minimal integration order

Do this in this order:

1. Add `swell_handles.hpp`.
2. Add `HwndWidget`.
3. Implement `CreateWindowEx`, `DestroyWindow`, `ShowWindow`, `IsWindow`.
4. Make `Widget::invalidate()` call `window()->repaint_now()`.
5. Implement `SendMessage`, `PostMessage`, `DefWindowProc`.
6. Implement `Get/SetWindowLong`.
7. Implement geometry APIs.
8. Implement text APIs.
9. Implement `HMENU` enough to route `WM_COMMAND`.
10. Implement `HDC` as a temporary `Canvas` adapter.

That gets old WDL-style code talking to your modern widget tree without polluting your C++ API with Win32 types.

---

## Recommended final shape

```text
src/swell/
  swell_abi.hpp          existing C ABI types
  swell_dispatch.cpp     existing SWELLAPI_GetFunc table
  swell_core.cpp         misc platform functions
  swell_handles.hpp      new opaque handle backing structs
  swell_widget_host.hpp  new Widget -> HWND bridge
  swell_widget_host.cpp
  swell_window.cpp       HWND/window/control APIs
  swell_messages.cpp     SendMessage/PostMessage/DefWindowProc
  swell_menu.cpp         HMENU model + WM_COMMAND routing
  swell_gdi.cpp          HDC/brush/pen/font drawing adapters
```

The important design choice is this:

```cpp
HWND  -> HWND__ shell -> swole::Window or swole::Widget
HMENU -> HMENU__ shell -> menu model
HDC   -> HDC__ shell -> temporary swole::Canvas adapter
```

That gives you SWELL compatibility while keeping `swole::Widget`, `swole::Window`, and `swole::Canvas` clean.

[1]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/include/swole/window/widget.hpp "raw.githubusercontent.com"
[2]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/include/swole/window/window.hpp "raw.githubusercontent.com"
[3]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/src/swell/swell_abi.hpp "raw.githubusercontent.com"
[4]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/src/swell/swell_dispatch.cpp "raw.githubusercontent.com"
[5]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/src/window/widget.cpp "raw.githubusercontent.com"
[6]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/src/swell/swell_core.cpp "raw.githubusercontent.com"
[7]: https://raw.githubusercontent.com/gurrgur/swole/claude/ui-framework-skeleton-UsgPY/include/swole/render/canvas.hpp "raw.githubusercontent.com"
