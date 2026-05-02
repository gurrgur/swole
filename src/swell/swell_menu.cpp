#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include <algorithm>

extern "C" {

HMENU CreatePopupMenu() {
    return reinterpret_cast<HMENU>(new HMENU__{});
}

HMENU CreatePopupMenuEx(const char*) {
    return CreatePopupMenu();
}

HMENU CreateMenu() {
    return CreatePopupMenu();
}

BOOL DestroyMenu(HMENU menu) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    m->magic = 0;
    delete m;
    return SWELL_TRUE;
}

HMENU GetSubMenu(HMENU menu, int pos) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m || pos < 0 || pos >= int(m->items.size()))
        return nullptr;

    return m->items[size_t(pos)].submenu ? reinterpret_cast<HMENU>(m->items[size_t(pos)].submenu.get()) : nullptr;
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

BOOL SetMenuItemModifier(HMENU menu, UINT id, UINT, UINT) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    for (auto& item : m->items) {
        if (item.id == id)
            return SWELL_TRUE;
    }
    return SWELL_FALSE;
}

BOOL SetMenuItemText(HMENU menu, UINT id, const char* text) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    for (auto& item : m->items) {
        if (item.id == id) {
            item.text = text ? text : "";
            return SWELL_TRUE;
        }
    }
    return SWELL_FALSE;
}

BOOL EnableMenuItem(HMENU menu, UINT id, UINT flags) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

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

BOOL DeleteMenu(HMENU menu, UINT pos, UINT flags) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    constexpr UINT MF_BYPOSITION = 0x0400;

    if (flags & MF_BYPOSITION) {
        if (pos >= m->items.size())
            return SWELL_FALSE;
        m->items.erase(m->items.begin() + pos);
        return SWELL_TRUE;
    }

    for (auto it = m->items.begin(); it != m->items.end(); ++it) {
        if (it->id == pos) {
            m->items.erase(it);
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

BOOL InsertMenuItem(HMENU, UINT, BOOL, const void*) {
    return SWELL_FALSE;
}

BOOL SWELL_InsertMenu(HMENU menu, int pos, int flags, int id, const char* text) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    constexpr int MF_POPUP = 0x00000010;
    constexpr int MF_SEPARATOR = 0x00000800;
    constexpr int MF_BYPOSITION = 0x00000400;

    MenuItem item;

    if (flags & MF_SEPARATOR) {
        item.separator = true;
        item.text = "";
    } else if (flags & MF_POPUP) {
        item.submenu = std::unique_ptr<HMENU__>(reinterpret_cast<HMENU__*>(id));
        item.text = text ? text : "";
        item.id = 0;
    } else {
        item.id = UINT(id);
        item.text = text ? text : "";
    }

    int insert_pos = pos;
    if (!(flags & MF_BYPOSITION)) {
        insert_pos = -1;
        for (int i = 0; i < int(m->items.size()); ++i) {
            if (m->items[size_t(i)].id == UINT(pos)) {
                insert_pos = i;
                break;
            }
        }
    }

    if (insert_pos < 0 || insert_pos > int(m->items.size()))
        insert_pos = int(m->items.size());

    m->items.insert(m->items.begin() + insert_pos, std::move(item));
    return SWELL_TRUE;
}

BOOL AppendMenu(HMENU menu, UINT flags, UINT id, const char* text) {
    return SWELL_InsertMenu(menu, -1, flags | 0x0400, int(id), text);
}

BOOL GetMenuItemInfo(HMENU, UINT, BOOL, void*) {
    return SWELL_FALSE;
}

BOOL SetMenuItemInfo(HMENU, UINT, BOOL, const void*) {
    return SWELL_FALSE;
}

BOOL DrawMenuBar(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return SWELL_FALSE;

    if (h->widget)
        h->widget->invalidate_all();

    return SWELL_TRUE;
}

HMENU SWELL_LoadMenu(HINSTANCE, const char*) {
    return nullptr;
}

BOOL TrackPopupMenu(HMENU menu, UINT, int, int, int, HWND hwnd, const RECT*) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return SWELL_FALSE;

    HWND dest = m->destination ? m->destination : hwnd;

    for (const auto& item : m->items) {
        if (!item.separator && item.enabled && item.id != 0) {
            swole::swell::swell_send_message(
                dest,
                0x0111,
                WPARAM(item.id),
                0
            );
            return SWELL_TRUE;
        }
    }

    return SWELL_FALSE;
}

BOOL TrackPopupMenuEx(HMENU menu, UINT flags, int x, int y, HWND hwnd, void*) {
    return TrackPopupMenu(menu, flags, x, y, 0, hwnd, nullptr);
}

void SWELL_SetMenuDestination(HMENU menu, HWND hwnd) {
    auto* m = swole::swell::as_hmenu(menu);
    if (m) m->destination = hwnd;
}

HMENU SWELL_DuplicateMenu(HMENU menu) {
    auto* m = swole::swell::as_hmenu(menu);
    if (!m) return nullptr;

    auto* dup = new HMENU__{};
    dup->destination = m->destination;

    for (const auto& item : m->items) {
        MenuItem dup_item;
        dup_item.id = item.id;
        dup_item.text = item.text;
        dup_item.enabled = item.enabled;
        dup_item.checked = item.checked;
        dup_item.separator = item.separator;
        if (item.submenu)
            dup_item.submenu = std::unique_ptr<HMENU__>(
                reinterpret_cast<HMENU__*>(SWELL_DuplicateMenu(reinterpret_cast<HMENU>(item.submenu.get()))));
        dup->items.push_back(std::move(dup_item));
    }

    return reinterpret_cast<HMENU>(dup);
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

HMENU SWELL_GetDefaultWindowMenu(HWND) {
    return nullptr;
}

void SWELL_SetDefaultWindowMenu(HWND, HMENU) {
}

HMENU SWELL_GetDefaultModalWindowMenu() {
    return nullptr;
}

void SWELL_SetDefaultModalWindowMenu(HMENU) {
}

HMENU SWELL_GetCurrentMenu() {
    return nullptr;
}

void SWELL_SetCurrentMenu(HMENU) {
}

void SWELL_Menu_AddMenuItem(HMENU menu, const char* text, int id) {
    SWELL_InsertMenu(menu, -1, 0x0400, id, text);
}

BOOL SWELL_GenerateMenuFromList(HMENU menu, const void* list) {
    (void)menu;
    (void)list;
    return SWELL_FALSE;
}

}
