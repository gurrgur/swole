#include "swole/dialog.hpp"
#include "swole/core/application.hpp"
#include "swole/window/window.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "swole/render/font.hpp"
#include "swole/widgets/button.hpp"
#include "theme.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace swole {

// ── Helpers ───────────────────────────────────────────────────────────────────

static void modal_loop(std::function<bool()> done, Window* modal = nullptr) {
    SDL_Event ev;
    while (!done()) {
        SDL_WaitEventTimeout(&ev, 16);
        do {
            if (modal) modal->process_sdl_event(&ev);
            else for (Window* w : Application::instance().windows())
                     w->process_sdl_event(&ev);
        } while (!done() && SDL_PollEvent(&ev));
    }
}

// ── Message box ───────────────────────────────────────────────────────────────

MessageBoxResult message_box(Window*           parent,
                             std::string_view  title,
                             std::string_view  message,
                             MessageBoxButtons buttons,
                             MessageBoxIcon    icon) {

    SDL_MessageBoxFlags sdl_flags = SDL_MESSAGEBOX_INFORMATION;
    switch (icon) {
    case MessageBoxIcon::Warning: sdl_flags = SDL_MESSAGEBOX_WARNING; break;
    case MessageBoxIcon::Error:   sdl_flags = SDL_MESSAGEBOX_ERROR;   break;
    default:                                                           break;
    }

    struct BtnDef { int id; const char* text; };
    std::vector<BtnDef> btn_defs;
    switch (buttons) {
    case MessageBoxButtons::OkCancel:    btn_defs = {{1,"OK"},{0,"Cancel"}};              break;
    case MessageBoxButtons::YesNo:       btn_defs = {{2,"Yes"},{3,"No"}};                 break;
    case MessageBoxButtons::YesNoCancel: btn_defs = {{2,"Yes"},{3,"No"},{0,"Cancel"}};    break;
    case MessageBoxButtons::RetryCancel: btn_defs = {{4,"Retry"},{0,"Cancel"}};           break;
    default:                             btn_defs = {{1,"OK"}};                           break;
    }

    std::vector<SDL_MessageBoxButtonData> sdl_btns;
    sdl_btns.reserve(btn_defs.size());
    for (auto& b : btn_defs)
        sdl_btns.push_back({0, b.id, b.text});
    sdl_btns.front().flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    sdl_btns.back().flags  = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

    std::string title_str{title}, msg_str{message};
    SDL_MessageBoxData data{};
    data.flags      = sdl_flags;
    data.window     = parent ? static_cast<SDL_Window*>(parent->native_handle()) : nullptr;
    data.title      = title_str.c_str();
    data.message    = msg_str.c_str();
    data.numbuttons = int(sdl_btns.size());
    data.buttons    = sdl_btns.data();

    int clicked = 0;
    SDL_ShowMessageBox(&data, &clicked);

    switch (clicked) {
    case 0: return MessageBoxResult::Cancel;
    case 1: return MessageBoxResult::Ok;
    case 2: return MessageBoxResult::Yes;
    case 3: return MessageBoxResult::No;
    case 4: return MessageBoxResult::Retry;
    default: return MessageBoxResult::Cancel;
    }
}

// ── File dialogs ──────────────────────────────────────────────────────────────

struct FileDialogState {
    std::atomic<bool>                  done{false};
    std::vector<std::filesystem::path> paths;
};

static void file_dialog_cb(void* userdata,
                           const char* const* filelist,
                           int /*filter*/) {
    auto* s = static_cast<FileDialogState*>(userdata);
    if (filelist)
        for (int i = 0; filelist[i]; ++i)
            s->paths.emplace_back(filelist[i]);
    s->done.store(true, std::memory_order_release);
}

// Build SDL3 filter structs. The string data is kept alive in `storage`.
static std::vector<SDL_DialogFileFilter>
build_sdl_filters(const std::vector<FileFilter>& filters,
                  std::vector<std::string>& storage) {
    std::vector<SDL_DialogFileFilter> out;
    out.reserve(filters.size());
    for (auto& f : filters) {
        std::string pat;
        for (size_t i = 0; i < f.extensions.size(); ++i) {
            if (i) pat += ';';
            pat += f.extensions[i];
        }
        storage.push_back(std::move(pat));
        out.push_back({f.name.c_str(), storage.back().c_str()});
    }
    return out;
}

std::vector<std::filesystem::path>
open_file_dialog(Window* parent, OpenFileOptions opts) {
    FileDialogState state;
    std::vector<std::string> pat_storage;
    auto sdl_filters = build_sdl_filters(opts.filters, pat_storage);

    SDL_Window* sdl_win = parent
        ? static_cast<SDL_Window*>(parent->native_handle()) : nullptr;

    std::string loc = !opts.initial_file.empty() ? opts.initial_file.string()
                    : !opts.initial_dir.empty()  ? opts.initial_dir.string()
                    : std::string{};

    SDL_ShowOpenFileDialog(
        file_dialog_cb, &state, sdl_win,
        sdl_filters.empty() ? nullptr : sdl_filters.data(),
        int(sdl_filters.size()),
        loc.empty() ? nullptr : loc.c_str(),
        opts.allow_multiple);

    modal_loop([&]{ return state.done.load(std::memory_order_acquire); });
    return std::move(state.paths);
}

std::optional<std::filesystem::path>
save_file_dialog(Window* parent, SaveFileOptions opts) {
    FileDialogState state;
    std::vector<std::string> pat_storage;
    auto sdl_filters = build_sdl_filters(opts.filters, pat_storage);

    SDL_Window* sdl_win = parent
        ? static_cast<SDL_Window*>(parent->native_handle()) : nullptr;

    std::string loc = !opts.initial_file.empty() ? opts.initial_file.string()
                    : !opts.initial_dir.empty()  ? opts.initial_dir.string()
                    : std::string{};

    SDL_ShowSaveFileDialog(
        file_dialog_cb, &state, sdl_win,
        sdl_filters.empty() ? nullptr : sdl_filters.data(),
        int(sdl_filters.size()),
        loc.empty() ? nullptr : loc.c_str());

    modal_loop([&]{ return state.done.load(std::memory_order_acquire); });
    if (state.paths.empty()) return std::nullopt;
    return state.paths.front();
}

std::optional<std::filesystem::path>
choose_directory(Window* parent, std::string_view /*title*/,
                 std::filesystem::path initial) {
    FileDialogState state;
    SDL_Window* sdl_win = parent
        ? static_cast<SDL_Window*>(parent->native_handle()) : nullptr;
    std::string loc = initial.empty() ? std::string{} : initial.string();

    SDL_ShowOpenFolderDialog(
        file_dialog_cb, &state, sdl_win,
        loc.empty() ? nullptr : loc.c_str(),
        false);

    modal_loop([&]{ return state.done.load(std::memory_order_acquire); });
    if (state.paths.empty()) return std::nullopt;
    return state.paths.front();
}

// ── Color picker ──────────────────────────────────────────────────────────────

namespace {

static void hsv_to_rgb(float h, float s, float v,
                       float& r, float& g, float& b) noexcept {
    if (s <= 0.f) { r = g = b = v; return; }
    float hh = std::fmod(h, 360.f) / 60.f;
    int   i  = int(hh);
    float ff = hh - float(i);
    float p = v*(1.f-s), q = v*(1.f-s*ff), t = v*(1.f-s*(1.f-ff));
    switch (i) {
    case 0: r=v;g=t;b=p; break; case 1: r=q;g=v;b=p; break;
    case 2: r=p;g=v;b=t; break; case 3: r=p;g=q;b=v; break;
    case 4: r=t;g=p;b=v; break; default:r=v;g=p;b=q; break;
    }
}

static void rgb_to_hsv(float r, float g, float b,
                       float& h, float& s, float& v) noexcept {
    float mx = std::max({r,g,b}), mn = std::min({r,g,b});
    v = mx;
    float d = mx - mn;
    s = (mx < 1e-6f) ? 0.f : d / mx;
    if (d < 1e-6f) { h = 0.f; return; }
    if      (r >= mx) h = (g-b)/d;
    else if (g >= mx) h = 2.f+(b-r)/d;
    else              h = 4.f+(r-g)/d;
    h *= 60.f;
    if (h < 0.f) h += 360.f;
}

static void draw_checker(Canvas& canvas, RectF r, float cell = 6.f) {
    canvas.draw_rect(r, Paint::fill({255,255,255}));
    for (float y = r.y; y < r.bottom(); y += cell)
        for (float x = r.x; x < r.right(); x += cell)
            if ((int((x-r.x)/cell) + int((y-r.y)/cell)) % 2)
                canvas.draw_rect({x, y,
                    std::min(cell, r.right()-x),
                    std::min(cell, r.bottom()-y)},
                    Paint::fill({200,200,200}));
}

static Image make_sv_image(float hue) {
    constexpr int N = 256;
    std::vector<uint8_t> px(N * N * 4);
    for (int y = 0; y < N; ++y) {
        float v = 1.f - float(y)/float(N-1);
        for (int x = 0; x < N; ++x) {
            float r,g,b;
            hsv_to_rgb(hue, float(x)/float(N-1), v, r, g, b);
            int k = (y*N+x)*4;
            px[k]=uint8_t(r*255); px[k+1]=uint8_t(g*255);
            px[k+2]=uint8_t(b*255); px[k+3]=255;
        }
    }
    return Image::from_pixels(N, N, ImageFormat::RGBA8, px);
}

static Image make_hue_image() {
    constexpr int W=16, H=256;
    std::vector<uint8_t> px(W*H*4);
    for (int y = 0; y < H; ++y) {
        float r,g,b;
        hsv_to_rgb(float(y)/float(H-1)*360.f, 1.f, 1.f, r, g, b);
        uint8_t R=uint8_t(r*255), G=uint8_t(g*255), B=uint8_t(b*255);
        for (int x = 0; x < W; ++x) {
            int k=(y*W+x)*4; px[k]=R;px[k+1]=G;px[k+2]=B;px[k+3]=255;
        }
    }
    return Image::from_pixels(W, H, ImageFormat::RGBA8, px);
}

static Image make_alpha_image(Color c) {
    constexpr int W=256, H=16;
    std::vector<uint8_t> px(W*H*4);
    for (int x = 0; x < W; ++x) {
        uint8_t a = uint8_t(float(x)/float(W-1)*255.f);
        for (int y = 0; y < H; ++y) {
            int k=(y*W+x)*4; px[k]=c.r;px[k+1]=c.g;px[k+2]=c.b;px[k+3]=a;
        }
    }
    return Image::from_pixels(W, H, ImageFormat::RGBA8, px);
}

class ColorPickerWidget final : public Widget {
public:
    static constexpr int kSVSize  = 200;
    static constexpr int kHueW    =  20;
    static constexpr int kAlphaH  =  20;
    static constexpr int kGap     =   8;
    static constexpr int kPreviewH=  28;

    Color cur_rgba;
    bool  show_alpha;

    ColorPickerWidget(Widget* parent, Color initial, bool alpha)
        : Widget(parent), cur_rgba(initial), show_alpha(alpha) {
        set_focus_policy(FocusPolicy::Click);
        rgb_to_hsv(initial.r/255.f, initial.g/255.f, initial.b/255.f,
                   hue_, sat_, val_);
        hue_img_ = make_hue_image();
        sv_dirty_ = alpha_dirty_ = true;
    }

    Color cur_color() const {
        float r,g,b;
        hsv_to_rgb(hue_, sat_, val_, r, g, b);
        return {uint8_t(r*255), uint8_t(g*255), uint8_t(b*255), cur_rgba.a};
    }

protected:
    void on_paint(Canvas& canvas) override {
        if (sv_dirty_)    { sv_img_    = make_sv_image(hue_); sv_dirty_ = false; }
        if (alpha_dirty_ && show_alpha) {
            alpha_img_  = make_alpha_image(cur_color()); alpha_dirty_ = false;
        }

        RectF sv = sv_r();
        canvas.draw_image(sv_img_, sv);
        float cx = sv.x + sat_*sv.w, cy = sv.y + (1.f-val_)*sv.h;
        canvas.draw_circle(cx, cy, 5.f, Paint::stroke({255,255,255}, 1.5f));
        canvas.draw_circle(cx, cy, 3.5f, Paint::stroke({0,0,0}, 1.f));

        RectF hr = hue_r();
        canvas.draw_image(hue_img_, hr);
        float hy = hr.y + hue_/360.f * hr.h;
        canvas.draw_line(hr.x-2.f, hy, hr.right()+2.f, hy, Paint::stroke({255,255,255},2.f));
        canvas.draw_line(hr.x-2.f, hy, hr.right()+2.f, hy, Paint::stroke({0,0,0},1.f));

        if (show_alpha) {
            RectF ar = alpha_r();
            draw_checker(canvas, ar, 5.f);
            canvas.draw_image(alpha_img_, ar);
            float ax = ar.x + float(cur_rgba.a)/255.f * ar.w;
            canvas.draw_line(ax, ar.y-2.f, ax, ar.bottom()+2.f, Paint::stroke({255,255,255},2.f));
            canvas.draw_line(ax, ar.y-2.f, ax, ar.bottom()+2.f, Paint::stroke({0,0,0},1.f));
        }

        RectF pr = preview_r();
        draw_checker(canvas, pr, 6.f);
        Color cc = cur_color();
        canvas.draw_rect(pr, Paint::fill(cc));
        canvas.draw_rect(pr.inset(.5f), Paint::stroke(theme::border));

        Font f = theme::default_font(12.f);
        auto m = f.metrics();
        char hex[12];
        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", cc.r, cc.g, cc.b, cc.a);
        canvas.draw_text(hex, pr.x+6.f,
                         pr.y+(pr.h-m.ascent-m.descent)*.5f+m.ascent,
                         f, Paint::fill(theme::text));
    }

    void on_mouse_press(const MouseEvent& e) override {
        PointF p{float(e.pos.x), float(e.pos.y)};
        if (sv_r().contains(p))                      { drag_=Drag::SV;    do_sv(p); }
        else if (hue_r().contains(p))                { drag_=Drag::Hue;   do_hue(p); }
        else if (show_alpha && alpha_r().contains(p)){ drag_=Drag::Alpha; do_alpha(p); }
    }
    void on_mouse_move(const MouseEvent& e) override {
        PointF p{float(e.pos.x), float(e.pos.y)};
        switch (drag_) {
        case Drag::SV:    do_sv(p);    break;
        case Drag::Hue:   do_hue(p);   break;
        case Drag::Alpha: do_alpha(p); break;
        default: break;
        }
    }
    void on_mouse_release(const MouseEvent&) override { drag_ = Drag::None; }

private:
    RectF sv_r()      const { return {0.f,0.f,float(kSVSize),float(kSVSize)}; }
    RectF hue_r()     const { return {float(kSVSize+kGap),0.f,float(kHueW),float(kSVSize)}; }
    RectF alpha_r()   const { return {0.f,float(kSVSize+kGap),float(kSVSize),float(kAlphaH)}; }
    RectF preview_r() const {
        float y = float(kSVSize) + (show_alpha ? kGap+kAlphaH : 0) + kGap;
        return {0.f, y, float(kSVSize+kGap+kHueW), float(kPreviewH)};
    }

    void do_sv(PointF p) {
        RectF sv=sv_r();
        sat_ = std::clamp((p.x-sv.x)/sv.w, 0.f,1.f);
        val_ = std::clamp(1.f-(p.y-sv.y)/sv.h, 0.f,1.f);
        alpha_dirty_=true; cur_rgba=cur_color(); invalidate();
    }
    void do_hue(PointF p) {
        RectF hr=hue_r();
        hue_ = std::clamp((p.y-hr.y)/hr.h, 0.f,1.f)*360.f;
        sv_dirty_=alpha_dirty_=true; cur_rgba=cur_color(); invalidate();
    }
    void do_alpha(PointF p) {
        RectF ar=alpha_r();
        cur_rgba.a = uint8_t(std::clamp((p.x-ar.x)/ar.w, 0.f,1.f)*255.f);
        invalidate();
    }

    float hue_{0.f}, sat_{1.f}, val_{1.f};
    Image sv_img_, hue_img_, alpha_img_;
    bool  sv_dirty_{true}, alpha_dirty_{true};
    enum class Drag { None, SV, Hue, Alpha } drag_{Drag::None};
};

} // namespace (color picker)

std::optional<Color> choose_color(Window* parent, Color initial, bool show_alpha) {
    const int kSV      = ColorPickerWidget::kSVSize;
    const int kHue     = ColorPickerWidget::kHueW;
    const int kGap     = ColorPickerWidget::kGap;
    const int kPrev    = ColorPickerWidget::kPreviewH;
    const int kAlpha   = show_alpha ? ColorPickerWidget::kAlphaH + kGap : 0;

    const int kW = kSV + kGap + kHue + 16;
    const int kPickH = kSV + kAlpha + kGap + kPrev;
    const int kBtnH  = 36;
    const int kH = kPickH + kBtnH + 16;

    WindowConfig cfg;
    cfg.title = "Choose Colour";
    cfg.size  = {kW, kH};
    cfg.flags = WindowFlags::None;
    if (parent) {
        PointI pp  = parent->position();
        SizeI  ps  = parent->size();
        cfg.position = {pp.x + (ps.w - kW)/2, pp.y + (ps.h - kH)/2};
    }

    Window dlg{cfg};
    bool cancelled = false;

    auto* picker = dlg.root().emplace_child<ColorPickerWidget>(initial, show_alpha);
    picker->set_bounds({8, 8, kW-16, kPickH});

    auto* ok_btn = dlg.root().emplace_child<Button>("OK");
    auto* cn_btn = dlg.root().emplace_child<Button>("Cancel");
    ok_btn->set_bounds({kW-162, kPickH+12, 72, 28});
    cn_btn->set_bounds({kW- 82, kPickH+12, 72, 28});

    (void)ok_btn->on_clicked.connect([&]{ dlg.close(); });
    (void)cn_btn->on_clicked.connect([&]{ cancelled = true; dlg.close(); });

    dlg.show();
    modal_loop([&]{ return dlg.is_closed(); }, &dlg);

    if (cancelled) return std::nullopt;
    return picker->cur_color();
}

// ── Font picker ───────────────────────────────────────────────────────────────

namespace {

static constexpr const char* kFontFamilies[] = {
    "",              // system default (displayed as "System Default")
    "Arial", "Helvetica Neue", "Verdana", "Tahoma", "Trebuchet MS",
    "Georgia", "Times New Roman", "Palatino Linotype",
    "Courier New", "Lucida Console", "Menlo", "Monaco", "Consolas",
    "Impact", "Comic Sans MS",
};
static constexpr int kFontCount = int(std::size(kFontFamilies));

class FontPickerWidget final : public Widget {
public:
    static constexpr int kListW    = 200;
    static constexpr int kItemH    =  22;
    static constexpr int kListRows =   7;
    static constexpr int kListH    = kItemH * kListRows;
    static constexpr int kRightX   = kListW + 12;
    static constexpr int kPreviewH =  60;

    Color    color{31,41,55};
    bool     show_color;

    FontPickerWidget(Widget* parent, const Font& initial, bool sc)
        : Widget(parent), show_color(sc) {
        set_focus_policy(FocusPolicy::Click);
        for (int i = 0; i < kFontCount; ++i)
            if (initial.family() == kFontFamilies[i]) { family_idx_ = i; break; }
        font_size_ = initial.size();
        bold_      = initial.weight() >= FontWeight::Bold;
        italic_    = initial.style() == FontStyle::Italic;
    }

    Font cur_font() const {
        return Font{kFontFamilies[family_idx_], font_size_,
                    bold_   ? FontWeight::Bold    : FontWeight::Regular,
                    italic_ ? FontStyle::Italic   : FontStyle::Normal};
    }

protected:
    void on_paint(Canvas& canvas) override {
        Font ui = theme::default_font(12.f);
        auto m  = ui.metrics();
        float lh = m.ascent + m.descent;

        // ── Family list ──
        RectF lr{4.f, 4.f, float(kListW), float(kListH)};
        canvas.draw_rect(lr.inset(.5f), Paint::fill(theme::bg));
        canvas.draw_rect(lr.inset(.5f), Paint::stroke(theme::border));
        {
            auto g = canvas.scoped_save();
            canvas.clip_rect(lr);
            for (int i = 0; i < kFontCount; ++i) {
                float iy = lr.y + float(i*kItemH - scroll_y_);
                if (iy + kItemH < lr.y || iy > lr.bottom()) continue;
                RectF ir{lr.x, iy, lr.w, float(kItemH)};
                if (i == family_idx_)
                    canvas.draw_rect(ir, Paint::fill(theme::sel_bg));
                else if (i == hovered_)
                    canvas.draw_rect(ir, Paint::fill(theme::hover_bg));
                const char* nm = kFontFamilies[i][0] ? kFontFamilies[i] : "System Default";
                Color tc = (i == family_idx_) ? theme::sel_text : theme::text;
                canvas.draw_text(nm, ir.x+6.f, ir.y+(ir.h-lh)*.5f+m.ascent, ui, Paint::fill(tc));
            }
        }

        // ── Right panel ──
        float rx = float(kRightX);
        float ry = 4.f;

        // Size label + box
        canvas.draw_text("Size:", rx, ry+m.ascent, ui, Paint::fill(theme::text));
        RectF sz_box{rx+38.f, ry, 54.f, 22.f};
        theme::draw_widget_bg(canvas, sz_box);
        char sz_str[16]; std::snprintf(sz_str, sizeof(sz_str), "%.0f", font_size_);
        canvas.draw_text(sz_str, sz_box.x+5.f, ry+m.ascent, ui, Paint::fill(theme::text));

        // Bold / Italic toggles
        ry += 30.f;
        auto draw_toggle = [&](const char* label, bool on, float tx, float tw) {
            RectF tr{tx, ry, tw, 22.f};
            canvas.draw_round_rect(tr.inset(.5f), 3.f,3.f,
                Paint::fill(on ? theme::sel_bg : theme::surface));
            canvas.draw_round_rect(tr.inset(.5f), 3.f,3.f, Paint::stroke(theme::border));
            float tw2 = ui.measure_text_width(label);
            canvas.draw_text(label, tr.x+(tr.w-tw2)*.5f,
                             tr.y+(tr.h-lh)*.5f+m.ascent,
                             ui, Paint::fill(on ? theme::sel_text : theme::text));
        };
        draw_toggle("Bold",   bold_,   rx,      50.f);
        draw_toggle("Italic", italic_, rx+56.f, 50.f);

        // Preview
        ry += 30.f;
        float pw = float(size().w) - rx - 4.f;
        RectF pr{rx, ry, pw, float(kPreviewH)};
        theme::draw_widget_bg(canvas, pr);
        {
            auto g = canvas.scoped_save();
            canvas.clip_rect(pr.inset(2.f));
            Font pf = cur_font();
            auto pm = pf.metrics();
            canvas.draw_text("The quick brown fox",
                             pr.x+6.f,
                             pr.y+(pr.h-pm.ascent-pm.descent)*.5f+pm.ascent,
                             pf, Paint::fill(theme::text));
        }
    }

    void on_mouse_press(const MouseEvent& e) override {
        PointF p{float(e.pos.x), float(e.pos.y)};
        RectF lr{4.f,4.f,float(kListW),float(kListH)};
        if (lr.contains(p)) {
            int i = int(p.y - lr.y + float(scroll_y_)) / kItemH;
            if (i >= 0 && i < kFontCount) { family_idx_ = i; invalidate(); }
            return;
        }
        float rx = float(kRightX);
        float ry = 34.f;
        RectF bold_r{rx, ry, 50.f, 22.f};
        RectF ital_r{rx+56.f, ry, 50.f, 22.f};
        if (bold_r.contains(p)) { bold_   = !bold_;   invalidate(); }
        if (ital_r.contains(p)) { italic_ = !italic_; invalidate(); }

        // Size box: clicking increments/decrements via keyboard only (see on_key_press)
    }

    void on_mouse_move(const MouseEvent& e) override {
        PointF p{float(e.pos.x), float(e.pos.y)};
        RectF lr{4.f,4.f,float(kListW),float(kListH)};
        if (lr.contains(p)) {
            int i = int(p.y - lr.y + float(scroll_y_)) / kItemH;
            if (i != hovered_) { hovered_ = i; invalidate(); }
        } else if (hovered_ >= 0) {
            hovered_ = -1; invalidate();
        }
    }

    void on_mouse_scroll(const MouseEvent& e) override {
        int max_scroll = std::max(0, kFontCount * kItemH - kListH);
        scroll_y_ = std::clamp(scroll_y_ - int(e.wheel.y) * kItemH, 0, max_scroll);
        invalidate();
    }

    void on_key_press(const KeyEvent& e) override {
        if (e.key == Key::Up) {
            font_size_ = std::max(6.f, font_size_ - 1.f); invalidate();
        } else if (e.key == Key::Down) {
            font_size_ = std::min(144.f, font_size_ + 1.f); invalidate();
        }
    }

private:
    int   family_idx_{0};
    float font_size_{13.f};
    bool  bold_{false}, italic_{false};
    int   scroll_y_{0};
    int   hovered_{-1};
};

} // namespace (font picker)

std::optional<FontDialogResult> choose_font(Window* parent,
                                            const Font& initial,
                                            bool show_color) {
    const int kW = FontPickerWidget::kRightX + 180;
    const int kPickH = FontPickerWidget::kListH + 20 + FontPickerWidget::kPreviewH + 4;
    const int kBtnH  = 36;
    const int kH     = kPickH + kBtnH + 12;

    WindowConfig cfg;
    cfg.title = "Choose Font";
    cfg.size  = {kW, kH};
    cfg.flags = WindowFlags::None;
    if (parent) {
        PointI pp = parent->position();
        SizeI  ps = parent->size();
        cfg.position = {pp.x+(ps.w-kW)/2, pp.y+(ps.h-kH)/2};
    }

    Window dlg{cfg};
    bool cancelled = false;

    auto* picker = dlg.root().emplace_child<FontPickerWidget>(initial, show_color);
    picker->set_bounds({0, 0, kW, kPickH});

    auto* ok_btn = dlg.root().emplace_child<Button>("OK");
    auto* cn_btn = dlg.root().emplace_child<Button>("Cancel");
    ok_btn->set_bounds({kW-162, kPickH+6, 72, 28});
    cn_btn->set_bounds({kW- 82, kPickH+6, 72, 28});

    (void)ok_btn->on_clicked.connect([&]{ dlg.close(); });
    (void)cn_btn->on_clicked.connect([&]{ cancelled = true; dlg.close(); });

    dlg.show();
    modal_loop([&]{ return dlg.is_closed(); }, &dlg);

    if (cancelled) return std::nullopt;
    return FontDialogResult{picker->cur_font(), picker->color};
}

} // namespace swole
