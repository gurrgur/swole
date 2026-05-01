#include "swole/render/paint.hpp"

#include "include/core/SkPaint.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkColor.h"
#include "include/core/SkScalar.h"
#include "include/core/SkSpan.h"
#include "include/effects/SkDashPathEffect.h"

#include <vector>

namespace swole {

namespace {
inline SkColor to_sk(Color c)   { return SkColorSetARGB(c.a, c.r, c.g, c.b); }
inline Color   from_sk(SkColor c) {
    return {SkColorGetR(c), SkColorGetG(c), SkColorGetB(c), SkColorGetA(c)};
}
} // namespace

struct Paint::Impl {
    SkPaint  fill;
    SkPaint  stroke;
    bool     has_fill{true};
    bool     has_stroke{false};
    FillRule fill_rule{FillRule::Winding};
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
    impl_->fill_rule = r;
    return *this;
}

Paint& Paint::set_dash_pattern(std::span<const float> intervals, float offset) {
    if (intervals.empty()) {
        impl_->stroke.setPathEffect(nullptr);
    } else {
        impl_->stroke.setPathEffect(
            SkDashPathEffect::Make(
                SkSpan<const SkScalar>(intervals.data(), intervals.size()), offset));
    }
    return *this;
}

namespace {
StrokeCap  from_sk_cap(SkPaint::Cap c)  {
    switch (c) {
    case SkPaint::kRound_Cap:  return StrokeCap::Round;
    case SkPaint::kSquare_Cap: return StrokeCap::Square;
    default:                   return StrokeCap::Butt;
    }
}
StrokeJoin from_sk_join(SkPaint::Join j) {
    switch (j) {
    case SkPaint::kRound_Join: return StrokeJoin::Round;
    case SkPaint::kBevel_Join: return StrokeJoin::Bevel;
    default:                   return StrokeJoin::Miter;
    }
}
BlendMode from_sk_blend(SkBlendMode m) { return static_cast<BlendMode>(m); }
} // namespace

Color      Paint::fill_color()   const { return from_sk(impl_->fill.getColor()); }
Color      Paint::stroke_color() const { return from_sk(impl_->stroke.getColor()); }
float      Paint::stroke_width() const { return impl_->stroke.getStrokeWidth(); }
bool       Paint::anti_alias()   const { return impl_->fill.isAntiAlias(); }
StrokeCap  Paint::stroke_cap()   const { return from_sk_cap(impl_->stroke.getStrokeCap()); }
StrokeJoin Paint::stroke_join()  const { return from_sk_join(impl_->stroke.getStrokeJoin()); }
float      Paint::miter_limit()  const { return impl_->stroke.getStrokeMiter(); }
BlendMode  Paint::blend_mode()   const {
    return from_sk_blend(impl_->fill.getBlendMode_or(SkBlendMode::kSrcOver));
}
FillRule   Paint::fill_rule()    const { return impl_->fill_rule; }

void* Paint::native_fill_handle()   const { return &impl_->fill; }
void* Paint::native_stroke_handle() const { return &impl_->stroke; }

} // namespace swole
