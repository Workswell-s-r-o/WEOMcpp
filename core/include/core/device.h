#ifndef CORE_DEVICE_H
#define CORE_DEVICE_H

#include "core/misc/result.h"

#include <vector>
#include <map>
#include <set>
#include <array>

namespace core
{

struct Size
{
    int width;
    int height;
    bool isValid() const {return width > 0 && height > 0 ? true : false;}
    auto operator<=>(const Size&) const = default;
};

class DeviceType
{
private:
    explicit DeviceType(size_t internalId);

public:
    size_t getInternalId() const;

    std::strong_ordering operator<=>(const DeviceType& other) const = default;

    static DeviceType createDeviceType();

    static const std::vector<DeviceType>& getAllDeviceTypes();

private:
    size_t m_internalId {0};

    static std::vector<DeviceType> m_allDevices;
};


class Baudrate
{
public:
    enum class Item
    {
        B_9600,
        B_19200,
        B_38400,
        B_57600,
        B_115200,
        B_230400,
        B_460800,
        B_921600,
        B_2000000,
        B_3000000,
    };

    static const std::set<Item> ALL_ITEMS;

    static unsigned getBaudrateSpeed(Item baudrate);
};

struct Version
{
    unsigned major {0};
    unsigned minor {0};
    unsigned minor2 {0};

    Version(unsigned major, unsigned minor, unsigned minor2);

    static constexpr unsigned VERSION_SIZE = 3;

    std::strong_ordering  operator<=>(const Version& other) const = default;

    std::string toString() const;
    [[nodiscard]] static ValueResult<Version> fromString(const std::string& versionString);
};

} // namespace core

#endif // CORE_DEVICE_H
