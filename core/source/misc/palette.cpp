#include "core/misc/palette.h"

#include <cmath>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <filesystem>

namespace core
{

const std::string Palette::DEFAULT_PALETTE_NAME = "gray";
const char Palette::PLT_FILE_DELIMITER = ';';

Palette::Palette() :
    m_name(DEFAULT_PALETTE_NAME)
{
    for(size_t colorIndex = 0; colorIndex < Palette::SIZE; ++colorIndex)
    {
        for (size_t i = 0; i < Palette::COLOR_COMPONENT_COUNT; ++i)
        {
            m_rgb[colorIndex][i] = static_cast<int>(colorIndex);
        }
    }

    convertRGBtoYCbCr(m_rgb, m_ycbcr);
}

Palette::Palette(const std::string& name, const ColorData& rgb) :
    m_name(name),
    m_rgb(rgb)
{
    convertRGBtoYCbCr(m_rgb, m_ycbcr);
}

bool Palette::operator==(const Palette& other) const
{
    return m_name == other.m_name && m_rgb == other.m_rgb;
}

bool Palette::operator!=(const Palette& other) const
{
    return !(*this == other);
}

const std::string& Palette::getName() const
{
    return m_name;
}

void Palette::setName(const std::string& name)
{
    m_name = name;
}

const Palette::ColorData& Palette::getRgb() const
{
    return m_rgb;
}

void Palette::setRgb(const ColorData& rgb)
{
    m_rgb = rgb;

    convertRGBtoYCbCr(m_rgb, m_ycbcr);
}

const Palette::ColorData& Palette::getYCbCr() const
{
    return m_ycbcr;
}

void Palette::setYCbCr(const ColorData& yCbCr)
{
    m_ycbcr = yCbCr;

    convertYCbCrtoRGB(m_ycbcr, m_rgb);
}

bool Palette::readFromPltFile(const std::string& path, Palette& palette)
{
    std::filesystem::path file(path);
    std::ifstream paletteFile(file);
    if (!paletteFile.is_open())
    {
        return false;
    }

    std::string data = {std::istreambuf_iterator<char>(paletteFile), std::istreambuf_iterator<char>()};

    palette.setName(file.stem().string());

    return readFromString(data, palette);
}

bool Palette::readFromString(const std::string& input, Palette& palette)
{
    std::istringstream stream(input);
    ColorData rgb;
    size_t colorIndex = 0;
    std::string line;

    while (std::getline(stream, line))
    {
        std::vector<std::string> components;
        size_t start = 0;
        size_t end = line.find(PLT_FILE_DELIMITER);

        while (end != std::string::npos)
        {
            std::string token = line.substr(start, end - start);
            if (!token.empty())
                components.push_back(token);
            start = end + 1;
            end = line.find(PLT_FILE_DELIMITER, start);
        }

        std::string lastToken = line.substr(start);
        if (!lastToken.empty())
            components.push_back(lastToken);

        if (components.size() != Palette::COLOR_COMPONENT_COUNT || colorIndex >= Palette::SIZE)
        {
            return false;
        }

        for (unsigned componentIndex = 0; componentIndex < Palette::COLOR_COMPONENT_COUNT; ++componentIndex)
        {
            try
            {
                int value = std::stoi(components[componentIndex]);
                if (value < 0 || value >= 256)
                {
                    return false;
                }
                rgb[colorIndex][componentIndex] = static_cast<uint8_t>(value);
            }
            catch (...)
            {
                return false;
            }
        }

        ++colorIndex;
    }

    if (colorIndex != palette.getRgb().size())
    {
        return false;
    }

    palette.setRgb(rgb);
    return true;
}

bool Palette::readFromHexFile(const std::string& path, Palette& palette)
{
    std::filesystem::path file(path);
    std::ifstream paletteFile(file);

    if (!paletteFile.is_open())
    {
        return false;
    }
    if(std::filesystem::file_size(file) != Palette::SIZE*Palette::COLOR_COMPONENT_COUNT)
    {
        return false;
    }

    std::vector<uint8_t> data = {std::istreambuf_iterator<char>(paletteFile), std::istreambuf_iterator<char>()};

    palette.setName(file.stem().string());

    return readFromHex(data, palette);
}

bool Palette::readFromHex(std::vector<uint8_t> array, Palette& palette)
{
    if(array.size() != Palette::SIZE*Palette::COLOR_COMPONENT_COUNT)
    {
        return false;
    }

    ColorData rgb;
    for(size_t i = 0; i < Palette::SIZE; i++)
    {
        for (size_t componentIndex = 0; componentIndex < Palette::COLOR_COMPONENT_COUNT; ++componentIndex)
        {
            rgb[i][componentIndex] = array[i*3+componentIndex];
        }
    }
    palette.setRgb(rgb);
    return true;
}

bool Palette::saveAsPltFile(const std::string& path, const Palette& palette)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    for (const auto& color : palette.getRgb())
    {
        for (size_t colorComponentIndex = 0; colorComponentIndex < color.size(); ++colorComponentIndex)
        {
            if (colorComponentIndex > 0)
            {
                file << PLT_FILE_DELIMITER;
            }

            file << static_cast<int>(color.at(colorComponentIndex));
        }

        file << std::endl;
    }

    return true;
}

void Palette::convertRGBtoYCbCr(const ColorData& rgbValues, ColorData& YCbCrValues)
{
    for (size_t colorIndex = 0; colorIndex < SIZE; ++colorIndex)
    {
        const int y  = static_cast<int>(std::round( 0.299   * rgbValues[colorIndex][INDEX_R] + 0.587    * rgbValues[colorIndex][INDEX_G] + 0.114   * rgbValues[colorIndex][INDEX_B]));
        const int cb = static_cast<int>(std::round(-0.16874 * rgbValues[colorIndex][INDEX_R] - 0.33126  * rgbValues[colorIndex][INDEX_G] + 0.5     * rgbValues[colorIndex][INDEX_B])) + 128;
        const int cr = static_cast<int>(std::round( 0.5     * rgbValues[colorIndex][INDEX_R] - 0.418689 * rgbValues[colorIndex][INDEX_G] - 0.08131 * rgbValues[colorIndex][INDEX_B])) + 128;

        YCbCrValues[colorIndex][INDEX_Y]  = std::clamp(y , 0, SIZE - 1);
        YCbCrValues[colorIndex][INDEX_CB] = std::clamp(cb, 0, SIZE - 1);
        YCbCrValues[colorIndex][INDEX_CR] = std::clamp(cr, 0, SIZE - 1);
    }
}

void Palette::convertYCbCrtoRGB(const ColorData& YCbCrValues, ColorData& rgbValues)
{
    for (int colorIndex = 0; colorIndex < SIZE; ++colorIndex)
    {
        const int r = static_cast<int>(std::round(YCbCrValues[colorIndex][INDEX_Y] + 1.40200 * (YCbCrValues[colorIndex][INDEX_CR] - 128)));
        const int g = static_cast<int>(std::round(YCbCrValues[colorIndex][INDEX_Y] - 0.34414 * (YCbCrValues[colorIndex][INDEX_CB] - 128) - 0.71414 * (YCbCrValues[colorIndex][INDEX_CR] - 128)));
        const int b = static_cast<int>(std::round(YCbCrValues[colorIndex][INDEX_Y] + 1.77200 * (YCbCrValues[colorIndex][INDEX_CB] - 128)));

        rgbValues[colorIndex][INDEX_R] = std::clamp(r, 0, SIZE - 1);
        rgbValues[colorIndex][INDEX_G] = std::clamp(g, 0, SIZE - 1);
        rgbValues[colorIndex][INDEX_B] = std::clamp(b, 0, SIZE - 1);
    }
}
} // namespace wtilib
