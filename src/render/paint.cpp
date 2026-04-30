#include "swole/render/paint.hpp"

#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"

namespace swole {

namespace {
inline SkColor to_sk(Color c)   { return SkColorSetARGB(c.a, c.r, c.g, c.b); }
inline Color   from_sk(SkColor c) {
    return {SkColorGetR(c), SkColorGetG(c), SkColorGetB(c), SkColorGetA(c)};
}
} // namespace

struct Paint::Impl {
    SkPaint fill;
    SkPaint stroke;
    bool    has_fill{true};
    bool    has_stroke{false};
};

Paint::Paint()  : impl_{std::make_unique<Impl>()} {
    impl_->fill.setStyle(SkPaint::kFill_Style);
    impl_->fill.setAntiAlias(true);
    impl_->stroke.setStyle(SkPaint::kStroke_Style);
    impl_->stroke.setAntiAlias(true);
    impl_->stroke.setStrokeWidth(1.f);
}

Paint::Paint(Color fill_color) : Paint() {
    impl_->fill.setColor(to_sk(fill_color));
}

Paint::~Paint() = default;
Paint::Paint(const Paint& o)            : impl_{std::make_unique<Impl>(*o.impl_)} {}
Paint& Paint::operator=(const Paint& o) { *impl_ = *o.impl_; return *this; }
Paint::Paint(Paint&&) noexcept            = default;
Paint& Paint::operator=(Paint&&) noexcept = default;

Paint Paint::fill(Color c)       { Paint p(c); return p; }
Paint Paint::stroke(Color c, float w) {
    Paint p; p.impl_->has_fill = false; p.impl_->has_stroke = true;
    p.impl_->stroke.setColor(to_sk(c)); p.impl_->stroke.setStrokeWidth(w);
    return p;
}
Paint Paint::fill_and_stroke(Color fc, Color sc, float sw) {
    Paint p(fc); p.impl_->has_stroke = true;
    p.impl_->stroke.setColor(to_sk(sc)); p.impl_->stroke.setStrokeWidth(sw);
    return p;
}

bool Paint::is_fill()   const { return impl_->has_fill; }
bool Paint::is_stroke() const { return impl_->has_stroke; }

Paint& Paint::set_fill_color(Color c)  { impl_->fill.setColor(to_sk(c));   return *this; }
Paint& Paint::set_stroke_color(Color c){ impl_->stroke.setColor(to_sk(c)); return *this; }
Paint& Paint::set_stroke_width(float w){ impl_->stroke.setStrokeWidth(w);  return *this; }
Paint& Paint::set_anti_alias(bool aa)  {
    impl_->fill.setAntiAlias(aa); impl_->stroke.setAntiAlias(aa); return *this;
}
Paint& Paint::set_alpha(float a) {
    impl_->fill.setAlphaf(a); impl_->stroke.setAlphaf(a); return *this;
}

namespace {
SkPaint::Cap  to_sk_cap(StrokeCap c)  {
    switch (c) {
    case StrokeCap::Round:  return SkPaint::kRound_Cap;
    case StrokeCap::Square: return SkPaint::kSquare_Cap;
    default:                return SkPaint::kButt_Cap;
    }
}
SkPaint::Join to_sk_join(StrokeJoin j) {
    switch (j) {
    case StrokeJoin::Round: return SkPaint::kRound_Join;
    case StrokeJoin::Bevel: return SkPaint::kBevel_Join;
    default:                return SkPaint::kMiter_Join;
    }
}
SkBlendMode to_sk_blend(BlendMode m) {
    return static_cast<SkBlendMode>(m); // enum values intentionally match
}
} // namespace

Paint& Paint::set_stroke_cap(StrokeCap c)  { impl_->stroke.setStrokeCap(to_sk_cap(c));   return *this; }
Paint& Paint::set_stroke_join(StrokeJoin j){ impl_->stroke.setStrokeJoin(to_sk_join(j));  return *this; }
Paint& Paint::set_miter_limit(float l)     { impl_->stroke.setStrokeMiter(l);             return *this; }
Paint& Paint::set_blend_mode(BlendMode m)  {
    impl_->fill.setBlendMode(to_sk_blend(m));
    impl_->stroke.setBlendMode(to_sk_blend(m));
    return *this;
}
Paint& Paint::set_fill_rule(FillRule r) {
    (void)r; // applied per draw-call via SkPath fill type
    return *this;
}

Color      Paint::fill_color()   const { return from_sk(impl_->fill.getColor()); }
Color      Paint::stroke_color() const { return from_sk(impl_->stroke.getColor()); }
float      Paint::stroke_width() const { return impl_->stroke.getStrokeWidth(); }
bool       Paint::anti_alias()   const { return impl_->fill.isAntiAlias(); }

// The canvas wrapper calls native_handle() twice — once for fill, once for
// stroke — so we expose the *fill* paint as the primary handle.  The canvas
// handles the dual-paint draw pattern via the has_fill / has_stroke booleans.
void* Paint::native_handle() const { return &impl_->fill; }

} // namespace swole
