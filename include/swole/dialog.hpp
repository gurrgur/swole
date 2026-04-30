#pragma once

#include "window/window.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace swole {

// ── Message box ──────────────────────────────────────────────────────────────

enum class MessageBoxIcon    { None, Info, Warning, Error, Question };
enum class MessageBoxButtons { Ok, OkCancel, YesNo, YesNoCancel, RetryCancel };
enum class MessageBoxResult  { Ok, Cancel, Yes, No, Retry };

MessageBoxResult message_box(Window*           parent,
                             std::string_view  title,
                             std::string_view  message,
                             MessageBoxButtons buttons = MessageBoxButtons::Ok,
                             MessageBoxIcon    icon    = MessageBoxIcon::None);

// ── File dialogs ─────────────────────────────────────────────────────────────

struct FileFilter {
    std::string name;       // e.g. "Audio files"
    std::vector<std::string> extensions; // e.g. {"wav", "aiff", "flac"}
};

struct OpenFileOptions {
    std::string              title{"Open File"};
    std::filesystem::path    initial_dir;
    std::filesystem::path    initial_file;
    std::vector<FileFilter>  filters;
    bool                     allow_multiple{false};
};

struct SaveFileOptions {
    std::string              title{"Save File"};
    std::filesystem::path    initial_dir;
    std::filesystem::path    initial_file;
    std::vector<FileFilter>  filters;
    bool                     confirm_overwrite{true};
};

// Returns selected paths, or empty on cancel.
std::vector<std::filesystem::path> open_file_dialog(Window* parent, OpenFileOptions opts = {});
std::optional<std::filesystem::path> save_file_dialog(Window* parent, SaveFileOptions opts = {});
std::optional<std::filesystem::path> choose_directory(Window* parent,
                                                       std::string_view title = "Choose Folder",
                                                       std::filesystem::path initial = {});

// ── Color picker ──────────────────────────────────────────────────────────────

std::optional<Color> choose_color(Window* parent,
                                  Color initial = Color::white(),
                                  bool show_alpha = false);

// ── Font picker ───────────────────────────────────────────────────────────────

struct FontDialogResult {
    Font    font;
    Color   color;
};

std::optional<FontDialogResult> choose_font(Window* parent,
                                            const Font& initial = Font{},
                                            bool show_color = false);

} // namespace swole
