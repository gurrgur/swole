#include "swole/render/canvas.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkFont.h"
#include "include/core/SkImage.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRRect.h"
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"

#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "swole/render/image.hpp"

#include <vector>

namespace swole {

// ── Helpers ──────────────────────────────────────────────────────────────────

namespace {

inline SkRect  to_sk(RectF r)  { return SkRect::MakeXYWH(r.x, r.y, r.w, r.h); }
inline SkPoint to_sk(PointF p) { return SkPoint::Make(p.x, p.y); }
inline SkColor to_sk(Color c)  { return SkColorSetARGB(c.a, c.r, c.g, c.b); }

const SkFont& as_sk_font(const Font& f) {
    return *static_cast<const SkFont*>(f.native_handle());
}
const SkPath& as_sk_path(const Path& p) {
    return *static_cast<const SkPath*>(p.native_handle());
}
const SkPaint& fill_sk(const Paint& p) {
    return *static_cast<const SkPaint*>(p.native_fill_handle());
}
const SkPaint& stroke_sk(const Paint& p) {
    return *static_cast<const SkPaint*>(p.native_stroke_handle());
}

SkCanvas* sk(void* h) { return static_cast<SkCanvas*>(h); }

// Apply paint: draws with fill first, then stroke, so stroke sits on top.
template<typename DrawFn>
void apply_paint(const Paint& p, DrawFn&& fn) {
    if (p.is_fill())   fn(fill_sk(p));
    if (p.is_stroke()) fn(stroke_sk(p));
}

} // namespace

// ── Path impl ─────────────────────────────────────────────────────────────────

struct Path::Impl { SkPath path; };

Path::Path()  : impl_{std::make_unique<Impl>()} {}
Path::~Path() = default;
Path::Path(const Path& o)            : impl_{std::make_unique<Impl>(*o.impl_)} {}
Path& Path::operator=(const Path& o) { *impl_ = *o.impl_; return *this; }
Path::Path(Path&&) noexcept            = default;
Path& Path::operator=(Path&&) noexcept = default;

Path& Path::move_to(float x, float y)  { impl_->path.moveTo(x, y); return *this; }
Path& Path::line_to(float x, float y)  { impl_->path.lineTo(x, y); return *this; }
Path& Path::quad_to(float cx, float cy, float x, float y) {
    impl_->path.quadTo(cx, cy, x, y); return *this;
}
Path& Path::cubic_to(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    impl_->path.cubicTo(c1x, c1y, c2x, c2y, x, y); return *this;
}
Path& Path::arc_to(float rx, float ry, float xr, bool large, bool sweep, float x, float y) {
    using F = SkPath::ArcSize;
    impl_->path.arcTo(rx, ry, xr, large ? F::kLarge_ArcSize : F::kSmall_ArcSize,
                      sweep ? SkPathDirection::kCW : SkPathDirection::kCCW, x, y);
    return *this;
}
Path& Path::close()                    { impl_->path.close(); return *this; }
Path& Path::add_rect(RectF r, bool cw) {
    impl_->path.addRect(to_sk(r), cw ? SkPathDirection::kCW : SkPathDirection::kCCW);
    return *this;
}
Path& Path::add_round_rect(RectF r, float rx, float ry) {
    impl_->path.addRoundRect(to_sk(r), rx, ry); return *this;
}
Path& Path::add_oval(RectF r) { impl_->path.addOval(to_sk(r)); return *this; }
Path& Path::add_circle(float cx, float cy, float radius) {
    impl_->path.addCircle(cx, cy, radius); return *this;
}
Path& Path::reset()           { impl_->path.reset(); return *this; }
bool  Path::is_empty()  const { return impl_->path.isEmpty(); }
RectF Path::bounds()    const {
    auto b = impl_->path.getBounds();
    return {b.x(), b.y(), b.width(), b.height()};
}
void* Path::native_handle() const { return &impl_->path; }

// ── Canvas ────────────────────────────────────────────────────────────────────

void Canvas::bind(void* sk_canvas) { sk_canvas_ = sk_canvas; }

void Canvas::save()    { sk(sk_canvas_)->save(); }
void Canvas::restore() { sk(sk_canvas_)->restore(); }

void Canvas::translate(float dx, float dy)         { sk(sk_canvas_)->translate(dx, dy); }
void Canvas::scale(float sx, float sy)             { sk(sk_canvas_)->scale(sx, sy); }
void Canvas::rotate(float deg, float cx, float cy) { sk(sk_canvas_)->rotate(deg, cx, cy); }
void Canvas::skew(float sx, float sy)              { sk(sk_canvas_)->skew(sx, sy); }
void Canvas::concat(const float m[9]) {
    sk(sk_canvas_)->concat(SkMatrix::MakeAll(
        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]));
}
void Canvas::reset_transform() { sk(sk_canvas_)->resetMatrix(); }

void Canvas::clip_rect(RectF r, bool aa) {
    sk(sk_canvas_)->clipRect(to_sk(r), SkClipOp::kIntersect, aa);
}
void Canvas::clip_round_rect(RectF r, float rx, float ry, bool aa) {
    sk(sk_canvas_)->clipRRect(SkRRect::MakeRectXY(to_sk(r), rx, ry),
                              SkClipOp::kIntersect, aa);
}
void Canvas::clip_path(const Path& p, bool aa) {
    sk(sk_canvas_)->clipPath(as_sk_path(p), SkClipOp::kIntersect, aa);
}

void Canvas::clear(Color c) { sk(sk_canvas_)->clear(to_sk(c)); }

void Canvas::draw_rect(RectF r, const Paint& p) {
    apply_paint(p, [&](const SkPaint& sp) { sk(sk_canvas_)->drawRect(to_sk(r), sp); });
}
void Canvas::draw_round_rect(RectF r, float rx, float ry, const Paint& p) {
    apply_paint(p, [&](const SkPaint& sp) { sk(sk_canvas_)->drawRoundRect(to_sk(r), rx, ry, sp); });
}
void Canvas::draw_oval(RectF r, const Paint& p) {
    apply_paint(p, [&](const SkPaint& sp) { sk(sk_canvas_)->drawOval(to_sk(r), sp); });
}
void Canvas::draw_circle(float cx, float cy, float rad, const Paint& p) {
    apply_paint(p, [&](const SkPaint& sp) { sk(sk_canvas_)->drawCircle(cx, cy, rad, sp); });
}
void Canvas::draw_line(float x0, float y0, float x1, float y1, const Paint& p) {
    // Lines are always stroked; use the stroke paint if available, else fill.
    const SkPaint& sp = p.is_stroke() ? stroke_sk(p) : fill_sk(p);
    sk(sk_canvas_)->drawLine(x0, y0, x1, y1, sp);
}
void Canvas::draw_path(const Path& path, const Paint& p) {
    apply_paint(p, [&](const SkPaint& sp) { sk(sk_canvas_)->drawPath(as_sk_path(path), sp); });
}
void Canvas::draw_points(std::span<const PointF> pts, const Paint& p) {
    std::vector<SkPoint> sk_pts;
    sk_pts.reserve(pts.size());
    for (auto& pt : pts) sk_pts.push_back(to_sk(pt));
    const SkPaint& sp = p.is_stroke() ? stroke_sk(p) : fill_sk(p);
    sk(sk_canvas_)->drawPoints(SkCanvas::kPoints_PointMode,
                               sk_pts.size(), sk_pts.data(), sp);
}
void Canvas::draw_polygon(std::span<const PointF> pts, bool closed, const Paint& p) {
    if (pts.empty()) return;
    Path path;
    path.move_to(pts[0].x, pts[0].y);
    for (size_t i = 1; i < pts.size(); ++i) path.line_to(pts[i].x, pts[i].y);
    if (closed) path.close();
    draw_path(path, p);
}

// ── Images ────────────────────────────────────────────────────────────────────

void Canvas::draw_image(const Image& img, float x, float y, const Paint* p) {
    auto* sk_img = static_cast<SkImage*>(img.native_handle());
    if (!sk_img) return;
    sk_sp<SkImage> ref(sk_img);
    const SkPaint* sp = p ? &fill_sk(*p) : nullptr;
    sk(sk_canvas_)->drawImage(ref, x, y, SkSamplingOptions(), sp);
}
void Canvas::draw_image(const Image& img, RectF dst, const Paint* p) {
    draw_image(img, {0, 0, float(img.width()), float(img.height())}, dst,
               ScaleFilter::Linear, p);
}
void Canvas::draw_image(const Image& img, RectF src, RectF dst,
                        ScaleFilter filter, const Paint* p) {
    auto* sk_img = static_cast<SkImage*>(img.native_handle());
    if (!sk_img) return;
    sk_sp<SkImage> ref(sk_img);
    SkSamplingOptions sampling;
    switch (filter) {
    case ScaleFilter::Nearest: sampling = SkSamplingOptions(SkFilterMode::kNearest); break;
    case ScaleFilter::Linear:  sampling = SkSamplingOptions(SkFilterMode::kLinear);  break;
    case ScaleFilter::Cubic:   sampling = SkSamplingOptions(SkCubicResampler::Mitchell()); break;
    }
    const SkPaint* sp = p ? &fill_sk(*p) : nullptr;
    sk(sk_canvas_)->drawImageRect(ref, to_sk(src), to_sk(dst), sampling, sp,
                                  SkCanvas::kStrict_SrcRectConstraint);
}
void Canvas::draw_image_nine(const Image& img, RectI center, RectF dst, const Paint* p) {
    auto* sk_img = static_cast<SkImage*>(img.native_handle());
    if (!sk_img) return;
    sk_sp<SkImage> ref(sk_img);
    SkIRect sk_center = SkIRect::MakeLTRB(center.left(), center.top(),
                                          center.right(), center.bottom());
    const SkPaint* sp = p ? &fill_sk(*p) : nullptr;
    sk(sk_canvas_)->drawImageNine(ref.get(), sk_center, to_sk(dst),
                                  SkFilterMode::kLinear, sp);
}

// ── Text ──────────────────────────────────────────────────────────────────────

void Canvas::draw_text(std::string_view text, float x, float y,
                       const Font& font, const Paint& p) {
    // Text is always drawn with the fill paint.
    sk(sk_canvas_)->drawSimpleText(text.data(), text.size(),
                                   SkTextEncoding::kUTF8, x, y,
                                   as_sk_font(font), fill_sk(p));
}

void Canvas::draw_text(std::string_view text, RectF bounds,
                       TextAlign align, TextBaseline baseline,
                       const Font& font, const Paint& paint) {
    const SkFont& sf = as_sk_font(font);
    SkFontMetrics fm;
    sf.getMetrics(&fm);

    float text_w = sf.measureText(text.data(), text.size(), SkTextEncoding::kUTF8);

    float x = bounds.x;
    switch (align) {
    case TextAlign::Left:   x = bounds.x; break;
    case TextAlign::Center: x = bounds.x + (bounds.w - text_w) * 0.5f; break;
    case TextAlign::Right:  x = bounds.right() - text_w; break;
    }

    float y = bounds.y;
    switch (baseline) {
    case TextBaseline::Top:
        y = bounds.y - fm.fAscent;
        break;
    case TextBaseline::Middle:
        y = bounds.y + bounds.h * 0.5f - (fm.fAscent + fm.fDescent) * 0.5f - fm.fDescent;
        break;
    case TextBaseline::Bottom:
        y = bounds.bottom() - fm.fDescent;
        break;
    case TextBaseline::Alphabetic:
        y = bounds.y;
        break;
    }

    draw_text(text, x, y, font, paint);
}

// ── Gradients ─────────────────────────────────────────────────────────────────

namespace {
// Build SkColor array and optional stops vector for gradient calls.
// Returns the skia-ready colors; populates sk_stops if stops is non-empty.
static std::vector<SkColor>
build_gradient_colors(std::span<const Color> colors,
                      std::span<const float> stops,
                      std::vector<float>& sk_stops_out) {
    std::vector<SkColor> sk_colors;
    sk_colors.reserve(colors.size());
    for (auto& c : colors) sk_colors.push_back(to_sk(c));
    if (!stops.empty()) {
        sk_stops_out.assign(stops.begin(), stops.end());
    }
    return sk_colors;
}
} // namespace

void Canvas::fill_linear_gradient(RectF dst, PointF from, PointF to,
                                   std::span<const Color> colors,
                                   std::span<const float> stops) {
    if (colors.size() < 2) return;
    std::vector<float> sk_stops;
    auto sk_colors = build_gradient_colors(colors, stops, sk_stops);

    SkPoint pts[2] = {to_sk(from), to_sk(to)};
    auto shader = SkGradientShader::MakeLinear(
        pts, sk_colors.data(),
        sk_stops.empty() ? nullptr : sk_stops.data(),
        int(sk_colors.size()),
        SkTileMode::kClamp);

    SkPaint sp;
    sp.setAntiAlias(true);
    sp.setShader(std::move(shader));
    sk(sk_canvas_)->drawRect(to_sk(dst), sp);
}

void Canvas::fill_radial_gradient(RectF dst, PointF center, float radius,
                                   std::span<const Color> colors,
                                   std::span<const float> stops) {
    if (colors.size() < 2) return;
    std::vector<float> sk_stops;
    auto sk_colors = build_gradient_colors(colors, stops, sk_stops);

    auto shader = SkGradientShader::MakeRadial(
        to_sk(center), radius,
        sk_colors.data(),
        sk_stops.empty() ? nullptr : sk_stops.data(),
        int(sk_colors.size()),
        SkTileMode::kClamp);

    SkPaint sp;
    sp.setAntiAlias(true);
    sp.setShader(std::move(shader));
    sk(sk_canvas_)->drawRect(to_sk(dst), sp);
}

void Canvas::fill_round_rect_linear_gradient(RectF dst, float rx, float ry,
                                              PointF from, PointF to,
                                              std::span<const Color> colors,
                                              std::span<const float> stops) {
    if (colors.size() < 2) return;
    std::vector<float> sk_stops;
    auto sk_colors = build_gradient_colors(colors, stops, sk_stops);

    SkPoint pts[2] = {to_sk(from), to_sk(to)};
    auto shader = SkGradientShader::MakeLinear(
        pts, sk_colors.data(),
        sk_stops.empty() ? nullptr : sk_stops.data(),
        int(sk_colors.size()),
        SkTileMode::kClamp);

    SkPaint sp;
    sp.setAntiAlias(true);
    sp.setShader(std::move(shader));
    sk(sk_canvas_)->drawRoundRect(to_sk(dst), rx, ry, sp);
}

// ── Shadows ───────────────────────────────────────────────────────────────────

void Canvas::draw_shadow(RectF r, float blur_radius,
                          float dx, float dy,
                          Color color, float corner_radius) {
    SkPaint sp;
    sp.setAntiAlias(true);
    sp.setColor(to_sk(color));
    sp.setImageFilter(
        SkImageFilters::Blur(blur_radius / 2.f, blur_radius / 2.f, nullptr));

    // Expand the rect slightly so the blur doesn't get clipped at edges
    float expand = blur_radius * 1.5f;
    RectF shadow_r{r.x + dx - expand, r.y + dy - expand,
                   r.w + expand * 2.f,  r.h + expand * 2.f};

    auto guard = scoped_save();
    // Clip to prevent shadow bleeding into foreground content
    clip_rect({r.x - expand, r.y - expand,
               r.w + expand*2.f, r.h + expand*2.f + blur_radius*2.f});
    if (corner_radius > 0.f)
        sk(sk_canvas_)->drawRoundRect(to_sk(shadow_r), corner_radius, corner_radius, sp);
    else
        sk(sk_canvas_)->drawRect(to_sk(shadow_r), sp);
}

} // namespace swole
