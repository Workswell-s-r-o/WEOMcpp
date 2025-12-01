#include "core/device.h"
#include "core/utils.h"

namespace core
{

std::vector<DeviceType> DeviceType::m_allDevices {};

DeviceType::DeviceType(size_t internalId) :
    m_internalId(internalId)
{
}

size_t DeviceType::getInternalId() const
{
    return m_internalId;
}

DeviceType DeviceType::createDeviceType()
{
    const auto internalId = m_allDevices.size();
    const DeviceType deviceType(internalId);

    m_allDevices.push_back(deviceType);

    return deviceType;
}

const std::vector<DeviceType>& DeviceType::getAllDeviceTypes()
{
    return m_allDevices;
}

const std::set<Baudrate::Item> Baudrate::ALL_ITEMS
{
    Item::B_9600,
    Item::B_19200,
    Item::B_38400,
    Item::B_57600,
    Item::B_115200,
    Item::B_230400,
    Item::B_460800,
    Item::B_921600,
    Item::B_2000000,
    Item::B_3000000,
};

unsigned int Baudrate::getBaudrateSpeed(Item baudrate)
{
    switch (baudrate)
    {
        case Item::B_9600:
            return 9600;

        case Item::B_19200:
            return 19200;

        case Item::B_38400:
            return 38400;

        case Item::B_57600:
            return 57600;

        case Item::B_115200:
            return 115200;

        case Item::B_230400:
            return 230400;

        case Item::B_460800:
            return 460800;

        case Item::B_921600:
            return 921600;

        case Item::B_2000000:
            return 2000000;

        case Item::B_3000000:
            return 3000000;

        default:
            assert(false);
            return 0;
    }
}

Version::Version(unsigned major, unsigned minor, unsigned minor2) :
    major(major),
    minor(minor),
    minor2(minor2)
{
}

std::string Version::toString() const
{
    return utils::format("{}.{}.{}", major, minor, minor2);
}

ValueResult<Version> Version::fromString(const std::string& versionString)
{
    using ResultType = ValueResult<Version>;

    std::vector<std::string> versionNumbers;

    size_t start = 0;
    size_t end = versionString.find('.');

    while (end != std::string::npos)
    {
        versionNumbers.push_back(versionString.substr(start, end - start));
        start = end + 1;
        end = versionString.find('.', start);
    }

    versionNumbers.push_back(versionString.substr(start));

    if (versionNumbers.size() != 3)
    {
        return ResultType::createError("Invalid version format!",
                                       utils::format("parts: {} expected: 3", versionNumbers.size()));
    }

    unsigned major, minor, minor2;

    try
    {
        major = std::stoi(versionNumbers[0]);
        minor = std::stoi(versionNumbers[1]);
        minor2 = std::stoi(versionNumbers[2]);
    }
    catch (const std::invalid_argument&)
    {
        return ResultType::createError("Invalid version format!", "One or more parts are not valid integers.");
    }
    catch (const std::out_of_range&)
    {
        return ResultType::createError("Invalid version format!", "One or more parts are out of range.");
    }

    return ResultType(Version(major, minor, minor2));
}

} // namespace core
