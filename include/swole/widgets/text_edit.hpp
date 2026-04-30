#pragma once

#include "../window/widget.hpp"
#include "../render/font.hpp"
#include <functional>
#include <string>

namespace swole {

enum class TextEditMode { SingleLine, MultiLine, Password };

class TextEdit : public Widget {
public:
    explicit TextEdit(Widget* parent = nullptr);

    // Content
    void set_text(std::string_view text);
    [[nodiscard]] const std::string& text() const { return text_; }

    // Selection (byte offsets into UTF-8 text)
    void select_all();
    void select_range(int start, int end);
    void clear_selection();
    [[nodiscard]] bool has_selection() const { return sel_start_ != sel_end_; }
    [[nodiscard]] int  selection_start() const { return sel_start_; }
    [[nodiscard]] int  selection_end()   const { return sel_end_; }
    [[nodiscard]] std::string selected_text() const;

    // Cursor
    void set_cursor_pos(int pos);
    [[nodiscard]] int cursor_pos() const { return cursor_pos_; }

    // Editing config
    void set_mode(TextEditMode mode);
    void set_max_length(int n) { max_length_ = n; }
    void set_read_only(bool ro) { read_only_ = ro; }
    void set_placeholder(std::string_view text) { placeholder_ = text; invalidate(); }

    void set_font(Font font);
    void set_text_color(Color c) { text_color_ = c; invalidate(); }
    void set_selection_color(Color c) { sel_color_ = c; invalidate(); }

    [[nodiscard]] WidgetSizeHint size_hint() const override;

    // Clipboard ops
    void cut();
    void copy() const;
    void paste();

    // Signals
    Signal<>                   on_text_changed;
    Signal<std::string_view>   on_editing_finished;  // Return / focus-out
    Signal<>                   on_return_pressed;

protected:
    void on_paint(Canvas& canvas) override;
    void on_key_press(const KeyEvent& e) override;
    void on_text_input(const TextInputEvent& e) override;
    void on_mouse_press(const MouseEvent& e) override;
    void on_mouse_move(const MouseEvent& e) override;
    void on_focus_gain(const FocusEvent& e) override;
    void on_focus_loss(const FocusEvent& e) override;

private:
    void insert_at_cursor(std::string_view s);
    void delete_selection();
    int  pos_from_point(PointF p) const;
    RectF cursor_rect() const;

    std::string   text_;
    std::string   placeholder_;
    Font          font_;
    Color         text_color_{Color::black()};
    Color         sel_color_{Color::from_rgba32(0x3399FF80)};
    TextEditMode  mode_{TextEditMode::SingleLine};
    int           cursor_pos_{0};
    int           sel_start_{0}, sel_end_{0};
    int           scroll_offset_{0};   // pixels scrolled horizontally
    int           max_length_{-1};     // -1 = unlimited
    bool          read_only_{false};
    bool          cursor_blink_{false};
    uint32_t      cursor_timer_id_{0};
};

} // namespace swole
