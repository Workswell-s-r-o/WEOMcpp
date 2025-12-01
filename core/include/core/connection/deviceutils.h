#ifndef CORE_CONNECTION_DEVICEUTILS_H
#define CORE_CONNECTION_DEVICEUTILS_H

#include <string>
#include <utility>

namespace core
{
namespace connection
{

std::pair<std::string, std::string> findVideoDeviceNameWithFormat(const std::string& serialNumber);

} // namespace connection
} // namespace core

#endif // CORE_CONNECTION_DEVICEUTILS_H
