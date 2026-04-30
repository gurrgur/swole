#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace swole {

// ── Color ────────────────────────────────────────────────────────────────────

struct Color {
    uint8_t r{0}, g{0}, b{0}, a{255};

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r{r}, g{g}, b{b}, a{a} {}

    static constexpr Color from_rgba32(uint32_t v) {
        return {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
    }
    static constexpr Color from_argb32(uint32_t v) {
        return {uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v), uint8_t(v >> 24)};
    }

    [[nodiscard]] constexpr uint32_t to_rgba32() const {
        return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | a;
    }

    [[nodiscard]] constexpr Color with_alpha(uint8_t alpha) const {
        return {r, g, b, alpha};
    }

    [[nodiscard]] constexpr Color lerp(Color other, float t) const {
        auto lerp_u8 = [](uint8_t a, uint8_t b, float t) -> uint8_t {
            return uint8_t(float(a) + (float(b) - float(a)) * t);
        };
        return {lerp_u8(r, other.r, t), lerp_u8(g, other.g, t),
                lerp_u8(b, other.b, t), lerp_u8(a, other.a, t)};
    }

    constexpr bool operator==(const Color&) const = default;

    // Named constants
    static constexpr Color transparent() { return {0, 0, 0, 0}; }
    static constexpr Color black()       { return {0, 0, 0}; }
    static constexpr Color white()       { return {255, 255, 255}; }
    static constexpr Color red()         { return {255, 0, 0}; }
    static constexpr Color green()       { return {0, 255, 0}; }
    static constexpr Color blue()        { return {0, 0, 255}; }
    static constexpr Color gray()        { return {128, 128, 128}; }
    static constexpr Color dark_gray()   { return {64, 64, 64}; }
    static constexpr Color light_gray()  { return {192, 192, 192}; }
};

// ── Point ────────────────────────────────────────────────────────────────────

template <typename T>
struct Point {
    T x{}, y{};

    constexpr Point() = default;
    constexpr Point(T x, T y) : x{x}, y{y} {}

    template <typename U>
    explicit constexpr Point(Point<U> other) : x{T(other.x)}, y{T(other.y)} {}

    constexpr Point operator+(Point other) const { return {x + other.x, y + other.y}; }
    constexpr Point operator-(Point other) const { return {x - other.x, y - other.y}; }
    constexpr Point operator*(T s) const { return {x * s, y * s}; }
    constexpr Point operator/(T s) const { return {x / s, y / s}; }
    constexpr Point& operator+=(Point other) { x += other.x; y += other.y; return *this; }
    constexpr Point& operator-=(Point other) { x -= other.x; y -= other.y; return *this; }

    constexpr bool operator==(const Point&) const = default;
};

using PointI = Point<int>;
using PointF = Point<float>;

// ── Size ─────────────────────────────────────────────────────────────────────

template <typename T>
struct Size {
    T w{}, h{};

    constexpr Size() = default;
    constexpr Size(T w, T h) : w{w}, h{h} {}

    template <typename U>
    explicit constexpr Size(Size<U> other) : w{T(other.w)}, h{T(other.h)} {}

    [[nodiscard]] constexpr bool is_empty() const { return w <= 0 || h <= 0; }
    [[nodiscard]] constexpr T area() const { return w * h; }

    constexpr Size operator+(Size other) const { return {w + other.w, h + other.h}; }
    constexpr Size operator-(Size other) const { return {w - other.w, h - other.h}; }
    constexpr Size operator*(T s) const { return {w * s, h * s}; }
    constexpr Size operator/(T s) const { return {w / s, h / s}; }

    constexpr bool operator==(const Size&) const = default;
};

using SizeI = Size<int>;
using SizeF = Size<float>;

// ── Rect ─────────────────────────────────────────────────────────────────────

template <typename T>
struct Rect {
    T x{}, y{}, w{}, h{};

    constexpr Rect() = default;
    constexpr Rect(T x, T y, T w, T h) : x{x}, y{y}, w{w}, h{h} {}
    constexpr Rect(Point<T> origin, Size<T> size) : x{origin.x}, y{origin.y}, w{size.w}, h{size.h} {}

    static constexpr Rect from_ltrb(T l, T t, T r, T b) { return {l, t, r - l, b - t}; }

    template <typename U>
    explicit constexpr Rect(Rect<U> other)
        : x{T(other.x)}, y{T(other.y)}, w{T(other.w)}, h{T(other.h)} {}

    [[nodiscard]] constexpr T left()   const { return x; }
    [[nodiscard]] constexpr T top()    const { return y; }
    [[nodiscard]] constexpr T right()  const { return x + w; }
    [[nodiscard]] constexpr T bottom() const { return y + h; }

    [[nodiscard]] constexpr Point<T> origin() const { return {x, y}; }
    [[nodiscard]] constexpr Size<T>  size()   const { return {w, h}; }
    [[nodiscard]] constexpr Point<T> center() const { return {x + w / 2, y + h / 2}; }

    [[nodiscard]] constexpr bool is_empty() const { return w <= 0 || h <= 0; }

    [[nodiscard]] constexpr bool contains(Point<T> p) const {
        return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
    }
    [[nodiscard]] constexpr bool contains(Rect other) const {
        return other.x >= x && other.y >= y &&
               other.right() <= right() && other.bottom() <= bottom();
    }
    [[nodiscard]] constexpr bool intersects(Rect other) const {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    [[nodiscard]] constexpr Rect intersection(Rect other) const {
        T lx = std::max(x, other.x);
        T ly = std::max(y, other.y);
        T rx = std::min(right(), other.right());
        T ry = std::min(bottom(), other.bottom());
        if (rx <= lx || ry <= ly) return {};
        return {lx, ly, rx - lx, ry - ly};
    }

    [[nodiscard]] constexpr Rect united(Rect other) const {
        if (is_empty()) return other;
        if (other.is_empty()) return *this;
        T lx = std::min(x, other.x);
        T ly = std::min(y, other.y);
        T rx = std::max(right(), other.right());
        T ry = std::max(bottom(), other.bottom());
        return {lx, ly, rx - lx, ry - ly};
    }

    [[nodiscard]] constexpr Rect inset(T dx, T dy) const {
        return {x + dx, y + dy, w - dx * 2, h - dy * 2};
    }
    [[nodiscard]] constexpr Rect inset(T d) const { return inset(d, d); }

    [[nodiscard]] constexpr Rect translated(T dx, T dy) const { return {x + dx, y + dy, w, h}; }
    [[nodiscard]] constexpr Rect translated(Point<T> d) const { return translated(d.x, d.y); }

    constexpr bool operator==(const Rect&) const = default;
};

using RectI = Rect<int>;
using RectF = Rect<float>;

// ── Insets ───────────────────────────────────────────────────────────────────

template <typename T>
struct Insets {
    T top{}, right{}, bottom{}, left{};

    constexpr Insets() = default;
    explicit constexpr Insets(T all) : top{all}, right{all}, bottom{all}, left{all} {}
    constexpr Insets(T v, T h) : top{v}, right{h}, bottom{v}, left{h} {}
    constexpr Insets(T top, T right, T bottom, T left)
        : top{top}, right{right}, bottom{bottom}, left{left} {}

    [[nodiscard]] constexpr T h_total() const { return left + right; }
    [[nodiscard]] constexpr T v_total() const { return top + bottom; }

    constexpr bool operator==(const Insets&) const = default;
};

using InsetsI = Insets<int>;
using InsetsF = Insets<float>;

// ── Common UI enums ───────────────────────────────────────────────────────────

enum class Orientation { Horizontal, Vertical };
enum class SelectionMode { Single, Multi, None };

} // namespace swole
