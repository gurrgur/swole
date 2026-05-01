#include "swole/core/application.hpp"
#include "swole/window/window.hpp"

#include <SDL3/SDL.h>
#include <cassert>
#include <mutex>
#include <queue>
#include <vector>
#include <algorithm>

namespace swole {

Application* Application::s_instance = nullptr;

struct Application::Impl {
    std::string  app_name;
    bool         running{false};
    int          exit_code{0};
    uint32_t     wakeup_event{SDL_EVENT_USER}; // registered user event type

    // Cross-thread dispatch queue
    std::mutex                          queue_mutex;
    std::queue<std::function<void()>>   queue;

    // Delayed callbacks: {fire_at_tick, id, fn}
    uint32_t next_timer_id{1};
    struct Delayed { uint64_t fire_at; uint32_t id; std::function<void()> fn; };
    std::vector<Delayed> delayed;

    // Window registry — raw non-owning pointers; windows own themselves
    std::vector<Window*> windows;

    uint64_t next_delayed_fire() const {
        uint64_t t = UINT64_MAX;
        for (auto& d : delayed) t = std::min(t, d.fire_at);
        return t;
    }

    void flush_queue() {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard lock{queue_mutex};
            std::swap(local, queue);
        }
        while (!local.empty()) {
            local.front()();
            local.pop();
        }
    }

    void fire_due_delayed() {
        uint64_t now = SDL_GetTicks();
        // Copy to avoid re-entrancy issues if a callback schedules another
        std::vector<Delayed> due;
        for (auto it = delayed.begin(); it != delayed.end(); ) {
            if (now >= it->fire_at) { due.push_back(std::move(*it)); it = delayed.erase(it); }
            else ++it;
        }
        for (auto& d : due) d.fn();
    }

    void register_window(Window* w)   { windows.push_back(w); }
    void unregister_window(Window* w) { std::erase(windows, w); }
};

Application::Application(int /*argc*/, char** argv)
    : impl_{std::make_unique<Impl>()}
{
    assert(!s_instance && "Only one Application per process");
    s_instance = this;

    impl_->app_name = argv ? argv[0] : "swole";

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        SDL_Log("SDL_Init failed: %s", SDL_GetError());

    // Reserve one user-event type for cross-thread wakeups.
    uint32_t base = SDL_RegisterEvents(1);
    if (base != (uint32_t)-1)
        impl_->wakeup_event = base;
}

Application::~Application() {
    SDL_Quit();
    s_instance = nullptr;
}

Application& Application::instance() {
    assert(s_instance);
    return *s_instance;
}

int Application::run() {
    impl_->running   = true;
    impl_->exit_code = 0;

    SDL_Event ev;
    while (impl_->running) {
        impl_->flush_queue();
        impl_->fire_due_delayed();

        // Compute how long until the next delayed callback so we can
        // sleep exactly that long instead of busy-polling.
        uint64_t now   = SDL_GetTicks();
        uint64_t next  = impl_->next_delayed_fire();
        int      wait_ms = (next == UINT64_MAX) ? 16
                         : int(std::max(uint64_t(0), next - now));
        wait_ms = std::clamp(wait_ms, 0, 16); // cap at one frame

        // Wait for an event or timeout, then drain all pending events.
        if (SDL_WaitEventTimeout(&ev, wait_ms)) {
            do {
                if (ev.type == SDL_EVENT_QUIT) { quit(); break; }
                if (ev.type == impl_->wakeup_event) continue; // just a wakeup ping

                for (Window* w : impl_->windows)
                    w->process_sdl_event(&ev);

            } while (SDL_PollEvent(&ev));
        }

        // Repaint all visible windows every frame so tooltip timers fire,
        // animations run, and invalidated widgets are always flushed.
        for (Window* w : impl_->windows)
            if (w->is_visible()) w->repaint_now();

        // Quit when no windows remain.
        if (impl_->windows.empty()) quit();
    }

    return impl_->exit_code;
}

void Application::quit(int exit_code) {
    impl_->running   = false;
    impl_->exit_code = exit_code;
}

bool Application::is_running() const { return impl_->running; }

void Application::register_window(Window* w)   { impl_->register_window(w); }
void Application::unregister_window(Window* w) { impl_->unregister_window(w); }

const std::vector<Window*>& Application::windows() const { return impl_->windows; }

void Application::post(std::function<void()> fn) {
    {
        std::lock_guard lock{impl_->queue_mutex};
        impl_->queue.push(std::move(fn));
    }
    SDL_Event ev{};
    ev.type = impl_->wakeup_event;
    SDL_PushEvent(&ev);
}

uint32_t Application::post_delayed(uint32_t ms, std::function<void()> fn) {
    uint32_t id = impl_->next_timer_id++;
    impl_->delayed.push_back({SDL_GetTicks() + ms, id, std::move(fn)});
    return id;
}

void Application::cancel_delayed(uint32_t id) {
    std::erase_if(impl_->delayed,
                  [id](const Impl::Delayed& d) { return d.id == id; });
}

std::string Application::clipboard_text() const {
    const char* t = SDL_GetClipboardText();
    return t ? std::string{t} : std::string{};
}

void Application::set_clipboard_text(std::string_view text) {
    SDL_SetClipboardText(std::string{text}.c_str());
}

std::string_view Application::app_name() const { return impl_->app_name; }

} // namespace swole
