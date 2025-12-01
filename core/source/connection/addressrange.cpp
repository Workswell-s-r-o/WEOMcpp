#include "core/connection/addressrange.h"

#include "core/utils.h"

#include <charconv>

namespace core
{

namespace connection
{

bool AddressRange::contains(uint32_t address) const
{
    return address >= m_firstAddress && address <= m_lastAddress;
}

bool AddressRange::contains(const AddressRange& other) const
{
    return contains(other.m_firstAddress) && contains(other.m_lastAddress);
}

bool AddressRange::overlaps(const AddressRange& other) const
{
    return other.m_firstAddress <= m_lastAddress && other.m_lastAddress >= m_firstAddress;
}

std::string AddressRange::toHexString() const
{
    return utils::format("[{} - {}]", addressToHexString(getFirstAddress()), addressToHexString(getLastAddress()));
}

std::string AddressRange::addressToHexString(uint32_t address)
{
    return utils::format("0x{}", utils::justifyRight(utils::numberToHex(address, false), '0', 8));
}

ValueResult<AddressRange> AddressRange::fromHexString(const std::string& address, uint32_t size)
{
    constexpr std::string_view ERROR_MESSAGE{"Failed to parse addressrange!"};
    using Result = ValueResult<AddressRange>;

    std::string cleaned;
    cleaned.reserve(address.size());
    std::copy_if(address.begin(), address.end(), std::back_inserter(cleaned), [](char c)
    {
        return c != '_';
    });

    if (cleaned.size() < 2 || cleaned.front() != '0' || std::tolower(static_cast<unsigned char>(cleaned[1])) != 'x')
    {
        return Result::createError(std::string(ERROR_MESSAGE), "address range does not start with 0x");
    }

    uint32_t startAddress{};
    const char* const begin = cleaned.data() + 2;
    const char* const end   = cleaned.data() + cleaned.size();

    auto [ptr, ec] = std::from_chars(begin, end, startAddress, 16);
    if (ec != std::errc{} || ptr != end)
    {
        return Result::createError(std::string(ERROR_MESSAGE), "address range failed to convert to uint32_t");
    }

    return AddressRange::firstAndSize(startAddress, size);}

AddressRanges::AddressRanges(const AddressRange& range) :
    m_addressRanges({range})
{
}

AddressRanges::AddressRanges(const std::vector<AddressRange>& ranges)
{
    setRanges(ranges);
}

AddressRanges::AddressRanges(const AddressRanges& addressRanges1, const AddressRanges& addressRanges2)
{
    auto ranges = addressRanges1.getRanges();
    ranges.insert(ranges.end(), addressRanges2.getRanges().begin(), addressRanges2.getRanges().end());

    setRanges(ranges);
}

const std::vector<AddressRange>& AddressRanges::getRanges() const
{
    return m_addressRanges;
}

bool AddressRanges::overlaps(const AddressRanges& other) const
{
    for (auto itThis = m_addressRanges.begin(), itOther = other.m_addressRanges.begin();
         itThis != m_addressRanges.end() && itOther != other.m_addressRanges.end(); )
    {
        if (itThis->getLastAddress() < itOther->getFirstAddress())
        {
            ++itThis;
        }
        else if (itOther->getLastAddress() < itThis->getFirstAddress())
        {
            ++itOther;
        }
        else
        {
            if (itThis->overlaps(*itOther))
            {
                return true;
            }

            if (itThis->getFirstAddress() < itOther->getFirstAddress())
            {
                ++itThis;
            }
            else if (itOther->getFirstAddress() < itThis->getFirstAddress())
            {
                ++itOther;
            }
            else
            {
                ++itThis;
                ++itOther;
            }
        }
    }

    return false;
}

bool AddressRanges::contains(const AddressRanges& other) const
{
    if (m_addressRanges.empty() || other.m_addressRanges.empty())
    {
        return false;
    }

    for (auto itThis = m_addressRanges.begin(), itOther = other.m_addressRanges.begin();
         itOther != other.m_addressRanges.end(); )
    {
        if (itThis == m_addressRanges.end())
        {
            return false;
        }

        if (itThis->getLastAddress() < itOther->getFirstAddress())
        {
            ++itThis;
        }
        else if (itOther->getLastAddress() < itThis->getFirstAddress())
        {
            return false;
        }
        else
        {
            if (itThis->contains(*itOther))
            {
                ++itOther;
            }
            else
            {
                ++itThis;
            }
        }
    }

    return true;
}

bool AddressRanges::operator<(const AddressRanges& other) const
{
    if (m_addressRanges.size() != other.m_addressRanges.size())
    {
        return (m_addressRanges.size() < other.m_addressRanges.size());
    }

    for (auto itThis = m_addressRanges.begin(), itOther = other.m_addressRanges.begin();
         itThis != m_addressRanges.end(); ++itThis, ++itOther)
    {
        if (*itThis != *itOther)
        {
            return (*itThis < *itOther);
        }
    }

    return false;
}

void AddressRanges::setRanges(const std::vector<AddressRange>& ranges)
{
    m_addressRanges = ranges;

    std::sort(m_addressRanges.begin(), m_addressRanges.end());

    for (size_t i = 1; i < m_addressRanges.size(); )
    {
        auto itNext = m_addressRanges.begin() + i;
        auto itPrevious = itNext - 1;
        if (itPrevious->contains(*itNext))
        {
            m_addressRanges.erase(itNext);
        }
        else if (itPrevious->overlaps(*itNext) || itPrevious->getLastAddress() + 1 == itNext->getFirstAddress())
        {
            *itPrevious = AddressRange::firstToLast(itPrevious->getFirstAddress(), itNext->getLastAddress());

            m_addressRanges.erase(itNext);
        }
        else
        {
            ++i;
        }
    }
}

} // namespace connection

} // namespace core
