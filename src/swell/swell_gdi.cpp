#include "swell_handles.hpp"
#include "swell_widget_host.hpp"

#include "swole/render/font.hpp"
#include "swole/render/image.hpp"

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

extern "C" {

BOOL SWELL_InsertMenu(HMENU menu, int pos, int flags, int id, const char* text);

int MessageBox(HWND hwnd, const char* text, const char* title, int flags) {
    (void)hwnd;
    (void)text;
    (void)title;
    (void)flags;
    return 0;
}

BOOL AddMenuItem(HMENU menu, int pos, int flags, int id, const char* text) {
    return SWELL_InsertMenu(menu, pos, flags, id, text);
}

struct FontObject {
    std::string face;
    int height;
    int weight;
    bool italic;
    bool underline;
    bool strikeout;
};

static std::unordered_map<HGDIOBJ, std::unique_ptr<FontObject>> g_fonts;
static std::mutex g_fonts_mutex;

HFONT CreateFont(int height, int width, int escapement, int orientation, int weight,
                 DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                 DWORD outprecision, DWORD clipprecision, DWORD quality,
                 DWORD pitchandfamily, const char* face) {
    (void)width;
    (void)escapement;
    (void)orientation;
    (void)outprecision;
    (void)clipprecision;
    (void)quality;
    (void)pitchandfamily;

    auto font = std::make_unique<FontObject>();
    font->height = height;
    font->weight = weight;
    font->italic = (italic != 0);
    font->underline = (underline != 0);
    font->strikeout = (strikeout != 0);
    font->face = face ? face : "";

    HGDIOBJ handle = reinterpret_cast<HGDIOBJ>(font.get());

    std::lock_guard lock{g_fonts_mutex};
    g_fonts[handle] = std::move(font);

    return reinterpret_cast<HFONT>(handle);
}

COLORREF SetTextColor(HDC hdc, COLORREF color) {
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc) return 0;

    COLORREF old = dc->text_color;
    dc->text_color = color;
    return old;
}

COLORREF SetBkColor(HDC hdc, COLORREF color) {
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc) return 0;

    COLORREF old = dc->bk_color;
    dc->bk_color = color;
    return old;
}

BOOL RoundRect(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3) {
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc || !dc->canvas) return SWELL_FALSE;

    swole::Paint paint;
    if (dc->selected_brush) {
        paint = swole::Paint::fill(swole::Color(
            (dc->bk_color >> 16) & 0xFF,
            (dc->bk_color >> 8) & 0xFF,
            dc->bk_color & 0xFF
        ));
    }

    float rx = float(x3) / 2.f;
    float ry = float(y3) / 2.f;

    swole::RectF rect{float(x1), float(y1), float(x2 - x1), float(y2 - y1)};
    dc->canvas->draw_round_rect(rect, rx, ry, paint);

    return SWELL_TRUE;
}

struct BitmapObject {
    int width;
    int height;
    int planes;
    int bitcount;
    std::vector<uint8_t> bits;
};

static std::unordered_map<HGDIOBJ, std::unique_ptr<BitmapObject>> g_bitmaps;
static std::mutex g_bitmaps_mutex;

HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bitcount, const void* bits) {
    auto bmp = std::make_unique<BitmapObject>();
    bmp->width = width;
    bmp->height = height;
    bmp->planes = planes;
    bmp->bitcount = bitcount;

    int stride = (width * bitcount + 31) / 32 * 4;
    bmp->bits.resize(size_t(stride) * size_t(height));

    if (bits) {
        std::memcpy(bmp->bits.data(), bits, bmp->bits.size());
    }

    HGDIOBJ handle = reinterpret_cast<HGDIOBJ>(bmp.get());

    std::lock_guard lock{g_bitmaps_mutex};
    g_bitmaps[handle] = std::move(bmp);

    return reinterpret_cast<HBITMAP>(handle);
}

HDC GetDC(HWND hwnd) {
    auto* h = swole::swell::as_hwnd(hwnd);
    if (!h) return nullptr;

    auto* dc = new HDC__{};
    dc->owner = hwnd;

    return reinterpret_cast<HDC>(dc);
}

HDC GetWindowDC(HWND hwnd) {
    return GetDC(hwnd);
}

BOOL ReleaseDC(HWND hwnd, HDC hdc) {
    (void)hwnd;
    auto* dc = swole::swell::as_hdc(hdc);
    if (!dc) return SWELL_FALSE;

    delete dc;
    return SWELL_TRUE;
}

}
