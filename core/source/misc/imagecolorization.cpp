#include "core/misc/imagecolorization.h"

#include "core/misc/palette.h"
#include "core/stream/imagedata.h"
#include "core/execution.h"

#include <boost/range/irange.hpp>

#include <future>

namespace core
{
namespace
{
template<typename SetResultFunction>
void mono8ColorizationWithPaletteImpl(const core::Palette& palette, const std::vector<uint8_t>& data, SetResultFunction&& setFn)
{
    const auto idxRange = boost::irange(std::size_t(0), data.size());
    std::for_each(STD_EXECUTION_PAR_UNSEQ idxRange.begin(), idxRange.end(), [&](const std::size_t idx)
    {
        const auto& colors = palette.getRgb()[data[idx]];
        setFn(idx, colors[core::Palette::INDEX_R], colors[core::Palette::INDEX_G], colors[core::Palette::INDEX_B]);
    });
}
} // namespace

ImageData ImageColorization::mono8ColorizationWithPalette(const core::Palette& palette, const std::vector<uint8_t>& data)
{
    ImageData result{ImageData::Type::RGB};
    result.data.resize(data.size() * core::Palette::COLOR_COMPONENT_COUNT);
    mono8ColorizationWithPaletteImpl(palette, data, [&](const std::size_t idx, const int r, const int g, const int b)
    {
        const auto i = core::Palette::COLOR_COMPONENT_COUNT * idx;
        result.data[i] = r;
        result.data[i + 1] = g;
        result.data[i + 2] = b;
    });
    return result;
}

namespace
{
template<typename SetResultFunction>
void mono14ColorizationWithPaletteImpl(const core::Palette& palette, const std::vector<uint8_t>& data, SetResultFunction&& setFn)
{
    const auto idxRange = boost::irange(std::size_t(0), data.size() / 2);

    uint16_t min = std::numeric_limits<uint16_t>::max();
    uint16_t max = 0;
    for (const auto idx : idxRange)
    {
        const uint16_t combined = static_cast<uint16_t>(data[idx * 2]) | (static_cast<uint16_t>(data[idx * 2 + 1]) << 8);
        if (combined < min) min = combined;
        if (combined > max) max = combined;
    }

    const uint32_t range = max - min;
    assert(range != 0);

    std::for_each(STD_EXECUTION_PAR_UNSEQ idxRange.begin(), idxRange.end(), [&](const std::size_t idx)
    {
        const uint16_t combined = static_cast<uint16_t>(data[idx * 2]) | (static_cast<uint16_t>(data[idx * 2 + 1]) << 8);

        float normalized = float(combined - min) / float(range);
        uint8_t index = static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));

        const auto& colors = palette.getRgb()[index];
        setFn(idx, colors[core::Palette::INDEX_R], colors[core::Palette::INDEX_G], colors[core::Palette::INDEX_B]);
    });
}
} // namespace

ImageData ImageColorization::mono14ColorizationWithPalette(const core::Palette& palette, const std::vector<uint8_t>& data)
{
    ImageData result{ImageData::Type::RGB};
    result.data.resize(data.size() / 2 * core::Palette::COLOR_COMPONENT_COUNT);
    mono14ColorizationWithPaletteImpl(palette, data, [&](const std::size_t idx, const int r, const int g, const int b)
    {
        const auto i = core::Palette::COLOR_COMPONENT_COUNT * idx;
        result.data[i] = r;
        result.data[i + 1] = g;
        result.data[i + 2] = b;
    });
    return result;
}

std::future<std::vector<uint32_t>> ImageColorization::getColorData(std::optional<core::Palette> palette, ImageData imageData, PixelFormatConversionFunction pixelFormat, int alpha)
{
    switch (imageData.type)
    {
    case ImageData::Type::Raw14Bit:
        assert(palette.has_value());
        return mono14ColorizationWithPaletteAsync(palette.has_value() ? palette.value() : core::Palette(), imageData.data, pixelFormat, alpha);
    case ImageData::Type::PaletteIndices:
        assert(palette.has_value());
        return mono8ColorizationWithPaletteAsync(palette.has_value() ? palette.value() : core::Palette(), imageData.data, pixelFormat, alpha);
    case ImageData::Type::YUYV422:
        assert(palette.has_value());
        return YUYV422ColorizationAsync(imageData.data, pixelFormat, alpha);
    case ImageData::Type::RGB:
        assert(false && "Not implemented!");
    }
    return {};
}

std::future<std::vector<uint32_t>> core::ImageColorization::mono8ColorizationWithPaletteAsync(const core::Palette& palette, const std::vector<uint8_t>& data, PixelFormatConversionFunction pixelFormat, int alpha)
{
    return std::async(std::launch::async, [=]()
    {
        std::vector<uint32_t> result(data.size());
        mono8ColorizationWithPaletteImpl(palette, data, [&](const std::size_t idx, const int r, const int g, const int b)
        {
            result[idx] = pixelFormat(r, g, b, alpha);
        });
        return result;
    });
}

std::future<std::vector<uint32_t>> core::ImageColorization::mono14ColorizationWithPaletteAsync(const core::Palette& palette, const std::vector<uint8_t>& rawData, PixelFormatConversionFunction pixelFormat, int alpha)
{
    return std::async(std::launch::async, [=]()
    {
        std::vector<uint32_t> result(rawData.size() / 2);
        mono14ColorizationWithPaletteImpl(palette, rawData, [&](const std::size_t idx, const int r, const int g, const int b)
        {
            result[idx] = pixelFormat(r, g, b, alpha);
        });
        return result;
    });
}

namespace
{
static constexpr std::array<uint8_t, 3> CONVERT_YUV_TO_RGB(uint8_t y, uint8_t u, uint8_t v)
{
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;

    auto clamp = [](int value) -> uint8_t {
        return static_cast<uint8_t>(value < 0 ? 0 : (value > 255 ? 255 : value));
    };

    uint8_t r = clamp((298 * c + 409 * e + 128) >> 8);
    uint8_t g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
    uint8_t b = clamp((298 * c + 516 * d + 128) >> 8);

    return {r, g, b};
}
} // namespace

std::future<std::vector<uint32_t>> core::ImageColorization::YUYV422ColorizationAsync(const std::vector<uint8_t>& byteData, PixelFormatConversionFunction pixelFormat, int alpha)
{
    return std::async(std::launch::async, [=]()
    {
        std::vector<uint32_t> result(byteData.size() / 2);
        const auto idxRange = boost::irange(std::size_t(0), result.size() / 2);
        std::for_each(STD_EXECUTION_PAR_UNSEQ idxRange.begin(), idxRange.end(), [&](const std::size_t pairIdx)
        {
            uint8_t y = byteData[pairIdx * 4];
            uint8_t u = byteData[pairIdx * 4 + 1];
            uint8_t v = byteData[pairIdx * 4 + 3];

            auto rgb = CONVERT_YUV_TO_RGB(y, u, v);
            result[pairIdx * 2] = pixelFormat(rgb[core::Palette::INDEX_R], rgb[core::Palette::INDEX_G], rgb[core::Palette::INDEX_B], alpha);

            y = byteData[pairIdx * 4 + 2];
            rgb = CONVERT_YUV_TO_RGB(y, u, v);
            result[pairIdx * 2 + 1] = pixelFormat(rgb[core::Palette::INDEX_R], rgb[core::Palette::INDEX_G], rgb[core::Palette::INDEX_B], alpha);
        });

        return result;
    });
}

} // namespace core
