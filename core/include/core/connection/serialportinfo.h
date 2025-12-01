#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H

#include <string>
#include <cstdint>

namespace core
{
namespace connection
{

struct SerialPortInfo
{
    std::string systemLocation;
    std::string serialNumber;
    uint16_t vendorIdentifier = 1204;
    uint16_t productIdentifier = 249;
};

} // namespace connection
} // namespace core

#endif // SERIALPORTINFO_H
