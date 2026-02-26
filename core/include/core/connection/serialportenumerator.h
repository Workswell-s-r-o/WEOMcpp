#pragma once

#include "core/connection/serialportinfo.h"
#include <vector>

namespace core::connection
{
    std::vector<core::connection::SerialPortInfo> enumerateSerialPorts();
}
