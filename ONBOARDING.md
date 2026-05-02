# Swole — Contributor Onboarding Guide

Welcome to **swole**, a modern C++ Win32/SWELL-compatible UI library backed by SDL3 + Skia. This guide will get you productive quickly.

---

## What is Swole?

Swole is a drop-in replacement for [WDL/SWELL](https://github.com/justinfrankel/WDL), the cross-platform compatibility layer used by REAPER and other Cockos products. Our goal is **ABI compatibility** with SWELL so that REAPER can use `libSwell.so` from this project without any code changes.

Behind the scenes, swole uses a clean, modern C++23 widget framework with SDL3 for windowing/input and Skia (GPU) for rendering.

---

## Architecture at a Glance

```
REAPER (or any SWELL consumer)
        ↓  dlopen("libSwell.so"), SWELLAPI_GetFunc("...")
┌───────────────────────────────────────────┐
│  SWELL ABI Layer (C-style, extern "C")     │
│  swell_dispatch.cpp  —  name→func table    │
│  swell_core.cpp      —  platform utils     │
│  swell_window.cpp    —  HWND/window APIs   │
│  swell_messages.cpp  —  SendMessage/PostMsg│
│  swell_menu.cpp      —  HMENU/WM_COMMAND   │
│  swell_widget_host.cpp —  Widget→HWND bridge│
│  swell_handles.hpp   —  HWND__/HMENU__/HDC__│
├───────────────────────────────────────────┤
│  Modern C++ Widget Core (swole::)          │
│  Widget / Window / Canvas / Layout / …     │
├───────────────────────────────────────────┤
│  Backend: SDL3 (windowing, events)         │
│           Skia  (GPU rendering)            │
└───────────────────────────────────────────┘
```

### Key Design Principle

**HWND/HDC/HMENU are opaque handles backed by small C structs.** The real work happens in typed C++ objects (`swole::Widget`, `swole::Window`, `swole::Canvas`). The SWELL layer translates Win32-ish calls into clean C++ method calls.

```
HWND  →  HWND__ shell  →  swole::Window or swole::Widget
HMENU →  HMENU__ shell →  menu model
HDC   →  HDC__ shell   →  temporary swole::Canvas adapter
```

---

## Directory Structure

```
include/swole/          Public C++ API
  core/                 Application, types (Color, Rect, Point), events
  render/               Canvas, Font, Image, Paint (Skia-backed)
  window/               Widget base class, Window
  widgets/              Button, Label, ListView, TreeView, etc.
  layout/               BoxLayout, FormLayout, GridLayout
  menu.hpp, dialog.hpp  Menu bar, color/font dialogs

src/
  core/                 Application implementation
  render/               Canvas/Font/Image/Paint implementations
  window/               Widget/Window implementations
  widgets/              Widget implementations
  layout/               Layout implementations
  swell/                ★ SWELL compatibility layer ★
    swell_abi.hpp       C-compatible types (HWND, RECT, BOOL, etc.)
    swell_dispatch.cpp  SWELLAPI_GetFunc dispatch table
    swell_core.cpp      Platform utilities (INI, timers, etc.)
    swell_handles.hpp   HWND__/HMENU__/HDC__ backing structs
    swell_widget_host.* Widget that bridges swole events → HWND messages
    swell_window.cpp    CreateWindowEx, geometry, focus, capture, etc.
    swell_messages.cpp  SendMessage, PostMessage, timers
    swell_menu.cpp      HMENU model, TrackPopupMenu, WM_COMMAND routing

docs/
  swell-compat-todo.md  ★ What REAPER needs — our todo list
  chatgpt-cpp-to-c.md   Architecture design rationale
  chatgpt-integration-plan.md  Step-by-step integration guide

testing/reaper_linux_x86_64/  REAPER binary for testing
```

---

## Building

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### Testing with REAPER

```bash
cp ./build/libSwell.so testing/reaper_linux_x86_64/REAPER/libSwell.so
testing/reaper_linux_x86_64/REAPER/reaper > log.txt 2>&1
```

Check `log.txt` for `swole libSwell: unresolved SWELL API stubbed: ...` messages — these tell you which APIs REAPER is calling that aren't yet implemented.

---

## How the SWELL Dispatch Works

1. REAPER calls `SWELLAPI_GetFunc("SomeFunction")`
2. We search `kApiTable[]` in `swell_dispatch.cpp` for a matching name
3. Each entry is either `API_IMPL(name)` (real implementation) or `API_STUB(name)` (returns 0, logs once)
4. To implement a stub: write the function in the appropriate `.cpp` file, add a forward declaration in `swell_dispatch.cpp`, and change `API_STUB` → `API_IMPL`

---

## Adding a New SWELL API

1. **Write the implementation** in the appropriate file:
   - Window/control APIs → `swell_window.cpp`
   - Message APIs → `swell_messages.cpp`
   - Menu APIs → `swell_menu.cpp`
   - GDI/drawing APIs → new `swell_gdi.cpp` (not yet created)

2. **Add a forward declaration** in `swell_dispatch.cpp` inside the `extern "C"` block

3. **Change `API_STUB` → `API_IMPL`** in the dispatch table

4. **Rebuild and test** with REAPER

---

## Code Conventions

- **C++23**, no extensions
- Modern C++ in the `swole::` namespace: `unique_ptr`, `std::span`, `std::string_view`, structured bindings, etc.
- **C-style** at the SWELL boundary: `extern "C"`, raw pointers as handles, no exceptions across the ABI
- **No emojis** in code
- **No comments** unless asked (the code should be self-documenting)
- Compiler warnings: `-Wall -Wextra -Wpedantic -Wno-unused-parameter`

---

## The Handle Bridge Pattern

Every SWELL handle (`HWND`, `HMENU`, `HDC`) is a pointer to a struct with a magic number for validation:

```cpp
struct HWND__ {
    uint32_t magic{kHwndMagic};
    // ... parent/children, window/widget pointers, style, text, etc.
};

inline HWND__* as_hwnd(HWND h) {
    auto* p = reinterpret_cast<HWND__*>(h);
    return p && p->magic == kHwndMagic && !p->destroyed ? p : nullptr;
}
```

The `HwndWidget` class (in `swell_widget_host.cpp`) is a `swole::Widget` subclass that translates modern C++ events (`on_paint`, `on_mouse_press`, etc.) into Win32 messages (`WM_PAINT`, `WM_LBUTTONDOWN`, etc.) via `swell_send_message()`.

---

## Priority Areas

See `docs/swell-compat-todo.md` for the full list. The highest-impact items for REAPER compatibility are:

1. **HWND lifecycle** — `CreateWindowEx`, `DestroyWindow`, `ShowWindow` ✅ done
2. **Geometry** — `GetWindowRect`, `GetClientRect`, `SetWindowPos` ✅ done
3. **Messages** — `SendMessage`, `PostMessage`, `DefWindowProc` ✅ done
4. **Menus** — `CreatePopupMenu`, `TrackPopupMenu`, `WM_COMMAND` routing ✅ done
5. **WindowLong/Props** — `Get/SetWindowLong`, `Get/SetProp` ✅ done
6. **ListView** — 30+ stubs, heavily used by REAPER
7. **TreeView** — 15+ stubs
8. **GDI/Drawing** — `BeginPaint`, `BitBlt`, `SWELL_FillRect`, fonts, brushes
9. **Clipboard** — `OpenClipboard`, `Get/SetClipboardData`
10. **Threads/Events** — `CreateThread`, `CreateEvent`, `WaitForSingleObject`

Items marked ✅ are implemented; the rest are stubs that log when called.

---

## Useful Commands

```bash
# Full rebuild
cmake -S . -B build -G Ninja && cmake --build build --clean-first

# See which APIs are still stubbed
grep API_STUB src/swell/swell_dispatch.cpp | wc -l

# Run REAPER and capture stub warnings
testing/reaper_linux_x86_64/REAPER/reaper 2>&1 | grep "stubbed"
```
