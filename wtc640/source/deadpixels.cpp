#include "core/wtc640/deadpixels.h"

#include "core/logging.h"
#include "core/utils.h"
#include "core/wtc640/hungariandeadpixels.h"
#include "core/wtc640/memoryspacewtc640.h"
#include "core/wtc640/devicewtc640.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <array>
#include <memory>
#include <set>
#include <fstream>


namespace core
{

const std::string DeadPixels::CSV_DELIMITER = ";";
const std::vector<std::string> DeadPixels::DEAD_PIXELS_LYNRED_HEADER {"Ligne", "Colonne"};
const std::vector<std::string> DeadPixels::DEAD_PIXELS_HEADER {"Row", "Column"};
const std::vector<std::string> DeadPixels::REPLACEMENTS_HEADER {"Replacement 1 Row", "Replacement 1 Column", "Replacement 2 Row", "Replacement 2 Column"};

std::string PixelCoordinates::toString() const
{
    return utils::format("[r:{}, c:{}]", row, column);
}

unsigned PixelCoordinates::getPixelIndex(unsigned width) const
{
    return row * width + column;
}

DeadPixel::DeadPixel(const PixelCoordinates& coordinates) :
    m_coordinates(coordinates)
{
}

const PixelCoordinates& DeadPixel::getCoordinates() const
{
    return m_coordinates;
}

const std::vector<PixelCoordinates>& DeadPixel::getReplacements() const
{
    return m_replacements;
}

bool DeadPixel::removeReplacement(const PixelCoordinates& coordinates)
{
    const auto it = std::find(m_replacements.begin(), m_replacements.end(), coordinates);
    if (it != m_replacements.end())
    {
        m_replacements.erase(it);

        return true;
    }

    return false;
}

bool DeadPixel::addReplacement(const PixelCoordinates& coordinates)
{
    if (getReplacements().size() >= DeadPixel::MAX_REPLACEMENTS)
    {
        assert(false);
        return false;
    }

    for (const auto& replacement : m_replacements)
    {
        if (replacement == coordinates)
        {
            return false;
        }
    }

    m_replacements.push_back(coordinates);
    std::sort(m_replacements.begin(), m_replacements.end());
    return true;
}

void DeadPixel::clearReplacements()
{
    m_replacements.clear();
}

void DeadPixel::writeToMemoryData(std::vector<uint8_t>& data, const core::Size& resolutionInPixels) const
{
    assert(getCoordinates().row < resolutionInPixels.height && getCoordinates().column < resolutionInPixels.width);
    const uint32_t pixelnum = getCoordinates().getPixelIndex(resolutionInPixels.width) + 1;

    data.push_back(static_cast<uint8_t>(pixelnum));
    data.push_back(static_cast<uint8_t>(pixelnum >> 8));
    data.push_back(static_cast<uint8_t>(pixelnum >> 16));
    data.push_back(static_cast<uint8_t>(pixelnum >> 24));
    static_assert(connection::MemorySpaceWtc640::DEADPIXEL_SIZE == 4);
}

std::vector<uint8_t> DeadPixel::serializeDeadPixels(const std::vector<DeadPixel>& deadPixels, const core::Size& resolutionInPixels)
{
    std::vector<uint8_t> deadPixelsData;
    deadPixelsData.reserve((deadPixels.size() + 1) * connection::MemorySpaceWtc640::DEADPIXEL_SIZE);

    for (const auto& deadPixel : deadPixels)
    {
        deadPixel.writeToMemoryData(deadPixelsData, resolutionInPixels);
    }

    // add terminate pixel to the end
    for (size_t i = 0; i < connection::MemorySpaceWtc640::DEADPIXEL_SIZE; ++i)
    {
        deadPixelsData.push_back(0);
    }

    return deadPixelsData;
}

ValueResult<std::vector<DeadPixel>> DeadPixel::deserializeDeadPixels(const std::function<ValueResult<std::span<const uint8_t>> (size_t)>& getNextData,
                                                                     const core::Size& resolutionInPixels,
                                                                     ProgressTask progressTask)
{
    using ResutType = ValueResult<std::vector<DeadPixel>>;
    auto createError = [](const std::string& detailErrorMessage)
    {
        return ResutType::createError("Invalid dead pixels data!", detailErrorMessage);
    };

    std::vector<DeadPixel> deadPixels;

    while (true)
    {
        if (progressTask.advanceByIsCancelled(1))
        {
            return createError("User cancelled");
        }

        const auto readResult = getNextData(connection::MemorySpaceWtc640::DEADPIXEL_SIZE);
        if (!readResult.isOk())
        {
            return ResutType::createFromError(readResult);
        }
        assert(readResult.getValue().size() >= connection::MemorySpaceWtc640::DEADPIXEL_SIZE);

        assert(3 < readResult.getValue().size());
        uint32_t pixelnum = 0;
        pixelnum += readResult.getValue()[3];
        pixelnum <<= 8;
        pixelnum += readResult.getValue()[2];
        pixelnum <<= 8;
        pixelnum += readResult.getValue()[1];
        pixelnum <<= 8;
        pixelnum += readResult.getValue()[0];

        if (pixelnum == 0)
        {
            break;
        }

        const PixelCoordinates coordinates{(pixelnum - 1) / resolutionInPixels.width, (pixelnum - 1) % resolutionInPixels.width};
        if (coordinates.column >= resolutionInPixels.width || coordinates.row >= resolutionInPixels.height)
        {
            return createError(utils::format("Invalid dead pixel pixelnum: {} {}", pixelnum, std::dec, coordinates.toString()));
        }

        deadPixels.push_back(DeadPixel(coordinates));
    }

    return deadPixels;
}

ReplacementPixel::ReplacementPixel(const PixelCoordinates& coordinates) :
    m_coordinates(coordinates)
{
}

const PixelCoordinates& ReplacementPixel::getCoordinates() const
{
    return m_coordinates;
}

bool ReplacementPixel::hasPixelIndexA() const
{
    return m_replacedPixelIndexA.has_value();
}

uint16_t ReplacementPixel::getPixelIndexA() const
{
    return m_replacedPixelIndexA.value();
}

void ReplacementPixel::setPixelIndexA(uint16_t index)
{
    m_replacedPixelIndexA = index;
}

void ReplacementPixel::resetPixelIndexA()
{
    m_replacedPixelIndexA.reset();
}

bool ReplacementPixel::hasPixelIndexB() const
{
    return m_replacedPixelIndexB.has_value();
}

uint16_t ReplacementPixel::getPixelIndexB() const
{
    return m_replacedPixelIndexB.value();
}

void ReplacementPixel::setPixelIndexB(uint16_t index)
{
    m_replacedPixelIndexB = index;
}

void ReplacementPixel::resetPixelIndexB()
{
    m_replacedPixelIndexB.reset();
}

void ReplacementPixel::writeToMemoryData(std::vector<uint8_t>& data, const core::Size& resolutionInPixels) const
{
    assert(getCoordinates().row < resolutionInPixels.height && getCoordinates().column < resolutionInPixels.width);
    const uint32_t pixelnum = getCoordinates().getPixelIndex(resolutionInPixels.width) + 1;

    const uint16_t replacedPixelIndexA = hasPixelIndexA() ? m_replacedPixelIndexA.value() : 0;
    const uint16_t replacedPixelIndexB = hasPixelIndexB() ? m_replacedPixelIndexB.value() : 0;

    data.push_back(static_cast<uint8_t>(replacedPixelIndexB));
    data.push_back(static_cast<uint8_t>(replacedPixelIndexB >> 8));

    uint8_t lower4bits = static_cast<uint8_t>(replacedPixelIndexA & 0x0F) << 4;
    if (hasPixelIndexB())
    {
        lower4bits |= 0b1;
    }
    data.push_back(lower4bits);

    const uint8_t middle8Bits = static_cast<uint8_t>(replacedPixelIndexA >> 4);
    data.push_back(middle8Bits);

    uint8_t upper4bits = static_cast<uint8_t>(replacedPixelIndexA >> 12);
    if (hasPixelIndexA())
    {
        upper4bits |= 0b1'0000;
    }
    data.push_back(upper4bits);

    data.push_back(static_cast<uint8_t>(pixelnum));
    data.push_back(static_cast<uint8_t>(pixelnum >> 8));
    data.push_back(static_cast<uint8_t>(pixelnum >> 16));
    static_assert(connection::MemorySpaceWtc640::DEADPIXEL_REPLACEMENT_SIZE == 8);
}

std::vector<ReplacementPixel> ReplacementPixel::deadPixelsSlotDecollision(std::vector<ReplacementPixel> replacementsList)
{
    assert(std::is_sorted(replacementsList.begin(), replacementsList.end(), [](ReplacementPixel& a, ReplacementPixel& b)
    {
        return a.getCoordinates() < b.getCoordinates();
    }));

    enum class DecollisionDirection
    {
        A,
        B,
    };

    std::map<int , std::vector<uint16_t>> deadPixelIndexToReplacementIndices;

    for (int replacementIndex = 0; replacementIndex < replacementsList.size(); replacementIndex++)
    {
        if (const auto& replacement = replacementsList.at(replacementIndex); replacement.hasPixelIndexA())
        {
            deadPixelIndexToReplacementIndices[replacement.getPixelIndexA()].push_back(replacementIndex);
        }

        if (const auto& replacement = replacementsList.at(replacementIndex); replacement.hasPixelIndexB())
        {
            deadPixelIndexToReplacementIndices[replacement.getPixelIndexB()].push_back(replacementIndex);
        }
    }

    std::vector<bool> isChainDecollided(replacementsList.size());

    auto chainDecollision = [&isChainDecollided, &deadPixelIndexToReplacementIndices](std::vector<ReplacementPixel>& replacementsList, uint16_t startIndex, DecollisionDirection decollisionDirection)
    {
        bool isTheDirectionB = (decollisionDirection == DecollisionDirection::B);

        if ((!isTheDirectionB && !replacementsList.at(startIndex).hasPixelIndexA())
            || (isTheDirectionB && !replacementsList.at(startIndex).hasPixelIndexB()))
        {
            return;
        }

        for (auto previousReplacement = replacementsList.at(startIndex); ; )
        {
            auto currentCandidatesIndices = isTheDirectionB ? deadPixelIndexToReplacementIndices.at(previousReplacement.getPixelIndexB()) : deadPixelIndexToReplacementIndices.at(previousReplacement.getPixelIndexA());
            if (currentCandidatesIndices.size() == 1)
            {
                assert(previousReplacement == replacementsList.at(currentCandidatesIndices.at(0)));
                break;
            }
            assert(currentCandidatesIndices.size() == 2);

            assert(previousReplacement == replacementsList.at(currentCandidatesIndices.at(0)) || previousReplacement == replacementsList.at(currentCandidatesIndices.at(1)));
            auto currentIndex = (previousReplacement == replacementsList.at(currentCandidatesIndices.at(0))) ? currentCandidatesIndices.at(1) : currentCandidatesIndices.at(0);

            isChainDecollided.at(currentIndex) = true;

            auto& currentReplacement = replacementsList.at(currentIndex);

            if (currentIndex == startIndex)
            {
                assert((isTheDirectionB ? currentReplacement : previousReplacement).hasPixelIndexA()
                       && (isTheDirectionB ? previousReplacement : currentReplacement).hasPixelIndexB()
                       && (isTheDirectionB ? previousReplacement : currentReplacement).getPixelIndexB() == (isTheDirectionB ? currentReplacement : previousReplacement).getPixelIndexA()
                       && "certain dead pixel closes the circle with slots already corrected");
                break;
            }
            else if(!currentReplacement.hasPixelIndexA())
            {
                if (isTheDirectionB)
                {
                    assert(currentReplacement.hasPixelIndexB());
                    assert(previousReplacement.getPixelIndexB() == currentReplacement.getPixelIndexB());
                    currentReplacement.setPixelIndexA(currentReplacement.getPixelIndexB());
                    currentReplacement.resetPixelIndexB();
                }
                assert(previousReplacement.getPixelIndexB() == currentReplacement.getPixelIndexA());
                break;
            }
            else if(!currentReplacement.hasPixelIndexB())
            {
                if (!isTheDirectionB)
                {
                    assert(currentReplacement.hasPixelIndexA());
                    assert(previousReplacement.getPixelIndexA() == currentReplacement.getPixelIndexA());
                    currentReplacement.setPixelIndexB(currentReplacement.getPixelIndexA());
                    currentReplacement.resetPixelIndexA();
                }
                assert(previousReplacement.getPixelIndexA() == currentReplacement.getPixelIndexB());
                break;
            }
            else if ((isTheDirectionB ? previousReplacement : currentReplacement).getPixelIndexB() != (isTheDirectionB ? currentReplacement : previousReplacement).getPixelIndexA())
            {
                assert((!isTheDirectionB && previousReplacement.getPixelIndexA() == currentReplacement.getPixelIndexA()) || (isTheDirectionB && previousReplacement.getPixelIndexB() == currentReplacement.getPixelIndexB()));
                auto temporary = currentReplacement.getPixelIndexA();
                currentReplacement.setPixelIndexA(currentReplacement.getPixelIndexB());
                currentReplacement.setPixelIndexB(temporary);
            }
            assert((!isTheDirectionB && previousReplacement.getPixelIndexA() == currentReplacement.getPixelIndexB()) || (isTheDirectionB && previousReplacement.getPixelIndexB() == currentReplacement.getPixelIndexA()));

            previousReplacement = currentReplacement;
        }
    };

    for (uint16_t replacementIndex = 0; replacementIndex < replacementsList.size(); replacementIndex++)
    {
        if (isChainDecollided.at(replacementIndex))
        {
            continue;
        }

        chainDecollision(replacementsList, replacementIndex, DecollisionDirection::A);
        chainDecollision(replacementsList, replacementIndex, DecollisionDirection::B);
    }

    return replacementsList;
};

std::vector<uint8_t> ReplacementPixel::serializeReplacements(const std::vector<ReplacementPixel>& replacements, const core::Size& resolutionInPixels)
{
    std::vector<uint8_t> replacementsData;
    replacementsData.reserve((replacements.size() + 1) * connection::MemorySpaceWtc640::DEADPIXEL_REPLACEMENT_SIZE);

    for (const auto& replacementPixel: deadPixelsSlotDecollision(replacements))
    {
        replacementPixel.writeToMemoryData(replacementsData, resolutionInPixels);
    }

    // add terminate replacement to the end
    for (size_t i = 0; i < connection::MemorySpaceWtc640::DEADPIXEL_REPLACEMENT_SIZE; ++i)
    {
        replacementsData.push_back(0);
    }

    return replacementsData;
}

ValueResult<std::vector<ReplacementPixel>> ReplacementPixel::deserializeReplacements(const std::function<ValueResult<std::span<const uint8_t>> (size_t)>& getNextData,
                                                                                     const core::Size& resolutionInPixels,
                                                                                     ProgressTask progressTask)
{
    using ResutType = ValueResult<std::vector<ReplacementPixel>>;
    auto createError = [](const std::string& detailErrorMessage)
    {
        return ResutType::createError("Invalid dead pixels data!", detailErrorMessage);
    };

    std::vector<ReplacementPixel> replacements;

    std::set<uint16_t> deadPixelIndicesWithSlotAUsed;
    std::set<uint16_t> deadPixelIndicesWithSlotBUsed;
    while (true)
    {
        if (progressTask.advanceByIsCancelled(1))
        {
            return createError("User cancelled");
        }

        const auto readResult = getNextData(connection::MemorySpaceWtc640::DEADPIXEL_REPLACEMENT_SIZE);
        if (!readResult.isOk())
        {
            return ResutType::createFromError(readResult);
        }
        assert(readResult.getValue().size() >= connection::MemorySpaceWtc640::DEADPIXEL_REPLACEMENT_SIZE);

        assert(7 < readResult.getValue().size());

        uint32_t pixelnum = 0;
        pixelnum += readResult.getValue()[7];
        pixelnum <<= 8;
        pixelnum += readResult.getValue()[6];
        pixelnum <<= 8;
        pixelnum += readResult.getValue()[5];

        if (pixelnum == 0)
        {
            break;
        }

        const PixelCoordinates coordinates{(pixelnum - 1) / resolutionInPixels.width, (pixelnum - 1) % resolutionInPixels.width};
        if (coordinates.column >= resolutionInPixels.width || coordinates.row >= resolutionInPixels.height)
        {
            return createError(utils::format("Invalid replacement pixelnum: {} {}", pixelnum, coordinates.toString()));
        }

        const uint8_t hasIndexAData = readResult.getValue()[4] >> 4;
        if (hasIndexAData != 0 && hasIndexAData != 1)
        {
            return createError(utils::format("Invalid replacement value - has index A: {}", hasIndexAData));
        }

        const uint8_t hasIndexBData = readResult.getValue()[2] & 0x0F;
        if (hasIndexBData != 0 && hasIndexBData != 1)
        {
            return createError(utils::format("Invalid replacement value - has index B: {}", hasIndexBData));
        }

        ReplacementPixel replacementPixel(coordinates);
        if (hasIndexAData != 0)
        {
            uint16_t indexA = 0;
            indexA += readResult.getValue()[4] & 0x0F;
            indexA <<= 8;
            indexA += readResult.getValue()[3];
            indexA <<= 4;
            indexA += readResult.getValue()[2] >> 4;
            if (deadPixelIndicesWithSlotAUsed.contains(indexA))
            {
                return createError("Invalid replacement - slot A of dead pixel used twice");
            }
            replacementPixel.setPixelIndexA(indexA);
            deadPixelIndicesWithSlotAUsed.insert(indexA);
        }

        if (hasIndexBData != 0)
        {
            uint16_t indexB = 0;
            indexB += readResult.getValue()[1];
            indexB <<= 8;
            indexB += readResult.getValue()[0];
            if (deadPixelIndicesWithSlotBUsed.contains(indexB))
            {
                return createError("Invalid replacement - slot B of dead pixel used twice");
            }
            replacementPixel.setPixelIndexB(indexB);
            deadPixelIndicesWithSlotBUsed.insert(indexB);
        }

        replacements.push_back(replacementPixel);
    }

    return replacements;
}

DeadPixels::DeadPixels() :
    m_resolutionInPixels(core::DevicesWtc640::WIDTH, core::DevicesWtc640::HEIGHT)
{
}

size_t DeadPixels::getSize() const
{
    return m_deadPixelToReplacementsMap.size();
}

const core::Size& DeadPixels::getResolutionInPixels() const
{
    return m_resolutionInPixels;
}

const DeadPixels::DeadPixelToReplacementsMap& DeadPixels::getDeadPixelToReplacementsMap() const
{
    return m_deadPixelToReplacementsMap;
}

std::vector<DeadPixel> DeadPixels::createDeadPixelsList() const
{
    std::vector<DeadPixel> deadPixelsList;
    deadPixelsList.reserve(m_deadPixelToReplacementsMap.size());

    for (const auto& [deadPixel, replacements] : m_deadPixelToReplacementsMap)
    {
        deadPixelsList.emplace_back(deadPixel);

        for (const auto& replacement : replacements)
        {
            deadPixelsList.back().addReplacement(replacement);
        }
    }

    return deadPixelsList;
}

std::optional<DeadPixel> DeadPixels::getDeadPixel(const PixelCoordinates& deadPixelCoordinates) const
{
    const auto it = m_deadPixelToReplacementsMap.find(deadPixelCoordinates);
    if (it == m_deadPixelToReplacementsMap.end())
    {
        return std::nullopt;
    }

    DeadPixel deadPixel(it->first);
    for (const auto& replacement : it->second)
    {
        VERIFY(deadPixel.addReplacement(replacement));
    }

    return deadPixel;
}

bool DeadPixels::containsDeadPixel(const PixelCoordinates& deadPixelCoordinates) const
{
    return m_deadPixelToReplacementsMap.find(deadPixelCoordinates) != m_deadPixelToReplacementsMap.end();
}

bool DeadPixels::erasePixel(const PixelCoordinates& deadPixelCoordinates)
{
    return (m_deadPixelToReplacementsMap.erase(deadPixelCoordinates) > 0);
}

VoidResult DeadPixels::insertPixel(const DeadPixel& newDeadPixel)
{
    for (const auto& newReplacementCoordinates : newDeadPixel.getReplacements())
    {
        if (containsDeadPixel(newReplacementCoordinates))
        {
            return VoidResult::createError("Dead pixel not found!");
        }
    }

    auto deadPixelToReplacements = m_deadPixelToReplacementsMap;
    deadPixelToReplacements.erase(newDeadPixel.getCoordinates());
    for (auto& [deadPixel, replacements] : deadPixelToReplacements)
    {
        const auto itEnd = std::remove(replacements.begin(), replacements.end(), newDeadPixel.getCoordinates());
        replacements.erase(itEnd, replacements.end());
    }
    assert(std::is_sorted(newDeadPixel.getReplacements().begin(), newDeadPixel.getReplacements().end()));
    auto& addedReplacements = deadPixelToReplacements.emplace(newDeadPixel.getCoordinates(), newDeadPixel.getReplacements()).first->second;
    std::sort(addedReplacements.begin(), addedReplacements.end());

    const auto newReplacements = createAndCheckReplacementsList(deadPixelToReplacements);
    if (!newReplacements.isOk())
    {
        return newReplacements.toVoidResult();
    }

    m_deadPixelToReplacementsMap = deadPixelToReplacements;
    return VoidResult::createOk();
}

void DeadPixels::recomputeReplacements()
{
    std::vector<SimplePixel> deadPixels;
    deadPixels.reserve(m_deadPixelToReplacementsMap.size());

    for (const auto& [deadPixel, replacements] : m_deadPixelToReplacementsMap)
    {
        deadPixels.emplace_back(deadPixel.row, deadPixel.column);
    }

    const auto newReplacements = hungarianDeadPixelsInstance(core::DevicesWtc640::HEIGHT, core::DevicesWtc640::WIDTH, deadPixels);
    size_t i = 0;
    for (auto& [deadPixel, replacements] : m_deadPixelToReplacementsMap)
    {
        replacements.clear();

        const auto& newReplacement = newReplacements.at(i);
        replacements.emplace_back(newReplacement.row, newReplacement.column);
        ++i;
    }

    const auto result = createAndCheckReplacementsList(m_deadPixelToReplacementsMap);
    if (!result.isOk())
    {
        WW_LOG_PROPERTIES_DEBUG << result.getGeneralErrorMessage() << result.getDetailErrorMessage();
    }
}

std::vector<uint8_t> DeadPixels::serializeDeadPixels() const
{
    const auto deadPixels = createDeadPixelsList();

    return DeadPixel::serializeDeadPixels(deadPixels, getResolutionInPixels());
}

std::vector<uint8_t> DeadPixels::serializeReplacements() const
{
    const auto replacementsResult = createAndCheckReplacementsList(m_deadPixelToReplacementsMap);
    assert(replacementsResult.isOk());

    return ReplacementPixel::serializeReplacements(replacementsResult.getValue(), getResolutionInPixels());
}

ValueResult<DeadPixels> DeadPixels::createDeadPixels(const std::vector<DeadPixel>& deadPixelsVector, const std::vector<ReplacementPixel>& replacementsVector)
{
    using ResutType = ValueResult<DeadPixels>;
    auto createError = [](const std::string& detailErrorMessage)
    {
        return ResutType::createError("Invalid dead pixels data!", detailErrorMessage);
    };

    std::vector<std::set<PixelCoordinates>> deadPixelReplacements(deadPixelsVector.size());
    for (const auto& replacement : replacementsVector)
    {
        if (replacement.hasPixelIndexA())
        {
            if (replacement.getPixelIndexA() >= deadPixelReplacements.size())
            {
                return createError(utils::format("Invalid replacement index A: {}", replacement.getPixelIndexA()));
            }

            deadPixelReplacements.at(replacement.getPixelIndexA()).insert(replacement.getCoordinates());
        }

        if (replacement.hasPixelIndexB())
        {
            if (replacement.getPixelIndexB() >= deadPixelReplacements.size())
            {
                return createError(utils::format("Invalid replacement index B: {}", replacement.getPixelIndexB()));
            }

            deadPixelReplacements.at(replacement.getPixelIndexB()).insert(replacement.getCoordinates());
        }
    }

    DeadPixelToReplacementsMap deadPixelToReplacements;

    for (size_t i = 0; i < deadPixelsVector.size(); ++i)
    {
        const auto& deadPixelReplacementsSet = deadPixelReplacements.at(i);
        if (deadPixelReplacementsSet.size() > DeadPixel::MAX_REPLACEMENTS)
        {
            return createError(utils::format("More than {} replacements for one pixel", DeadPixel::MAX_REPLACEMENTS));
        }

        std::vector<PixelCoordinates> replacements(deadPixelReplacementsSet.begin(), deadPixelReplacementsSet.end());
        if (!deadPixelToReplacements.emplace(deadPixelsVector.at(i).getCoordinates(), replacements).second)
        {
            return createError(utils::format("Coordinates duplicity: {}", deadPixelsVector.at(i).getCoordinates().toString()));
        }
    }

    DeadPixels deadPixels;

    const auto replacementsResult = deadPixels.createAndCheckReplacementsList(deadPixelToReplacements);
    if (!replacementsResult.isOk())
    {
        return createError(replacementsResult.getDetailErrorMessage());
    }

    deadPixels.m_deadPixelToReplacementsMap = deadPixelToReplacements;

    return deadPixels;
}

VoidResult DeadPixels::exportPixelsToCSV(const std::string& filename, bool withReplacements) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return VoidResult::createError("Error exporting dead pixels", "Unable to open file!");
    }

    file << utils::joinStringVector(DEAD_PIXELS_HEADER, CSV_DELIMITER);
    if (withReplacements)
    {
        file << CSV_DELIMITER << utils::joinStringVector(REPLACEMENTS_HEADER, CSV_DELIMITER);
    }
    file << std::endl;

    for (const auto& [deadPixel, replacements] : m_deadPixelToReplacementsMap)
    {
        file << (deadPixel.row + 1) << CSV_DELIMITER << (deadPixel.column + 1);

        if (withReplacements)
        {
            PixelCoordinates replacement1{};
            PixelCoordinates replacement2{};

            if (!replacements.empty())
            {
                replacement1 = replacements.at(0);
            }

            if (replacements.size() > 1)
            {
                static_assert(DeadPixel::MAX_REPLACEMENTS == 2);
                assert(replacements.size() == 2);
                replacement2 = replacements.at(1);

                file << CSV_DELIMITER << (replacement1.row + 1) << CSV_DELIMITER << (replacement1.column + 1)
                     << CSV_DELIMITER << (replacement2.row + 1) << CSV_DELIMITER << (replacement2.column + 1);
            }
            else
            {
                file << CSV_DELIMITER << (replacement1.row + 1) << CSV_DELIMITER << (replacement1.column + 1);
            }
        }

        file << std::endl;
    }

    return VoidResult::createOk();
}

VoidResult DeadPixels::importPixelsFromCSV(const std::string& filename, bool withReplacements)
{
    auto createError = [](const std::string& detailErrorMessage)
    {
        return VoidResult::createError("Error importing dead pixels", detailErrorMessage);
    };

    std::ifstream file(filename);
    if (!file.is_open())
    {
        return createError("Unable to open file!");
    }

    std::string firstLine;
    if (!std::getline(file, firstLine))
    {
        return createError("File is empty or unreadable!");
    }

    auto getFileDelimiter = [](const std::string& firstLine, int columnsCountNeeded)
    {
        for (const auto& delimiter : {CSV_DELIMITER, std::string(",")})
        {
            std::vector<std::string> header;
            boost::split(header, firstLine, boost::is_any_of(CSV_DELIMITER));

            std::vector<std::string> concat = DEAD_PIXELS_HEADER;
            concat.insert(concat.end(), REPLACEMENTS_HEADER.begin(), REPLACEMENTS_HEADER.end());
            if ((header == DEAD_PIXELS_LYNRED_HEADER || header == DEAD_PIXELS_HEADER || header == concat) &&
                header.size() >= columnsCountNeeded)
            {
                return delimiter;
            }
        }

        return std::string();
    };

    const int headerColumnsCountNeeded = DEAD_PIXELS_HEADER.size() + (withReplacements ? REPLACEMENTS_HEADER.size() : 0);
    const std::string fileDelimiter = getFileDelimiter(firstLine, headerColumnsCountNeeded);
    if (fileDelimiter.empty())
    {
        return createError("Invalid file header");
    }

    DeadPixelToReplacementsMap deadPixelToReplacements;

    std::string line;
    int row = 1;
    while (std::getline(file, line))
    {
        ++row;
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(fileDelimiter));

        // Trim any empty entries from the back
        while (!tokens.empty() && tokens.back().empty())
            tokens.pop_back();

        if (tokens.size() < DEAD_PIXELS_HEADER.size())
        {
            return createError(utils::format("row {}: expected min {} columns", row, DEAD_PIXELS_HEADER.size()));
        }

        auto coordinatesOpt = getCoordinatesFromString(tokens[1], tokens[0]);
        if (!coordinatesOpt.has_value())
        {
            return createError(utils::format("row {} columns 1,2: invalid coordinates", row));
        }

        std::vector<PixelCoordinates> replacements;

        if (withReplacements)
        {
            if (tokens.size() >= DEAD_PIXELS_HEADER.size() + 2)
            {
                auto replacementOpt = getCoordinatesFromString(tokens[3], tokens[2]);
                if (replacementOpt.has_value())
                {
                    replacements.push_back(replacementOpt.value());
                }
            }

            if (tokens.size() >= DEAD_PIXELS_HEADER.size() + 4)
            {
                auto replacementOpt = getCoordinatesFromString(tokens[5], tokens[4]);
                if (replacementOpt.has_value())
                {
                    replacements.push_back(replacementOpt.value());
                }
            }
        }

        std::sort(replacements.begin(), replacements.end());
        if (!deadPixelToReplacements.emplace(coordinatesOpt.value(), replacements).second)
        {
            return createError(utils::format("row {}: dead pixel coordinates duplicity", row));
        }
    }

    const auto newReplacements = createAndCheckReplacementsList(deadPixelToReplacements);
    if (!newReplacements.isOk())
    {
        return createError(newReplacements.getDetailErrorMessage());
    }

    m_deadPixelToReplacementsMap = deadPixelToReplacements;

    if (!withReplacements)
    {
        recomputeReplacements();
    }

    return VoidResult::createOk();
}

std::optional<PixelCoordinates> DeadPixels::getCoordinatesFromString(const std::string& columnValue, const std::string& rowValue)
{
    PixelCoordinates coordinates;

    try
    {
        coordinates.row = std::stoi(rowValue);
        coordinates.column = std::stoi(columnValue);
    }
    catch (...)
    {
        return std::nullopt;
    }

    if ((coordinates.column == 0) || (coordinates.row == 0))
    {
        return std::nullopt;
    }

    coordinates.row--;
    coordinates.column--;

    if (coordinates.row >= core::DevicesWtc640::HEIGHT || coordinates.column >= core::DevicesWtc640::WIDTH)
    {
        return std::nullopt;
    }

    return coordinates;
}

ValueResult<std::vector<ReplacementPixel>> DeadPixels::createAndCheckReplacementsList(const DeadPixelToReplacementsMap& deadPixelToReplacements) const
{
    using ResultType = ValueResult<std::vector<ReplacementPixel>>;

    auto validateCoordinates = [](const PixelCoordinates& coordinates)
    {
        if (coordinates.column >= core::DevicesWtc640::WIDTH || coordinates.row >= core::DevicesWtc640::HEIGHT)
        {
            return VoidResult::createError("Invalid coordinates!", coordinates.toString());
        }

        return VoidResult::createOk();
    };

    std::map<PixelCoordinates, std::vector<uint16_t>> replacementToDeadPixelIndices;

    auto deadPixelIt = deadPixelToReplacements.cbegin();
    for (uint16_t deadPixelIndex = 0; deadPixelIndex < deadPixelToReplacements.size(); ++deadPixelIndex, ++deadPixelIt)
    {
        const auto deadPixelCoordinatesResult = validateCoordinates(deadPixelIt->first);
        if (!deadPixelCoordinatesResult.isOk())
        {
            return ResultType::createFromError(deadPixelCoordinatesResult);
        }

        auto pixelReplacementsCoordinates = deadPixelIt->second;
        if(!std::is_sorted(pixelReplacementsCoordinates.begin(), pixelReplacementsCoordinates.end()))
        {
            return ResultType::createError("Invalid replacement!", "replacements of dead pixels not sorted");
        }

        if (pixelReplacementsCoordinates.size() > DeadPixel::MAX_REPLACEMENTS)
        {
            return ResultType::createError("Invalid replacements!", utils::format("too many replacements: {} max allowed: {}", pixelReplacementsCoordinates.size(), DeadPixel::MAX_REPLACEMENTS));
        }

        for (const auto& replacement : pixelReplacementsCoordinates)
        {
            const auto replacementCoordinatesResult = validateCoordinates(replacement);
            if (!replacementCoordinatesResult.isOk())
            {
                return ResultType::createFromError(replacementCoordinatesResult);
            }

            if (deadPixelToReplacements.find(replacement) != deadPixelToReplacements.end())
            {
                return ResultType::createError("Invalid replacement!", utils::format("replacement cannot use dead pixel: {}", replacement.toString()));
            }
            
            replacementToDeadPixelIndices[replacement].push_back(deadPixelIndex);
        }
    }

    std::vector<ReplacementPixel> replacementsList;
    replacementsList.reserve(replacementToDeadPixelIndices.size());

    for (const auto& [replacement, deadPixelIndices] : replacementToDeadPixelIndices)
    {
        if (deadPixelIndices.size() > ReplacementPixel::MAX_REPLACED_PIXELS)
        {
            return ResultType::createError("Invalid replacements!", utils::format("replacement for too many ded pixels: {} max allowed: {}", deadPixelIndices.size(), ReplacementPixel::MAX_REPLACED_PIXELS));
        }

        replacementsList.emplace_back(replacement);
        if (!deadPixelIndices.empty())
        {
            replacementsList.back().setPixelIndexA(deadPixelIndices.at(0));
        }
        if (deadPixelIndices.size() > 1)
        {
            replacementsList.back().setPixelIndexB(deadPixelIndices.at(1));
        }
    }

    return replacementsList;
}

} // namespace core
