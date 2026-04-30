#pragma once

#include "types.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace swole {

class Widget;

// ── Key codes ────────────────────────────────────────────────────────────────

enum class Key : uint32_t {
    Unknown = 0,
    Backspace = 8, Tab = 9, Return = 13, Escape = 27,
    Space = 32,
    Left = 37, Up = 38, Right = 39, Down = 40,
    Delete = 46,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Home = 36, End = 35, PageUp = 33, PageDown = 34,
    Insert = 45,
    LShift = 160, RShift = 161, LCtrl = 162, RCtrl = 163,
    LAlt = 164, RAlt = 165, LMeta = 91, RMeta = 92,
};

struct KeyMods {
    bool shift   : 1 {false};
    bool ctrl    : 1 {false};
    bool alt     : 1 {false};
    bool meta    : 1 {false};
    bool caps    : 1 {false};
    uint8_t _pad : 3 {0};

    constexpr bool operator==(const KeyMods&) const = default;
};

// ── Mouse ────────────────────────────────────────────────────────────────────

enum class MouseButton : uint8_t { None = 0, Left = 1, Middle = 2, Right = 3, X1 = 4, X2 = 5 };

struct MouseEvent {
    PointI  pos;
    PointI  global_pos;
    PointF  delta;       // used by move events
    PointF  wheel;       // used by scroll events
    MouseButton button{MouseButton::None};
    uint8_t click_count{1};
    KeyMods mods;
};

// ── Keyboard ─────────────────────────────────────────────────────────────────

struct KeyEvent {
    Key     key{Key::Unknown};
    uint32_t scancode{0};
    KeyMods  mods;
    bool     is_repeat{false};
};

struct TextInputEvent {
    std::string text; // UTF-8 string from IME or direct key
};

// ── Window / focus ───────────────────────────────────────────────────────────

struct ResizeEvent {
    SizeI old_size;
    SizeI new_size;
};

struct MoveEvent {
    PointI old_pos;
    PointI new_pos;
};

struct FocusEvent {
    bool gained; // true = gained, false = lost
    Widget* prev_or_next{nullptr};
};

struct CloseEvent {
    bool& accepted; // set to false to cancel close
};

struct PaintEvent {
    RectI dirty; // region to repaint (may be larger than actual clip)
};

struct DropEvent {
    PointI  pos;
    // TODO: mime data
};

struct TimerEvent {
    uint32_t id;
};

// ── Event variant ────────────────────────────────────────────────────────────

using Event = std::variant<
    MouseEvent,
    KeyEvent,
    TextInputEvent,
    ResizeEvent,
    MoveEvent,
    FocusEvent,
    CloseEvent,
    PaintEvent,
    DropEvent,
    TimerEvent
>;

// ── Typed signal / slot ──────────────────────────────────────────────────────

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    [[nodiscard]] uint32_t connect(Slot slot) {
        uint32_t id = next_id_++;
        slots_.push_back({id, std::move(slot)});
        return id;
    }

    void disconnect(uint32_t id) {
        std::erase_if(slots_, [id](const auto& s) { return s.first == id; });
    }

    void emit(Args... args) const {
        for (auto& [id, slot] : slots_)
            slot(args...);
    }

    void operator()(Args... args) const { emit(std::forward<Args>(args)...); }

private:
    std::vector<std::pair<uint32_t, Slot>> slots_;
    uint32_t next_id_{1};
};

} // namespace swole
