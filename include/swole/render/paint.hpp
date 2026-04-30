#pragma once

#include "../core/types.hpp"
#include <memory>

namespace swole {

enum class FillRule { Winding, EvenOdd };
enum class StrokeCap { Butt, Round, Square };
enum class StrokeJoin { Miter, Round, Bevel };
enum class BlendMode {
    Clear, Src, Dst, SrcOver, DstOver, SrcIn, DstIn, SrcOut, DstOut,
    SrcATop, DstATop, Xor, Plus, Modulate, Screen,
    Overlay, Darken, Lighten, ColorDodge, ColorBurn, HardLight, SoftLight,
    Difference, Exclusion, Multiply, Hue, Saturation, Color, Luminosity,
};

// Encapsulates fill and stroke style for Canvas draw calls.
// Wraps SkPaint internally (pimpl).
class Paint {
public:
    Paint();
    explicit Paint(Color fill_color);
    ~Paint();

    Paint(const Paint&);
    Paint& operator=(const Paint&);
    Paint(Paint&&) noexcept;
    Paint& operator=(Paint&&) noexcept;

    // Convenience factories
    static Paint fill(Color c);
    static Paint stroke(Color c, float width = 1.f);
    static Paint fill_and_stroke(Color fill, Color stroke_color, float stroke_width = 1.f);

    [[nodiscard]] bool is_fill()   const;
    [[nodiscard]] bool is_stroke() const;

    Paint& set_fill_color(Color c);
    Paint& set_stroke_color(Color c);
    Paint& set_stroke_width(float w);
    Paint& set_stroke_cap(StrokeCap cap);
    Paint& set_stroke_join(StrokeJoin join);
    Paint& set_miter_limit(float limit);
    Paint& set_anti_alias(bool aa);
    Paint& set_alpha(float a);   // 0-1 multiplier applied on top of color alpha
    Paint& set_blend_mode(BlendMode mode);
    Paint& set_fill_rule(FillRule rule);

    [[nodiscard]] Color      fill_color()    const;
    [[nodiscard]] Color      stroke_color()  const;
    [[nodiscard]] float      stroke_width()  const;
    [[nodiscard]] StrokeCap  stroke_cap()    const;
    [[nodiscard]] StrokeJoin stroke_join()   const;
    [[nodiscard]] float      miter_limit()   const;
    [[nodiscard]] bool       anti_alias()    const;
    [[nodiscard]] BlendMode  blend_mode()    const;
    [[nodiscard]] FillRule   fill_rule()     const;

    // Internal: opaque handle for the renderer to cast
    [[nodiscard]] void* native_handle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace swole
