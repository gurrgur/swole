#pragma once

#include "../core/types.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace swole {

enum class FontStyle { Normal, Italic };
enum class FontWeight : int {
    Thin       = 100,
    ExtraLight = 200,
    Light      = 300,
    Regular    = 400,
    Medium     = 500,
    SemiBold   = 600,
    Bold       = 700,
    ExtraBold  = 800,
    Black      = 900,
};

// Horizontal text alignment for drawText calls.
enum class TextAlign { Left, Center, Right };
// Vertical text alignment.
enum class TextBaseline { Top, Middle, Bottom, Alphabetic };

struct FontMetrics {
    float ascent;    // distance from baseline to top of glyphs (positive up)
    float descent;   // distance from baseline to bottom (positive down)
    float leading;   // recommended line gap
    float cap_height;
    float x_height;
    [[nodiscard]] float line_height() const { return ascent + descent + leading; }
};

// Immutable font description. Wraps SkFont (pimpl).
class Font {
public:
    Font();
    explicit Font(std::string_view family, float size = 14.f,
                  FontWeight weight = FontWeight::Regular,
                  FontStyle style = FontStyle::Normal);
    ~Font();

    Font(const Font&);
    Font& operator=(const Font&);
    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;

    // Convenience: derive a modified copy
    [[nodiscard]] Font with_size(float sz) const;
    [[nodiscard]] Font with_weight(FontWeight w) const;
    [[nodiscard]] Font with_style(FontStyle s) const;
    [[nodiscard]] Font with_family(std::string_view family) const;
    [[nodiscard]] Font bold() const { return with_weight(FontWeight::Bold); }
    [[nodiscard]] Font italic() const { return with_style(FontStyle::Italic); }

    [[nodiscard]] std::string  family()  const;
    [[nodiscard]] float        size()    const;
    [[nodiscard]] FontWeight   weight()  const;
    [[nodiscard]] FontStyle    style()   const;

    [[nodiscard]] FontMetrics  metrics() const;
    [[nodiscard]] float        measure_text_width(std::string_view text) const;
    [[nodiscard]] SizeF        measure_text(std::string_view text) const;

    // Internal
    [[nodiscard]] void* native_handle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace swole
