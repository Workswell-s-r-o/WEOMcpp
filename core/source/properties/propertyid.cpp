#include "core/properties/propertyid.h"

#include <algorithm>
#include <cassert>

namespace core
{

std::vector<PropertyId> PropertyId::m_allPropertyIds {};
std::map<size_t, PropertyId::PropertyData> PropertyId::m_internalIdToData {};
std::map<std::string, size_t> PropertyId::m_idStringToInternalId {};

PropertyId::PropertyId(size_t internalId) :
        m_internalId(internalId)
{
}

size_t PropertyId::getInternalId() const
{
    return m_internalId;
}

const std::string& PropertyId::getIdString() const
{
    assert(m_internalIdToData.find(m_internalId) != m_internalIdToData.end());

    return m_internalIdToData.at(m_internalId).idString;
}

const std::string& PropertyId::getInfo() const
{
    assert(m_internalIdToData.find(m_internalId) != m_internalIdToData.end());

    return m_internalIdToData.at(m_internalId).info;
}

PropertyId PropertyId::createPropertyId(const std::string& idString, const std::string& info)
{
    assert(!idString.empty());

    const auto internalId = m_allPropertyIds.size();
    const PropertyId propertyId(internalId);

    assert(!std::binary_search(m_allPropertyIds.begin(), m_allPropertyIds.end(), propertyId));
    assert(m_internalIdToData.find(internalId) == m_internalIdToData.end());
    assert(m_idStringToInternalId.find(idString) == m_idStringToInternalId.end() && "id string duplicity!");

    m_allPropertyIds.push_back(propertyId);
    std::sort(m_allPropertyIds.begin(), m_allPropertyIds.end());

    m_internalIdToData.emplace(internalId, PropertyData{idString, info});
    m_idStringToInternalId.emplace(idString, internalId);

    return propertyId;
}

std::optional<PropertyId> PropertyId::getPropertyIdByInternalId(size_t internalId)
{
    const auto it = std::lower_bound(m_allPropertyIds.begin(), m_allPropertyIds.end(), PropertyId(internalId));
    if (it != m_allPropertyIds.end())
    {
        return *it;
    }

    return std::nullopt;
}

std::optional<PropertyId> PropertyId::getPropertyIdByIdString(const std::string& idString)
{
    const auto it = m_idStringToInternalId.find(idString);
    if (it != m_idStringToInternalId.end())
    {
        return PropertyId(it->second);
    }
    return std::nullopt;
}

const std::vector<PropertyId>& PropertyId::getAllPropertyIds()
{
    return m_allPropertyIds;
}

} // namespace core
