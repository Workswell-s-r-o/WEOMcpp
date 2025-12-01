#ifndef CORE_PROPERTYID_H
#define CORE_PROPERTYID_H

#include <vector>
#include <map>
#include <optional>
#include <string>

namespace core
{

class PropertyId
{
private:
    explicit PropertyId(size_t internalId);

public:
    size_t getInternalId() const;
    const std::string& getIdString() const;
    const std::string& getInfo() const;

    std::strong_ordering operator<=>(const PropertyId& other) const = default;

    static PropertyId createPropertyId(const std::string& idString, const std::string& info);

    static std::optional<PropertyId> getPropertyIdByInternalId(size_t internalId);
    static std::optional<PropertyId> getPropertyIdByIdString(const std::string& idString);

    static const std::vector<PropertyId>& getAllPropertyIds();

private:
    size_t m_internalId {0};

    struct PropertyData
    {
        std::string idString;
        std::string info;
    };

    static std::vector<PropertyId> m_allPropertyIds;

    static std::map<size_t, PropertyData> m_internalIdToData;
    static std::map<std::string, size_t> m_idStringToInternalId;
};

} // namespace core

#endif // CORE_PROPERTYID_H
