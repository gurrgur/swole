#pragma once

#include "font.hpp"
#include "image.hpp"
#include "paint.hpp"
#include "../core/types.hpp"
#include <memory>
#include <span>
#include <string_view>

namespace swole {

// Represents a 2-D path for complex shapes.
class Path {
public:
    Path();
    ~Path();
    Path(const Path&);
    Path& operator=(const Path&);
    Path(Path&&) noexcept;
    Path& operator=(Path&&) noexcept;

    Path& move_to(float x, float y);
    Path& line_to(float x, float y);
    Path& quad_to(float cx, float cy, float x, float y);
    Path& cubic_to(float c1x, float c1y, float c2x, float c2y, float x, float y);
    Path& arc_to(float rx, float ry, float x_rotate, bool large, bool sweep, float x, float y);
    Path& close();
    Path& add_rect(RectF r, bool cw = true);
    Path& add_round_rect(RectF r, float rx, float ry);
    Path& add_oval(RectF r);
    Path& add_circle(float cx, float cy, float radius);

    Path& reset();

    [[nodiscard]] bool  is_empty() const;
    [[nodiscard]] RectF bounds()   const;

    void* native_handle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ── Canvas ───────────────────────────────────────────────────────────────────

// Thin wrapper around SkCanvas that provides a clean, modern C++ API.
// Canvas instances are only valid for the duration of a paint event —
// do not store them beyond their issuing scope.
class Canvas {
public:
    Canvas() = default;
    ~Canvas() = default;

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    // ── State stack ──

    void save();
    void restore();

    struct ScopedSave {
        explicit ScopedSave(Canvas& c) : canvas_(c) { c.save(); }
        ~ScopedSave() { canvas_.restore(); }
        ScopedSave(const ScopedSave&) = delete;
    private:
        Canvas& canvas_;
    };
    [[nodiscard]] ScopedSave scoped_save() { return ScopedSave{*this}; }

    // ── Transform ──

    void translate(float dx, float dy);
    void scale(float sx, float sy);
    void scale(float s) { scale(s, s); }
    void rotate(float degrees, float cx = 0, float cy = 0);
    void skew(float sx, float sy);
    void concat(const float m[9]); // 3x3 row-major

    void reset_transform();

    // ── Clip ──

    void clip_rect(RectF r, bool anti_alias = true);
    void clip_round_rect(RectF r, float rx, float ry, bool anti_alias = true);
    void clip_path(const Path& p, bool anti_alias = true);

    // ── Clear ──

    void clear(Color c);
    void clear() { clear(Color::transparent()); }

    // ── Draw primitives ──

    void draw_rect(RectF r, const Paint& p);
    void draw_round_rect(RectF r, float rx, float ry, const Paint& p);
    void draw_oval(RectF r, const Paint& p);
    void draw_circle(float cx, float cy, float radius, const Paint& p);
    void draw_line(float x0, float y0, float x1, float y1, const Paint& p);
    void draw_line(PointF a, PointF b, const Paint& p) { draw_line(a.x, a.y, b.x, b.y, p); }
    void draw_path(const Path& path, const Paint& p);
    void draw_points(std::span<const PointF> pts, const Paint& p);
    void draw_polygon(std::span<const PointF> pts, bool closed, const Paint& p);

    // ── Gradients ──
    // Fill a rectangle with a linear gradient between two endpoints.
    // colors and stops must be the same length; stops may be empty (evenly distributed).
    void fill_linear_gradient(RectF dst, PointF from, PointF to,
                              std::span<const Color> colors,
                              std::span<const float> stops = {});
    // Fill a rectangle with a radial gradient.
    void fill_radial_gradient(RectF dst, PointF center, float radius,
                              std::span<const Color> colors,
                              std::span<const float> stops = {});
    // Fill a rounded rect with a linear gradient.
    void fill_round_rect_linear_gradient(RectF dst, float rx, float ry,
                                         PointF from, PointF to,
                                         std::span<const Color> colors,
                                         std::span<const float> stops = {});

    // ── Shadows ──
    // Draw a soft drop shadow beneath a (possibly rounded) rectangle.
    // The shadow is drawn before the caller's content, so call this first.
    void draw_shadow(RectF r, float blur_radius,
                     float dx = 0.f, float dy = 2.f,
                     Color color = {0, 0, 0, 80},
                     float corner_radius = 0.f);

    // ── Images ──

    void draw_image(const Image& img, float x, float y, const Paint* p = nullptr);
    void draw_image(const Image& img, RectF dst, const Paint* p = nullptr);
    void draw_image(const Image& img, RectF src, RectF dst,
                    ScaleFilter filter = ScaleFilter::Linear,
                    const Paint* p = nullptr);
    // 9-patch / border-image
    void draw_image_nine(const Image& img, RectI center, RectF dst, const Paint* p = nullptr);

    // ── Text ──

    void draw_text(std::string_view text, float x, float y,
                   const Font& font, const Paint& p);
    void draw_text(std::string_view text, RectF bounds,
                   TextAlign align, TextBaseline baseline,
                   const Font& font, const Paint& p);

    // ── Internal ──

    // Bind to an SkCanvas (called by the rendering backend before issuing paint events).
    void bind(void* sk_canvas);
    [[nodiscard]] void* native_handle() const { return sk_canvas_; }

private:
    void* sk_canvas_{nullptr};
};

} // namespace swole
