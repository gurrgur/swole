#pragma once

#include "../core/types.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace swole {

enum class ImageFormat { RGBA8, RGB8, BGRA8, BGR8, A8 };
enum class ScaleFilter { Nearest, Linear, Cubic };

// Immutable GPU/CPU image. Wraps SkImage (pimpl).
class Image {
public:
    Image() = default;
    ~Image();

    Image(const Image&);
    Image& operator=(const Image&);
    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;

    // Load from disk (JPEG, PNG, WebP, BMP, …)
    static Image load(const std::filesystem::path& path);

    // Encode from raw pixel data
    static Image from_pixels(int width, int height, ImageFormat format,
                             std::span<const uint8_t> data, int row_bytes = 0);

    [[nodiscard]] bool  is_valid()  const;
    [[nodiscard]] int   width()     const;
    [[nodiscard]] int   height()    const;
    [[nodiscard]] SizeI size()      const { return {width(), height()}; }
    [[nodiscard]] RectF bounds()    const;

    // Encode to PNG bytes
    [[nodiscard]] std::vector<uint8_t> encode_png() const;

    [[nodiscard]] void* native_handle() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_; // shared so copies are cheap
};

} // namespace swole
