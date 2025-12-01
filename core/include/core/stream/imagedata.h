#ifndef CORE_IMAGEDATA_H
#define CORE_IMAGEDATA_H

#include <vector>
#include <cstdint>

namespace core
{
class ImageData final
{
public:
    enum class Type
    {
        Raw14Bit,
        PaletteIndices,
        YUYV422,
        RGB,
    };

public:
    explicit ImageData(Type t) : type(t) {}

    bool operator==(const ImageData&) const = default;
    bool operator!=(const ImageData&) const = default;

public:
    Type type;
    std::vector<uint8_t> data;
};
} // namespace core

#endif // CORE_IMAGEDATA_H
