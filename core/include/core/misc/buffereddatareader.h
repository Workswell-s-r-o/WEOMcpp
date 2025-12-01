#ifndef BUFFEREDDATAREADER_H
#define BUFFEREDDATAREADER_H

#include "core/misc/result.h"

#include <functional>
#include <span>
#include <cstdint>


namespace core
{

class BufferedDataReader
{
public:
    using ReadDataFunction = std::function<ValueResult<std::vector<uint8_t>> (uint32_t address)>;

    explicit BufferedDataReader(const ReadDataFunction& readDataFunction, uint32_t addressBegin, uint32_t addressEnd);

    ValueResult<std::span<const uint8_t>> getData(size_t requiredDataSize);

private:
    ReadDataFunction m_readDataFunction;
    const uint32_t m_addressEnd;

    uint32_t m_nextReadAddress {0};
    std::vector<uint8_t> m_data;
    std::span<const uint8_t> m_restOfData;
};

} // namespace core

#endif // BUFFEREDDATAREADER_H
