#ifndef IEBUSPLUGIN_H
#define IEBUSPLUGIN_H

#include "core/device.h"
#include "core/stream/idatalinkwithbaudrateandstreamsource.h"

class PvDeviceInfo;

namespace core
{
namespace connection
{

struct EbusDevice
{
    enum class Type
    {
        UNKNOWN,
        USB,
        NETWORK
    };
    Type type{ Type::UNKNOWN };

    std::optional<std::string> mac;
    std::optional<std::string> gateway;
    std::optional<std::string> subnet;
    std::optional<std::string> ip;

    std::string serialNumber;

    std::string connectionID;

    bool valid{ false };
};


enum EbusSerialPort
{
    EBUS_SERIAL_PORT__BEGIN,
    EBUS_SERIAL_PORT_INVALID = EBUS_SERIAL_PORT__BEGIN,
    EBUS_SERIAL_PORT_0,
    EBUS_SERIAL_PORT_1,
    EBUS_SERIAL_PORT_BULK_0,
    EBUS_SERIAL_PORT_BULK_1,
    EBUS_SERIAL_PORT_BULK_2,
    EBUS_SERIAL_PORT_BULK_3,
    EBUS_SERIAL_PORT_BULK_4,
    EBUS_SERIAL_PORT_BULK_5,
    EBUS_SERIAL_PORT_BULK_6,
    EBUS_SERIAL_PORT_BULK_7,
    EBUS_SERIAL_PORT__END
};

struct NetworkSettings
{
    std::string address;
    std::string mask;
    std::string gateway;
};

class IEbusPlugin
{
public:
    virtual ~IEbusPlugin() = default;

    [[nodiscard]] virtual std::vector<EbusDevice> findDevices() = 0;

    [[nodiscard]] virtual VoidResult setIpAddress(const EbusDevice& device, const NetworkSettings& settings) = 0;

    [[nodiscard]] virtual ValueResult<std::shared_ptr<IDataLinkWithBaudrateAndStreamSource>> createConnection(const EbusDevice& device, Baudrate::Item baudrate, EbusSerialPort port) = 0;
};

} // namespace connection
} // namespace core

#endif // IEBUSPLUGIN_H
