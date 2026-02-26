#ifndef PORTARGHELPER_H
#define PORTARGHELPER_H

#include "core/connection/serialportenumerator.h"
#include "core/logging.h"

#include <optional>

namespace example
{

inline std::optional<core::connection::SerialPortInfo> getPortFromArgsOrLogUsage(int argc, char* argv[])
{
    if (argc == 3)
    {
        core::connection::SerialPortInfo portInfo;
        portInfo.serialNumber = argv[1];
        portInfo.systemLocation = argv[2];
        return portInfo;
    }

    WW_LOG_CONNECTION_CRITICAL << "Usage: " << argv[0] << " <serial_number> <system_location>";
    WW_LOG_CONNECTION_CRITICAL << "Available ports:";
    for (const auto& port : core::connection::enumerateSerialPorts())
    {
        WW_LOG_CONNECTION_CRITICAL << "  vid=" << static_cast<int>(port.vendorIdentifier)
                                   << ", pid=" << static_cast<int>(port.productIdentifier)
                                   << ", serial=" << port.serialNumber
                                   << ", location=" << port.systemLocation;
    }

    return std::nullopt;
}

} // namespace example

#endif // PORTARGHELPER_H
