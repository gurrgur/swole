#pragma once
// Internal default theme constants — not part of the public API.
// Replace with a proper theme system later.

#include "swole/core/types.hpp"
#include "swole/render/font.hpp"
#include "swole/render/paint.hpp"

namespace swole::theme {

// ── Palette ──────────────────────────────────────────────────────────────────

inline constexpr Color bg         {255, 255, 255};
inline constexpr Color bg_alt     {247, 247, 248};
inline constexpr Color surface    {240, 241, 243};
inline constexpr Color border     {200, 202, 206};
inline constexpr Color border_focus{59,  130, 246};

inline constexpr Color accent     {59,  130, 246};
inline constexpr Color accent_dark{37,   99, 235};
inline constexpr Color sel_bg     {219, 234, 254};
inline constexpr Color sel_text   {30,   64, 175};
inline constexpr Color hover_bg   {241, 245, 249};

inline constexpr Color text       {31,   41,  55};
inline constexpr Color text_dim   {107, 114, 128};
inline constexpr Color text_disabled{180,185,193};
inline constexpr Color placeholder {180, 185, 193};

inline constexpr Color track_bg   {226, 228, 231};
inline constexpr Color header_bg  {245, 246, 248};
inline constexpr Color row_alt    {250, 251, 252};
inline constexpr Color scrollbar  {200, 202, 206};
inline constexpr Color scrollbar_hover{150,153,160};

// ── Metrics ──────────────────────────────────────────────────────────────────

inline constexpr float radius       = 4.f;
inline constexpr float radius_small = 2.f;
inline constexpr int   scrollbar_w  = 10;

// ── Helpers ──────────────────────────────────────────────────────────────────

inline Font default_font(float size = 13.f) {
    return Font{"", size};
}

inline Paint fill(Color c) { return Paint::fill(c); }

inline Paint stroke(Color c, float w = 1.f) { return Paint::stroke(c, w); }

inline void draw_widget_bg(Canvas& canvas, RectF r, bool focused = false) {
    canvas.draw_round_rect(r.inset(.5f), radius, radius, fill(bg));
    canvas.draw_round_rect(r.inset(.5f), radius, radius,
        stroke(focused ? border_focus : border));
}

} // namespace swole::theme
