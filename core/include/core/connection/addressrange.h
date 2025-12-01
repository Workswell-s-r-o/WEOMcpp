#ifndef CORE_CONNECTION_ADDRESSRANGE_H
#define CORE_CONNECTION_ADDRESSRANGE_H

#include "core/misc/result.h"
#include "core/misc/verify.h"

#include <vector>
#include <cassert>
#include <map>
#include <set>
#include <limits>
#include <string>
#include <cstdint>


namespace core
{

namespace connection
{

class AddressRange
{
    explicit constexpr AddressRange(uint32_t firstAddress, uint32_t lastAddress);

public:
    static constexpr AddressRange firstAndSize(uint32_t firstAddress, uint32_t size);
    static constexpr AddressRange firstToLast(uint32_t firstAddress, uint32_t lastAddress);

    constexpr uint32_t getFirstAddress() const;
    constexpr uint32_t getLastAddress() const;
    constexpr uint32_t getSize() const;

    bool contains(uint32_t address) const;
    bool contains(const AddressRange& other) const;
    bool overlaps(const AddressRange& other) const;

    constexpr AddressRange moved(uint32_t offset) const;

    std::strong_ordering operator<=>(const AddressRange& other) const = default;

    std::string toHexString() const;

    static std::string addressToHexString(uint32_t address);
    static ValueResult<AddressRange> fromHexString(const std::string& address, uint32_t size);
private:
    uint32_t m_firstAddress;
    uint32_t m_lastAddress;
};


class AddressRanges
{
public:
    AddressRanges() = default;
    AddressRanges(const AddressRanges& addressRanges) = default;
    AddressRanges& operator=(const AddressRanges& other) = default;

    AddressRanges(const AddressRange& range);
    AddressRanges(const std::vector<AddressRange>& ranges);
    AddressRanges(const AddressRanges& addressRanges1, const AddressRanges& addressRanges2);

    const std::vector<AddressRange>& getRanges() const;

    bool overlaps(const AddressRanges& other) const;
    bool contains(const AddressRanges& other) const;

    bool operator<(const AddressRanges& other) const;

private:
    void setRanges(const std::vector<AddressRange>& ranges);

    std::vector<AddressRange> m_addressRanges;
};


template<class T, class CompareT = std::less<T>>
class AddressRangeMap final
{
public:
    bool addRanges(const AddressRanges& ranges, T value);
    void removeRanges(T value);

    std::set<T, CompareT> getOverlap(const AddressRanges& ranges) const;

    const std::map<AddressRange, T>& getMap() const;

private:
    std::map<AddressRange, T> m_map;
};

// Impl

constexpr AddressRange::AddressRange(uint32_t firstAddress, uint32_t lastAddress) :
    m_firstAddress(firstAddress),
    m_lastAddress(std::max(firstAddress, lastAddress))
{
    assert(firstAddress <= lastAddress);
}

constexpr AddressRange AddressRange::firstAndSize(uint32_t firstAddress, uint32_t size)
{
    return AddressRange(firstAddress, firstAddress + size - 1);
}

constexpr AddressRange AddressRange::firstToLast(uint32_t firstAddress, uint32_t lastAddress)
{
    return AddressRange(firstAddress, lastAddress);
}

constexpr uint32_t AddressRange::getFirstAddress() const
{
    return m_firstAddress;
}

constexpr uint32_t AddressRange::getLastAddress() const
{
    return m_lastAddress;
}

constexpr uint32_t AddressRange::getSize() const
{
    return m_lastAddress + 1 - m_firstAddress;
}

constexpr AddressRange AddressRange::moved(uint32_t offset) const
{
    return AddressRange(m_firstAddress + offset, m_lastAddress + offset);
}

template<class T, class CompareT>
bool AddressRangeMap<T, CompareT>::addRanges(const AddressRanges& ranges, T value)
{
    if (!getOverlap(ranges).empty())
    {
        return false;
    }

    for (const auto& range : ranges.getRanges())
    {
        VERIFY(m_map.emplace(range, value).second);
    }

    return true;
}

template<class T, class CompareT>
void AddressRangeMap<T, CompareT>::removeRanges(T value)
{
    for (auto it = m_map.begin(); it != m_map.end(); )
    {
        if (it->second == value)
        {
            it = m_map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

template<class T, class CompareT>
std::set<T, CompareT> AddressRangeMap<T, CompareT>::getOverlap(const AddressRanges& ranges) const
{
    std::set<T, CompareT> result;

    for (const auto& range : ranges.getRanges())
    {
        // find first range which overlaps input range (or if no such range, it will be the first after the input range)
        auto beginOverlapIt = m_map.lower_bound(range);
        if (beginOverlapIt != m_map.begin() && std::prev(beginOverlapIt)->first.overlaps(range))
        {
            --beginOverlapIt;
        }

        // find first range after the input range which doesn't overlap with it
        auto endOverlapIt = m_map.end();
        if (range.getLastAddress() < std::numeric_limits<decltype(range.getLastAddress())>::max())
        {
            const auto addressAfterLast = AddressRange::firstAndSize(range.getLastAddress() + 1, 1);
            endOverlapIt = m_map.lower_bound(addressAfterLast); // this is first range which doesnt overlap with input range
        }

        for (auto overlapIt = beginOverlapIt; overlapIt != endOverlapIt; ++overlapIt)
        {
            result.insert(overlapIt->second);
        }
    }

    return result;
}

template<class T, class CompareT>
const std::map<AddressRange, T>& AddressRangeMap<T, CompareT>::getMap() const
{
    return m_map;
}

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_ADDRESSRANGE_H
