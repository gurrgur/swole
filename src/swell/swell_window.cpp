#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include "swole/window/window.hpp"
#include "swole/window/widget.hpp"
#include "swole/core/application.hpp"

#include <algorithm>
#include <cstring>
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

    swell_send_message(reinterpret_cast<HWND>(hwnd), 0x0001, 0, reinterpret_cast<LPARAM>(param));

    return reinterpret_cast<HWND>(hwnd);
}

BOOL DestroyWindow(HWND hwnd) {
    using namespace swole::swell;

    if (!as_hwnd(hwnd))
        return SWELL_FALSE;

    swell_send_message(hwnd, 0x0002, 0, 0);
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

BOOL GetWindowContentViewRect(HWND hwnd, RECT* r) {
    return GetWindowRect(hwnd, r);
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

BOOL MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint) {
    BOOL ok = SetWindowPos(hwnd, nullptr, x, y, w, h, 0);
    if (ok && repaint)
        InvalidateRect(hwnd, nullptr, SWELL_TRUE);
    return ok;
}

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

    int n = std::min<int>(maxlen - 1, int(h->text.size()));
    std::memcpy(buf, h->text.data(), size_t(n));
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

LONG_PTR GetWindowLongPtr(HWND hwnd, int index) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return 0;

    switch (index) {
        case -21: return h->longs.user_data;
        case -12: return h->longs.id;
        case -4:  return h->longs.wndproc;
        case -16: return h->longs.style;
        case -20: return h->longs.exstyle;
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

BOOL SetParent(HWND hwnd, HWND new_parent) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    if (h->parent)
        swole::swell::unlink_child(h->parent, hwnd);

    h->parent = new_parent;

    if (new_parent) {
        auto* np = swole::swell::as_hwnd(new_parent);
        if (np)
            np->children.push_back(hwnd);
    }

    return SWELL_TRUE;
}

HWND GetWindow(HWND hwnd, UINT cmd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return nullptr;

    if (cmd == 4)
        return h->parent;

    if (cmd == 5 && !h->children.empty())
        return h->children.front();

    if (cmd == 2 && !h->children.empty())
        return h->children.back();

    return nullptr;
}

HWND GetParent(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h ? h->parent : nullptr;
}

HWND GetTopWindow(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    return h && !h->children.empty() ? h->children.front() : nullptr;
}

HWND GetNextWindow(HWND hwnd, UINT cmd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !h->parent) return nullptr;

    auto* p = swole::swell::as_hwnd(h->parent);
    if (!p) return nullptr;

    auto it = std::find(p->children.begin(), p->children.end(), hwnd);
    if (it == p->children.end()) return nullptr;

    if (cmd == 2) {
        auto next = std::next(it);
        return next != p->children.end() ? *next : nullptr;
    }

    if (cmd == 1) {
        if (it == p->children.begin()) return nullptr;
        return *std::prev(it);
    }

    return nullptr;
}

BOOL ClientToScreen(HWND hwnd, POINT* p) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !p) return SWELL_FALSE;

    p->x += h->bounds.left;
    p->y += h->bounds.top;
    return SWELL_TRUE;
}

BOOL ScreenToClient(HWND hwnd, POINT* p) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !p) return SWELL_FALSE;

    p->x -= h->bounds.left;
    p->y -= h->bounds.top;
    return SWELL_TRUE;
}

HWND SetFocus(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return nullptr;

    auto* win = swole::swell::window_from_hwnd(hwnd);
    if (!win) return nullptr;

    auto* prev = win->focused_widget();
    win->set_focused_widget(h->widget);
    return prev ? reinterpret_cast<HWND>(prev->native_handle()) : nullptr;
}

HWND GetFocus() {
    for (auto* w : swole::Application::instance().windows()) {
        auto* fw = w->focused_widget();
        if (fw) return reinterpret_cast<HWND>(fw->native_handle());
    }
    return nullptr;
}

BOOL SetForegroundWindow(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    if (h->window) {
        h->window->raise();
        return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

HWND GetForegroundWindow() {
    return nullptr;
}

HWND SetCapture(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return nullptr;

    auto* win = swole::swell::window_from_hwnd(hwnd);
    if (!win) return nullptr;

    auto* prev = win->mouse_capture();
    win->capture_mouse(h->widget);
    return prev ? reinterpret_cast<HWND>(prev->native_handle()) : nullptr;
}

HWND GetCapture() {
    for (auto* w : swole::Application::instance().windows()) {
        auto* mc = w->mouse_capture();
        if (mc) return reinterpret_cast<HWND>(mc->native_handle());
    }
    return nullptr;
}

BOOL ReleaseCapture() {
    for (auto* w : swole::Application::instance().windows()) {
        if (w->mouse_capture()) {
            w->release_capture();
            return SWELL_TRUE;
        }
    }
    return SWELL_FALSE;
}

BOOL EnumChildWindows(HWND parent, BOOL (*callback)(HWND, LPARAM), LPARAM lp) {
    auto* p = swole::swell::as_hwnd(parent);
    if (!p || !callback) return SWELL_FALSE;

    for (HWND child : p->children) {
        if (!callback(child, lp))
            return SWELL_FALSE;
    }

    return SWELL_TRUE;
}

BOOL EnumWindows(BOOL (*callback)(HWND, LPARAM), LPARAM lp) {
    (void)callback;
    (void)lp;
    return SWELL_FALSE;
}

HWND FindWindowEx(HWND parent, HWND child_after, const char* class_name, const char* window_name) {
    auto* p = swole::swell::as_hwnd(parent);
    if (!p) return nullptr;

    bool found_start = !child_after;

    for (HWND child : p->children) {
        if (!found_start) {
            if (child == child_after)
                found_start = true;
            continue;
        }

        auto* c = swole::swell::as_hwnd(child);
        if (!c) continue;

        bool class_match = !class_name || !*class_name || c->class_name == class_name;
        bool name_match = !window_name || !*window_name || c->text == window_name;

        if (class_match && name_match)
            return child;
    }

    return nullptr;
}

HWND WindowFromPoint(POINT p) {
    (void)p;
    return nullptr;
}

BOOL ScrollWindow(HWND hwnd, int dx, int dy, const RECT* prc_scroll, const RECT* prc_clip) {
    (void)hwnd;
    (void)dx;
    (void)dy;
    (void)prc_scroll;
    (void)prc_clip;
    return SWELL_FALSE;
}

HANDLE GetProp(HWND hwnd, const char* name) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !name) return nullptr;

    auto it = h->props.find(name);
    return it != h->props.end() ? it->second : nullptr;
}

BOOL SetProp(HWND hwnd, const char* name, HANDLE value) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !name) return SWELL_FALSE;

    h->props[name] = value;
    return SWELL_TRUE;
}

HANDLE RemoveProp(HWND hwnd, const char* name) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !name) return nullptr;

    auto it = h->props.find(name);
    if (it == h->props.end()) return nullptr;

    HANDLE value = it->second;
    h->props.erase(it);
    return value;
}

BOOL EnumPropsEx(HWND hwnd, BOOL (*callback)(HWND, const char*, HANDLE, LONG_PTR), LONG_PTR lp) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h || !callback) return SWELL_FALSE;

    for (const auto& [name, value] : h->props) {
        if (!callback(hwnd, name.c_str(), value, lp))
            return SWELL_FALSE;
    }

    return SWELL_TRUE;
}

HWND SetActiveWindow(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return nullptr;

    if (h->window) {
        h->window->raise();
        return hwnd;
    }

    return nullptr;
}

HWND GetActiveWindow() {
    return nullptr;
}

BOOL BringWindowToTop(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    if (h->window) {
        h->window->raise();
        return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

BOOL SetWindowRgn(HWND hwnd, HRGN, BOOL) {
    (void)hwnd;
    return SWELL_FALSE;
}

int GetWindowRgn(HWND hwnd, HRGN) {
    (void)hwnd;
    return 0;
}

BOOL IsIconic(HWND hwnd) {
    (void)hwnd;
    return SWELL_FALSE;
}

BOOL IsZoomed(HWND hwnd) {
    (void)hwnd;
    return SWELL_FALSE;
}

void SetWindowTextA(HWND hwnd, const char* text) {
    SetWindowText(hwnd, text);
}

int GetWindowTextA(HWND hwnd, char* buf, int maxlen) {
    return GetWindowText(hwnd, buf, maxlen);
}

int GetWindowTextLengthA(HWND hwnd) {
    return GetWindowTextLength(hwnd);
}

}
