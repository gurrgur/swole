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

HWND GetDlgItem(HWND parent, int id);

LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return swole::swell::swell_def_window_proc(hwnd, msg, wp, lp);
}

LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return swole::swell::swell_send_message(hwnd, msg, wp, lp);
}

LRESULT SendMessageTimeoutA(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT, UINT, DWORD*) {
    return SendMessage(hwnd, msg, wp, lp);
}

LRESULT SendDlgItemMessageA(HWND parent, int id, UINT msg, WPARAM wp, LPARAM lp) {
    HWND child = GetDlgItem(parent, id);
    return SendMessage(child, msg, wp, lp);
}

LRESULT SendDlgItemMessage(HWND parent, int id, UINT msg, WPARAM wp, LPARAM lp) {
    return SendDlgItemMessageA(parent, id, msg, wp, lp);
}

BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (!swole::swell::as_hwnd(hwnd))
        return SWELL_FALSE;

    std::lock_guard lock{g_queue_mutex};
    g_queue.push_back({hwnd, msg, wp, lp});
    return SWELL_TRUE;
}

void SWELL_BroadcastMessage(UINT msg, WPARAM wp, LPARAM lp) {
    std::lock_guard lock{g_queue_mutex};
    for (auto& m : g_queue) {
        if (m.msg == msg && m.wp == wp && m.lp == lp)
            return;
    }
    g_queue.push_back({nullptr, msg, wp, lp});
}

void SWELL_MessageQueue_Flush(HWND hwnd) {
    std::deque<QueuedMessage> local;

    {
        std::lock_guard lock{g_queue_mutex};

        for (auto it = g_queue.begin(); it != g_queue.end();) {
            if (!hwnd || it->hwnd == hwnd || !it->hwnd) {
                local.push_back(*it);
                it = g_queue.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (const auto& m : local) {
        if (m.hwnd)
            swole::swell::swell_send_message(m.hwnd, m.msg, m.wp, m.lp);
    }
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

void SWELL_Internal_PostMessage_Init() {
}

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, TIMERPROC proc) {
    (void)hwnd;
    (void)id;
    (void)ms;
    (void)proc;
    return 0;
}

BOOL KillTimer(HWND hwnd, UINT_PTR id) {
    (void)hwnd;
    (void)id;
    return SWELL_FALSE;
}

}
