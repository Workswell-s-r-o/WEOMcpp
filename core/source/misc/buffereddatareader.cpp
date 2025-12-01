#include "core/misc/buffereddatareader.h"

#include <cassert>


namespace core
{

BufferedDataReader::BufferedDataReader(const ReadDataFunction& readDataFunction, uint32_t addressBegin, uint32_t addressEnd) :
    m_readDataFunction(readDataFunction),
    m_addressEnd(addressEnd),
    m_nextReadAddress(addressBegin)
{
}

ValueResult<std::span<const uint8_t>> BufferedDataReader::getData(size_t requiredDataSize)
{
    using ResultType = ValueResult<std::span<const uint8_t>>;

    while (requiredDataSize > m_restOfData.size())
    {
        const size_t minSizeToRead = requiredDataSize - m_restOfData.size();
        if (m_nextReadAddress + minSizeToRead > m_addressEnd)
        {
            return ResultType::createError("Read error!", "Unexpected end of memory");
        }

        m_data.erase(m_data.begin(), m_data.begin() + m_data.size() - m_restOfData.size());
        m_restOfData = m_data;

        const auto readResult = m_readDataFunction(m_nextReadAddress);
        if (!readResult.isOk())
        {
            return ResultType::createFromError(readResult);
        }

        m_nextReadAddress += readResult.getValue().size();
        assert(m_nextReadAddress <= m_addressEnd);
        m_data.insert(m_data.end(), readResult.getValue().begin(), readResult.getValue().end());
        m_restOfData = m_data;
    }

    assert(m_restOfData.size() >= requiredDataSize);
    const auto data = m_restOfData.first(requiredDataSize);
    m_restOfData = m_restOfData.last(m_restOfData.size() - data.size());

    return data;
}

} // namespace core
