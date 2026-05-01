#include "swole/render/font.hpp"

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkTypeface.h"

#if defined(__linux__)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

namespace swole {

namespace {

SkFontStyle to_sk_style(FontWeight w, FontStyle s) {
    int width = SkFontStyle::kNormal_Width;
    int weight = static_cast<int>(w);
    auto slant = (s == FontStyle::Italic) ? SkFontStyle::kItalic_Slant
                                          : SkFontStyle::kUpright_Slant;
    return SkFontStyle(weight, width, slant);
}

sk_sp<SkFontMgr> default_font_mgr() {
#if defined(__linux__)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return SkFontMgr::RefEmpty();
#endif
}

} // namespace

struct Font::Impl {
    sk_sp<SkTypeface> typeface;
    SkFont            font;
    std::string       family;
    float             size{14.f};
    FontWeight        weight{FontWeight::Regular};
    FontStyle         style{FontStyle::Normal};

    void rebuild() {
        auto mgr = default_font_mgr();
        const char* family_name = family.empty() ? nullptr : family.c_str();
        typeface = mgr->matchFamilyStyle(family_name, to_sk_style(weight, style));
        if (!typeface) typeface = mgr->matchFamilyStyle(nullptr, to_sk_style(weight, style));
        if (!typeface) typeface = SkTypeface::MakeEmpty();
        font = SkFont(typeface, size);
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        font.setSubpixel(true);
    }
};

Font::Font()  : impl_{std::make_unique<Impl>()} {
    impl_->family = "";
    impl_->rebuild();
}

Font::Font(std::string_view family, float size, FontWeight weight, FontStyle style)
    : impl_{std::make_unique<Impl>()} {
    impl_->family = family;
    impl_->size   = size;
    impl_->weight = weight;
    impl_->style  = style;
    impl_->rebuild();
}

Font::~Font() = default;
Font::Font(const Font& o)            : impl_{std::make_unique<Impl>(*o.impl_)} {}
Font& Font::operator=(const Font& o) { *impl_ = *o.impl_; return *this; }
Font::Font(Font&&) noexcept            = default;
Font& Font::operator=(Font&&) noexcept = default;

Font Font::with_size(float sz) const {
    Font f = *this; f.impl_->size = sz; f.impl_->rebuild(); return f;
}
Font Font::with_weight(FontWeight w) const {
    Font f = *this; f.impl_->weight = w; f.impl_->rebuild(); return f;
}
Font Font::with_style(FontStyle s) const {
    Font f = *this; f.impl_->style = s; f.impl_->rebuild(); return f;
}
Font Font::with_family(std::string_view fam) const {
    Font f = *this; f.impl_->family = fam; f.impl_->rebuild(); return f;
}

std::string  Font::family()  const { return impl_->family; }
float        Font::size()    const { return impl_->size; }
FontWeight   Font::weight()  const { return impl_->weight; }
FontStyle    Font::style()   const { return impl_->style; }

FontMetrics Font::metrics() const {
    SkFontMetrics m;
    impl_->font.getMetrics(&m);
    return {
        .ascent    = -m.fAscent,   // Skia ascent is negative, we flip
        .descent   =  m.fDescent,
        .leading   =  m.fLeading,
        .cap_height =  m.fCapHeight,
        .x_height  =  m.fXHeight,
    };
}

float Font::measure_text_width(std::string_view text) const {
    return impl_->font.measureText(text.data(), text.size(), SkTextEncoding::kUTF8);
}

SizeF Font::measure_text(std::string_view text) const {
    SkRect bounds;
    float w = impl_->font.measureText(text.data(), text.size(),
                                      SkTextEncoding::kUTF8, &bounds);
    return {w, bounds.height()};
}

void* Font::native_handle() const { return &impl_->font; }

} // namespace swole
