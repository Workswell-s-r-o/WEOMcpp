#ifndef CORE_IMAGECOLORIZATION_H
#define CORE_IMAGECOLORIZATION_H

#include "core/stream/imagedata.h"
#include "core/execution.h"

#include "core/misc/palette.h"

#include <cassert>
#include <future>
#include <vector>
#include <array>
#include <optional>


namespace wtilib
{
class Palette;
}

namespace core
{


class ImageColorization final
{
public:
    using PixelFormatConversionFunction = std::function<uint32_t(int, int, int, int)>;

    /*!
     * @brief getColorData is used to get colorured data out of the stream, regardless of the settings it serves as a top-level call
     * @param palette - what color to colourize the stream with, if none is given and you need it for colorizing MONO14/MONO8 default grey palette will be used
     * @param imageData - ImageData to colourize
     * @param pixelFormat - either ARGB or GBRA from this file, or provide your own pixel format to give, the input is in order Red-Green-Blue-Alpha
     * @param alpha - alpha part of the pixelFormat
     * @return std::vector<uint32_t>, uint32_t is in the format given
     */
    static std::future<std::vector<uint32_t>> getColorData(std::optional<core::Palette> palette, ImageData imageData, PixelFormatConversionFunction pixelFormat, int alpha);

    static constexpr uint32_t ARGB_PIXEL_FORMAT(int r, int g, int b, int a)
    {
        return (a << 24) | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
    }

    static constexpr uint32_t BGRA_PIXEL_FORMAT(int r, int g, int b, int a)
    {
        return (b << 24) | ((g & 0xffu) << 16) | ((r & 0xffu) << 8) | (a & 0xffu);
    }

    static ImageData mono8ColorizationWithPalette(const core::Palette& palette, const std::vector<uint8_t>& data);
    static ImageData mono14ColorizationWithPalette(const core::Palette& palette, const std::vector<uint8_t>& data);

private:
    static std::future<std::vector<uint32_t>> mono8ColorizationWithPaletteAsync(const core::Palette& palette, const std::vector<uint8_t>& data, PixelFormatConversionFunction pixelFormat, int alpha);
    static std::future<std::vector<uint32_t>> mono14ColorizationWithPaletteAsync(const core::Palette& palette, const std::vector<uint8_t>& rawData, PixelFormatConversionFunction pixelFormat, int alpha);
    static std::future<std::vector<uint32_t>> YUYV422ColorizationAsync(const std::vector<uint8_t>& data, PixelFormatConversionFunction pixelFormat, int alpha);
};

} //namespace core
#endif // CORE_IMAGECOLORIZATION_H
