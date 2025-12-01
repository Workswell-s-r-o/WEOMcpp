#ifndef CORE_PALETTE_H
#define CORE_PALETTE_H

#include <string>
#include <vector>
#include <array>

#include <cstdint>

namespace core
{

class Palette
{
public:
    static constexpr int SIZE = 256;
    static constexpr int COLOR_COMPONENT_COUNT = 3;

    static constexpr size_t INDEX_R = 0;
    static constexpr size_t INDEX_G = 1;
    static constexpr size_t INDEX_B = 2;

    static constexpr size_t INDEX_Y  = 0;
    static constexpr size_t INDEX_CB = 1;
    static constexpr size_t INDEX_CR = 2;

    using ColorData = std::array<std::array<uint8_t, COLOR_COMPONENT_COUNT>, SIZE>;

    Palette();
    Palette(const std::string& name, const ColorData& rgb);

    Palette(const Palette&) = default;
    Palette(Palette&&) = default;
    Palette& operator=(const Palette&) = default;
    Palette& operator=(Palette&&) = default;

    bool operator==(const Palette& other) const;
    bool operator!=(const Palette& other) const;

    const std::string& getName() const;
    void setName(const std::string& name);

    const ColorData& getRgb() const;
    void setRgb(const ColorData& rgb);

    const ColorData& getYCbCr() const;
    void setYCbCr(const ColorData& yCbCr);

    static bool readFromPltFile(const std::string& path, Palette& palette);
    static bool readFromString(const std::string& string, Palette& palette);
    static bool readFromHexFile(const std::string& path, Palette& palette);
    static bool readFromHex(std::vector<uint8_t> array, Palette& palette);
    static bool saveAsPltFile(const std::string& path, const Palette& palette);

    static void convertRGBtoYCbCr(const ColorData& rgbValues, ColorData& YCbCrValues);
    static void convertYCbCrtoRGB(const ColorData& YCbCrValues, ColorData& rgbValues);

    static const std::string DEFAULT_PALETTE_NAME;

private:
    static const char PLT_FILE_DELIMITER;

    std::string m_name;
    ColorData m_rgb;
    ColorData m_ycbcr;
};

} // namespace wtilib

#endif // WTILIB_PALETTE_H
