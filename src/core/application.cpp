#include "swole/core/application.hpp"
#include "swole/window/window.hpp"

#include <SDL3/SDL.h>
#include <cassert>
#include <chrono>
#include <mutex>
#include <queue>
#include <vector>

namespace swole {

Application* Application::s_instance = nullptr;

struct Application::Impl {
    std::string app_name;
    bool        running{false};
    int         exit_code{0};

    // Cross-thread dispatch queue
    std::mutex              queue_mutex;
    std::queue<std::function<void()>> queue;

    // Delayed callbacks: {fire_at_tick, id, fn}
    uint32_t next_timer_id{1};
    struct Delayed { uint64_t fire_at; uint32_t id; std::function<void()> fn; };
    std::vector<Delayed> delayed;

    // Window registry — raw non-owning pointers; windows own themselves
    std::vector<Window*> windows;

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
        for (auto it = delayed.begin(); it != delayed.end(); ) {
            if (now >= it->fire_at) {
                it->fn();
                it = delayed.erase(it);
            } else {
                ++it;
            }
        }
    }
};

Application::Application(int /*argc*/, char** argv)
    : impl_{std::make_unique<Impl>()}
{
    assert(!s_instance && "Only one Application per process");
    s_instance = this;

    impl_->app_name = argv ? argv[0] : "swole";

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        // TODO: throw or log properly
    }
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
    impl_->running = true;
    impl_->exit_code = 0;

    SDL_Event ev;
    while (impl_->running) {
        // Drain pending cross-thread callbacks
        impl_->flush_queue();
        impl_->fire_due_delayed();

        // Poll SDL events and forward to the relevant window
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                quit();
                break;
            }

            // Route to the correct window
            for (Window* w : impl_->windows) {
                w->process_sdl_event(&ev);
            }
        }

        // If all windows closed, exit naturally
        bool any_visible = false;
        for (Window* w : impl_->windows)
            if (w->is_visible()) { any_visible = true; break; }

        if (!any_visible && impl_->windows.empty())
            quit();

        SDL_Delay(1); // yield; replace with frame-paced loop when rendering
    }

    return impl_->exit_code;
}

void Application::quit(int exit_code) {
    impl_->running  = false;
    impl_->exit_code = exit_code;
}

bool Application::is_running() const { return impl_->running; }

void Application::post(std::function<void()> fn) {
    std::lock_guard lock{impl_->queue_mutex};
    impl_->queue.push(std::move(fn));
    SDL_PushEvent(nullptr); // wake the event loop
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
    const char* text = SDL_GetClipboardText();
    return text ? std::string{text} : std::string{};
}

void Application::set_clipboard_text(std::string_view text) {
    SDL_SetClipboardText(std::string{text}.c_str());
}

std::string_view Application::app_name() const { return impl_->app_name; }

} // namespace swole
