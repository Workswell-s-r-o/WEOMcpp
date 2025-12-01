#ifndef CORE_DEADPIXELS_H
#define CORE_DEADPIXELS_H

#include "core/misc/progresscontroller.h"
#include "core/misc/result.h"
#include "core/device.h"

#include <vector>
#include <string>
#include <optional>
#include <set>
#include <memory>
#include <span>


namespace core
{

struct PixelCoordinates
{
    unsigned row {0};
    unsigned column {0};

    auto operator<=>(const PixelCoordinates& other) const = default;

    std::string toString() const;

    unsigned getPixelIndex(unsigned width) const;
};


class DeadPixel
{
public:
    explicit DeadPixel() = default;
    explicit DeadPixel(const PixelCoordinates& coordinates);
    DeadPixel(const DeadPixel& other) = default;
    DeadPixel& operator=(const DeadPixel& other) = default;
    bool operator==(const DeadPixel& other) const = default;
    auto operator<=>(const DeadPixel& other) const = default;

    const PixelCoordinates& getCoordinates() const;

    const std::vector<PixelCoordinates>& getReplacements() const;
    bool removeReplacement(const PixelCoordinates& coordinates);
    bool addReplacement(const PixelCoordinates& coordinates);
    void clearReplacements();

    void writeToMemoryData(std::vector<uint8_t>& data, const core::Size& resolutionInPixels) const;

    static std::vector<uint8_t> serializeDeadPixels(const std::vector<DeadPixel>& deadPixels, const core::Size& resolutionInPixels);

    [[nodiscard]] static ValueResult<std::vector<DeadPixel>> deserializeDeadPixels(const std::function<ValueResult<std::span<const uint8_t>> (size_t bytes)>& getNextData,
                                                                                   const core::Size& resolutionInPixels,
                                                                                   ProgressTask progressTask = core::ProgressTask());

    static constexpr size_t MAX_REPLACEMENTS = 2;

private:
    PixelCoordinates m_coordinates;

    std::vector<PixelCoordinates> m_replacements;
};

class ReplacementPixel
{
public:
    explicit ReplacementPixel() = default;
    explicit ReplacementPixel(const PixelCoordinates& coordinates);

    bool operator==(const ReplacementPixel& other) const = default;

    const PixelCoordinates& getCoordinates() const;

    bool hasPixelIndexA() const;
    uint16_t getPixelIndexA() const;
    void setPixelIndexA(uint16_t index);
    void resetPixelIndexA();

    bool hasPixelIndexB() const;
    uint16_t getPixelIndexB() const;
    void setPixelIndexB(uint16_t index);
    void resetPixelIndexB();

    void writeToMemoryData(std::vector<uint8_t>& data, const core::Size& resolutionInPixels) const;

    static std::vector<uint8_t> serializeReplacements(const std::vector<ReplacementPixel>& replacements, const core::Size& resolutionInPixels);

    [[nodiscard]] static ValueResult<std::vector<ReplacementPixel>> deserializeReplacements(const std::function<ValueResult<std::span<const uint8_t>> (size_t bytes)>& getNextData,
                                                                                            const core::Size& resolutionInPixels,
                                                                                            ProgressTask progressTask = core::ProgressTask());

    static constexpr int MAX_REPLACED_PIXELS = 2;

private:
    PixelCoordinates m_coordinates;

    static std::vector<ReplacementPixel> deadPixelsSlotDecollision(std::vector<ReplacementPixel> replacementsList);

    std::optional<uint16_t> m_replacedPixelIndexA;
    std::optional<uint16_t> m_replacedPixelIndexB;
};


class DeadPixels
{
public:
    explicit DeadPixels();

    size_t getSize() const;
    const core::Size& getResolutionInPixels() const;

    using DeadPixelToReplacementsMap = std::map<PixelCoordinates, std::vector<PixelCoordinates>>;
    const DeadPixelToReplacementsMap& getDeadPixelToReplacementsMap() const;

    std::vector<DeadPixel> createDeadPixelsList() const;
    std::optional<DeadPixel> getDeadPixel(const PixelCoordinates& deadPixelCoordinates) const;
    bool containsDeadPixel(const PixelCoordinates& deadPixelCoordinates) const;

    bool erasePixel(const PixelCoordinates& deadPixelCoordinates);
    VoidResult insertPixel(const DeadPixel& deadPixel);

    void recomputeReplacements();

    std::vector<uint8_t> serializeDeadPixels() const;
    std::vector<uint8_t> serializeReplacements() const;

    [[nodiscard]] static ValueResult<DeadPixels> createDeadPixels(const std::vector<DeadPixel>& deadPixelsVector, const std::vector<ReplacementPixel>& replacementsVector);

    [[nodiscard]] VoidResult exportPixelsToCSV(const std::string& filename, bool withReplacements) const;
    [[nodiscard]] VoidResult importPixelsFromCSV(const std::string& filename, bool withReplacements);

    bool operator==(const DeadPixels& other) const = default;

private:
    std::optional<PixelCoordinates> getCoordinatesFromString(const std::string& columnValue, const std::string& rowValue);

    ValueResult<std::vector<ReplacementPixel>> createAndCheckReplacementsList(const DeadPixelToReplacementsMap& deadPixelToReplacements) const;

    static const std::string CSV_DELIMITER;
    static const std::vector<std::string> DEAD_PIXELS_LYNRED_HEADER;
    static const std::vector<std::string> DEAD_PIXELS_HEADER;
    static const std::vector<std::string> REPLACEMENTS_HEADER;

    core::Size m_resolutionInPixels;

    DeadPixelToReplacementsMap m_deadPixelToReplacementsMap;
};

} // namespace core

#endif // CORE_DEADPIXELS_H
