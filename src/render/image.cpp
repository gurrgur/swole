#include "swole/render/image.hpp"

#include "include/core/SkImage.h"
#include "include/core/SkData.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColorType.h"
#include "include/encode/SkPngEncoder.h"

#include <fstream>
#include <vector>

namespace swole {

struct Image::Impl {
    sk_sp<SkImage> image;
};

Image::~Image() = default;
Image::Image(const Image&) = default;
Image& Image::operator=(const Image&) = default;
Image::Image(Image&&) noexcept = default;
Image& Image::operator=(Image&&) noexcept = default;

Image Image::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    auto data_bytes = std::vector<char>(std::istreambuf_iterator<char>(file), {});
    auto sk_data = SkData::MakeWithCopy(data_bytes.data(), data_bytes.size());
    auto sk_img  = SkImages::DeferredFromEncodedData(std::move(sk_data));

    Image img;
    if (sk_img) {
        img.impl_ = std::make_shared<Impl>();
        img.impl_->image = std::move(sk_img);
    }
    return img;
}

Image Image::from_pixels(int width, int height, ImageFormat format,
                          std::span<const uint8_t> data, int row_bytes) {
    SkColorType ct;
    switch (format) {
    case ImageFormat::RGBA8: ct = kRGBA_8888_SkColorType; break;
    case ImageFormat::BGRA8: ct = kBGRA_8888_SkColorType; break;
    case ImageFormat::A8:    ct = kAlpha_8_SkColorType;   break;
    case ImageFormat::RGB8:
    case ImageFormat::BGR8:  ct = kRGBA_8888_SkColorType; break;
    default:                 ct = kRGBA_8888_SkColorType; break;
    }

    SkImageInfo info = SkImageInfo::Make(width, height, ct, kPremul_SkAlphaType);
    int bytes_per_pixel = (format == ImageFormat::A8) ? 1
                        : (format == ImageFormat::RGB8 || format == ImageFormat::BGR8) ? 3
                        : 4;
    int rb = (row_bytes > 0) ? row_bytes : width * bytes_per_pixel;

    sk_sp<SkData> sk_data;
    if (format == ImageFormat::RGB8 || format == ImageFormat::BGR8) {
        std::vector<uint8_t> rgba(size_t(width) * size_t(height) * 4);
        for (int y = 0; y < height; ++y) {
            const uint8_t* src = data.data() + size_t(y) * size_t(rb);
            uint8_t* dst = rgba.data() + size_t(y) * size_t(width) * 4;
            for (int x = 0; x < width; ++x) {
                dst[x * 4 + 0] = format == ImageFormat::RGB8 ? src[x * 3 + 0] : src[x * 3 + 2];
                dst[x * 4 + 1] = src[x * 3 + 1];
                dst[x * 4 + 2] = format == ImageFormat::RGB8 ? src[x * 3 + 2] : src[x * 3 + 0];
                dst[x * 4 + 3] = 255;
            }
        }
        sk_data = SkData::MakeWithCopy(rgba.data(), rgba.size());
        rb = width * 4;
    } else {
        sk_data = SkData::MakeWithCopy(data.data(), data.size());
    }

    auto sk_img  = SkImages::RasterFromData(info, std::move(sk_data), rb);

    Image img;
    if (sk_img) {
        img.impl_ = std::make_shared<Impl>();
        img.impl_->image = std::move(sk_img);
    }
    return img;
}

bool  Image::is_valid() const { return impl_ && impl_->image != nullptr; }
int   Image::width()    const { return impl_ ? impl_->image->width()  : 0; }
int   Image::height()   const { return impl_ ? impl_->image->height() : 0; }
RectF Image::bounds()   const { return {0, 0, float(width()), float(height())}; }

std::vector<uint8_t> Image::encode_png() const {
    if (!is_valid()) return {};
    auto data = SkPngEncoder::Encode(nullptr, impl_->image.get(), {});
    if (!data) return {};
    return std::vector<uint8_t>(static_cast<const uint8_t*>(data->data()),
                                static_cast<const uint8_t*>(data->data()) + data->size());
}

void* Image::native_handle() const {
    return impl_ ? impl_->image.get() : nullptr;
}

} // namespace swole
