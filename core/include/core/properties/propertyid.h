#ifndef CORE_PROPERTYID_H
#define CORE_PROPERTYID_H

#include "core/device.h"

#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace core
{

class PropertyId final
{
private:
    explicit PropertyId(size_t internalId);

public:
    size_t getInternalId() const;
    const std::string& getIdString() const;
    const std::string& getInfo() const;
    const Version& getVersion() const;

    std::strong_ordering operator<=>(const PropertyId& other) const = default;

    static PropertyId createPropertyId(const std::string& idString,
                                       const std::string& info,
                                       const Version& version);

    static std::optional<PropertyId> getPropertyIdByInternalId(size_t internalId);
    static std::optional<PropertyId> getPropertyIdByIdString(const std::string& idString);

    static const std::vector<PropertyId>& getAllPropertyIds();

private:
    struct PropertyData
    {
        std::string idString;
        std::string info;
        Version version;
    };


    const PropertyData& getPropertyData() const;
private:
    size_t m_internalId {0};

    static std::vector<PropertyId> m_allPropertyIds;
    static std::vector<PropertyData> m_allPropertyData;
    static std::unordered_map<std::string, size_t> m_idStringToInternalId;
};

} // namespace core

#endif // CORE_PROPERTYID_H
