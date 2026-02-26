#include "core/properties/propertyid.h"

#include <algorithm>
#include <cassert>

namespace core
{

std::vector<PropertyId> PropertyId::m_allPropertyIds {};
std::vector<PropertyId::PropertyData> PropertyId::m_allPropertyData {};
std::unordered_map<std::string, size_t> PropertyId::m_idStringToInternalId {};

PropertyId::PropertyId(size_t internalId) : m_internalId(internalId)
{
}

size_t PropertyId::getInternalId() const
{
    return m_internalId;
}

const PropertyId::PropertyData& PropertyId::getPropertyData() const
{
    assert(m_internalId < m_allPropertyIds.size());
    assert(m_internalId < m_allPropertyData.size());
    assert(m_internalId == m_allPropertyIds[m_internalId].m_internalId);

    return m_allPropertyData[m_internalId];
}

const std::string& PropertyId::getIdString() const
{
    return getPropertyData().idString;
}

const std::string& PropertyId::getInfo() const
{
    return getPropertyData().info;
}

const Version& PropertyId::getVersion() const
{
    return getPropertyData().version;
}

PropertyId PropertyId::createPropertyId(const std::string& idString,
                                        const std::string& info,
                                        const Version& version)
{
    assert(!idString.empty());
    assert(m_allPropertyData.size() == m_allPropertyIds.size());

    const auto internalId = m_allPropertyIds.size();
    const PropertyId propertyId(internalId);

    assert(!std::binary_search(m_allPropertyIds.begin(), m_allPropertyIds.end(), propertyId));
    assert(m_idStringToInternalId.find(idString) == m_idStringToInternalId.end() && "id string duplicity!");

    m_allPropertyIds.push_back(propertyId);
    m_allPropertyData.emplace_back(idString, info, version);

    m_idStringToInternalId.emplace(idString, internalId);

    return propertyId;
}

std::optional<PropertyId> PropertyId::getPropertyIdByInternalId(size_t internalId)
{
    if (internalId < m_allPropertyIds.size())
    {
        auto propertyId = m_allPropertyIds[internalId];
        assert(propertyId.m_internalId == internalId);
        return propertyId;
    }
    return std::nullopt;
}

std::optional<PropertyId> PropertyId::getPropertyIdByIdString(const std::string& idString)
{
    const auto it = m_idStringToInternalId.find(idString);
    if (it != m_idStringToInternalId.end())
    {
        return getPropertyIdByInternalId(it->second);
    }
    return std::nullopt;
}

const std::vector<PropertyId>& PropertyId::getAllPropertyIds()
{
    return m_allPropertyIds;
}

} // namespace core
