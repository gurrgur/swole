#include "swole/widgets/text_edit.hpp"
#include "swole/core/application.hpp"
#include "swole/render/canvas.hpp"
#include "swole/render/paint.hpp"
#include "../theme.hpp"

#include <algorithm>

namespace swole {

static constexpr int kPad = 6; // horizontal inner padding in pixels

// ── UTF-8 helpers ─────────────────────────────────────────────────────────────

static int utf8_next(std::string_view s, int pos) {
    if (pos >= int(s.size())) return int(s.size());
    ++pos;
    while (pos < int(s.size()) && (uint8_t(s[pos]) & 0xC0) == 0x80) ++pos;
    return pos;
}
static int utf8_prev(std::string_view s, int pos) {
    if (pos <= 0) return 0;
    --pos;
    while (pos > 0 && (uint8_t(s[pos]) & 0xC0) == 0x80) --pos;
    return pos;
}

// ── construction ─────────────────────────────────────────────────────────────

TextEdit::TextEdit(Widget* parent) : Widget(parent), font_(theme::default_font()) {
    set_focus_policy(FocusPolicy::Click);
}

// ── public API ───────────────────────────────────────────────────────────────

void TextEdit::set_text(std::string_view text) {
    text_      = text;
    cursor_pos_ = int(text_.size());
    sel_start_ = sel_end_ = cursor_pos_;
    scroll_offset_ = 0;
    on_text_changed.emit();
    invalidate();
}

void TextEdit::select_all() {
    sel_start_  = 0;
    cursor_pos_ = sel_end_ = int(text_.size());
    invalidate();
}

void TextEdit::select_range(int start, int end) {
    sel_start_  = std::clamp(start, 0, int(text_.size()));
    sel_end_    = std::clamp(end,   0, int(text_.size()));
    cursor_pos_ = sel_end_;
    invalidate();
}

void TextEdit::clear_selection() {
    sel_start_ = sel_end_ = cursor_pos_;
    invalidate();
}

std::string TextEdit::selected_text() const {
    if (sel_start_ == sel_end_) return {};
    int lo = std::min(sel_start_, sel_end_);
    int hi = std::max(sel_start_, sel_end_);
    return text_.substr(lo, hi - lo);
}

void TextEdit::set_cursor_pos(int pos) {
    cursor_pos_ = std::clamp(pos, 0, int(text_.size()));
    sel_start_  = sel_end_ = cursor_pos_;
    invalidate();
}

void TextEdit::set_mode(TextEditMode mode) { mode_ = mode; invalidate(); }
void TextEdit::set_font(Font f)            { font_ = std::move(f); invalidate(); }

WidgetSizeHint TextEdit::size_hint() const {
    auto m = font_.metrics();
    int h = int(m.line_height()) + kPad * 2;
    return {.min_size = {60, h}, .preferred_size = {200, h}};
}

// ── clipboard ────────────────────────────────────────────────────────────────

void TextEdit::cut() {
    if (read_only_) return;
    Application::instance().set_clipboard_text(selected_text());
    delete_selection();
    on_text_changed.emit();
    invalidate();
}

void TextEdit::copy() const {
    Application::instance().set_clipboard_text(selected_text());
}

void TextEdit::paste() {
    if (read_only_) return;
    insert_at_cursor(Application::instance().clipboard_text());
}

// ── private helpers ───────────────────────────────────────────────────────────

// Display text: for password mode replace every code-point with '*'.
static std::string display_text(const std::string& t, TextEditMode mode) {
    if (mode != TextEditMode::Password) return t;
    std::string out;
    for (int i = 0; i < int(t.size()); i = utf8_next(t, i))
        out += '*';
    return out;
}

void TextEdit::insert_at_cursor(std::string_view s) {
    delete_selection();
    if (max_length_ >= 0 && int(text_.size() + s.size()) > max_length_)
        s = s.substr(0, size_t(max_length_) - text_.size());
    text_.insert(size_t(cursor_pos_), s);
    cursor_pos_ += int(s.size());
    sel_start_  = sel_end_ = cursor_pos_;
    on_text_changed.emit();
    ensure_cursor_visible();
    invalidate();
}

void TextEdit::delete_selection() {
    if (sel_start_ == sel_end_) return;
    int lo = std::min(sel_start_, sel_end_);
    int hi = std::max(sel_start_, sel_end_);
    text_.erase(lo, hi - lo);
    cursor_pos_ = sel_start_ = sel_end_ = lo;
}

int TextEdit::pos_from_point(PointF p) const {
    std::string disp = display_text(text_, mode_);
    float origin_x = float(kPad) - float(scroll_offset_);
    int best_pos = 0;
    float best_dist = 1e9f;
    for (int i = 0; i <= int(disp.size()); ) {
        float gx = origin_x + font_.measure_text_width(disp.substr(0, i));
        float dist = std::abs(gx - p.x);
        if (dist < best_dist) { best_dist = dist; best_pos = i; }
        if (i == int(disp.size())) break;
        i = utf8_next(disp, i);
    }
    return best_pos;
}

RectF TextEdit::cursor_rect() const {
    std::string disp = display_text(text_, mode_);
    auto m = font_.metrics();
    float x = float(kPad) - float(scroll_offset_)
              + font_.measure_text_width(disp.substr(0, cursor_pos_));
    float h = m.ascent + m.descent;
    float y = (float(size().h) - h) * 0.5f;
    return {x - 1.f, y, 2.f, h};
}

void TextEdit::ensure_cursor_visible() {
    std::string disp = display_text(text_, mode_);
    float cx = font_.measure_text_width(disp.substr(0, cursor_pos_));
    float inner_w = float(size().w) - float(kPad) * 2;
    if (cx - float(scroll_offset_) < 0.f)
        scroll_offset_ = int(cx);
    else if (cx - float(scroll_offset_) > inner_w)
        scroll_offset_ = int(cx - inner_w);
    scroll_offset_ = std::max(0, scroll_offset_);
}

// ── cursor blink ─────────────────────────────────────────────────────────────

void TextEdit::start_blink() {
    cursor_blink_ = true;
    rearm_blink();
}

void TextEdit::rearm_blink() {
    Application::instance().cancel_delayed(cursor_timer_id_);
    cursor_timer_id_ = Application::instance().post_delayed(530, [this] {
        if (!has_focus()) return;
        cursor_blink_ = !cursor_blink_;
        invalidate();
        rearm_blink();
    });
}

void TextEdit::stop_blink() {
    Application::instance().cancel_delayed(cursor_timer_id_);
    cursor_blink_ = false;
}

// ── paint ─────────────────────────────────────────────────────────────────────

void TextEdit::on_paint(Canvas& canvas) {
    RectF r{local_bounds()};
    theme::draw_widget_bg(canvas, r, has_focus());

    auto guard = canvas.scoped_save();
    canvas.clip_rect(r.inset(float(kPad), 1.f));

    std::string disp = display_text(text_, mode_);
    auto m     = font_.metrics();
    float base_x = float(kPad) - float(scroll_offset_);
    float base_y = (r.h - m.ascent - m.descent) * 0.5f + m.ascent;

    // Selection highlight
    if (sel_start_ != sel_end_) {
        int lo = std::min(sel_start_, sel_end_);
        int hi = std::max(sel_start_, sel_end_);
        float x0 = base_x + font_.measure_text_width(disp.substr(0, lo));
        float x1 = base_x + font_.measure_text_width(disp.substr(0, hi));
        canvas.draw_rect({x0, 2.f, x1 - x0, r.h - 4.f}, Paint::fill(sel_color_));
    }

    // Text or placeholder
    if (disp.empty() && !placeholder_.empty()) {
        canvas.draw_text(placeholder_, base_x, base_y, font_,
                         Paint::fill(theme::placeholder));
    } else {
        canvas.draw_text(disp, base_x, base_y, font_,
                         Paint::fill(is_enabled() ? text_color_ : theme::text_disabled));
    }

    // Cursor
    if (has_focus() && !read_only_ && cursor_blink_) {
        RectF cr = cursor_rect();
        canvas.draw_rect(cr, Paint::fill(text_color_));
    }
}

// ── events ────────────────────────────────────────────────────────────────────

void TextEdit::on_key_press(const KeyEvent& e) {
    bool shift = e.mods.shift;
    bool ctrl  = e.mods.ctrl || e.mods.meta;

    // Reshow cursor on any keystroke
    cursor_blink_ = true;
    rearm_blink();

    auto move_to = [&](int new_pos, bool extend) {
        if (!extend) sel_start_ = new_pos;
        cursor_pos_ = sel_end_ = new_pos;
        ensure_cursor_visible();
        invalidate();
    };

    switch (e.key) {
    case Key::Left:
        move_to(ctrl ? 0 : utf8_prev(text_, cursor_pos_), shift);
        break;
    case Key::Right:
        move_to(ctrl ? int(text_.size()) : utf8_next(text_, cursor_pos_), shift);
        break;
    case Key::Home:
        move_to(0, shift);
        break;
    case Key::End:
        move_to(int(text_.size()), shift);
        break;

    case Key::Backspace:
        if (read_only_) break;
        if (sel_start_ != sel_end_) {
            delete_selection();
        } else if (cursor_pos_ > 0) {
            int prev = utf8_prev(text_, cursor_pos_);
            text_.erase(prev, cursor_pos_ - prev);
            cursor_pos_ = sel_start_ = sel_end_ = prev;
        }
        on_text_changed.emit();
        ensure_cursor_visible();
        invalidate();
        break;

    case Key::Delete:
        if (read_only_) break;
        if (sel_start_ != sel_end_) {
            delete_selection();
        } else if (cursor_pos_ < int(text_.size())) {
            int next = utf8_next(text_, cursor_pos_);
            text_.erase(cursor_pos_, next - cursor_pos_);
        }
        on_text_changed.emit();
        invalidate();
        break;

    case Key::Return:
        if (mode_ == TextEditMode::MultiLine)
            insert_at_cursor("\n");
        else {
            on_return_pressed.emit();
            on_editing_finished.emit(text_);
        }
        break;

    case Key::A: if (ctrl) select_all(); break;
    case Key::C: if (ctrl) copy(); break;
    case Key::V: if (ctrl) paste(); break;
    case Key::X: if (ctrl) cut(); break;

    default: break;
    }
}

void TextEdit::on_text_input(const TextInputEvent& e) {
    if (read_only_) return;
    insert_at_cursor(e.text);
    cursor_blink_ = true;
    rearm_blink();
}

void TextEdit::on_mouse_press(const MouseEvent& e) {
    if (e.button != MouseButton::Left) return;
    mouse_selecting_ = true;
    int pos = pos_from_point(PointF{float(e.pos.x), float(e.pos.y)});
    if (e.mods.shift) {
        sel_end_ = cursor_pos_ = pos;
    } else {
        cursor_pos_ = sel_start_ = sel_end_ = pos;
    }
    cursor_blink_ = true;
    rearm_blink();
    invalidate();
}

void TextEdit::on_mouse_release(const MouseEvent& e) {
    if (e.button == MouseButton::Left) mouse_selecting_ = false;
}

void TextEdit::on_mouse_move(const MouseEvent& e) {
    if (!mouse_selecting_) return;
    int pos = pos_from_point(PointF{float(e.pos.x), float(e.pos.y)});
    sel_end_ = cursor_pos_ = pos;
    ensure_cursor_visible();
    invalidate();
}

void TextEdit::on_focus_gain(const FocusEvent& e) {
    Widget::on_focus_gain(e);
    start_blink();
}

void TextEdit::on_focus_loss(const FocusEvent& e) {
    Widget::on_focus_loss(e);
    stop_blink();
    on_editing_finished.emit(text_);
}

} // namespace swole
