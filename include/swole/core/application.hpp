#pragma once

#include <functional>
#include <memory>
#include <string>

namespace swole {

class Window;

// Main application object. Create exactly one per process before doing
// anything else. Owns the SDL event loop and the Skia GPU context.
class Application {
public:
    explicit Application(int argc, char** argv);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Returns the process-wide instance (asserts it exists).
    static Application& instance();

    // Block until quit() is called or all windows are closed.
    int run();

    // Request the event loop to exit with the given code.
    void quit(int exit_code = 0);

    // Whether the event loop is currently running.
    [[nodiscard]] bool is_running() const;

    // Invoke callback on the main thread from any thread.
    void post(std::function<void()> fn);

    // Invoke callback after at least `ms` milliseconds, on the main thread.
    uint32_t post_delayed(uint32_t ms, std::function<void()> fn);
    void cancel_delayed(uint32_t id);

    // Clipboard
    [[nodiscard]] std::string clipboard_text() const;
    void set_clipboard_text(std::string_view text);

    [[nodiscard]] std::string_view app_name() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static Application* s_instance;
};

} // namespace swole
