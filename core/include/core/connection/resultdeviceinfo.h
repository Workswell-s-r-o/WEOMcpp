#ifndef CORE_CONNECTION_RESULTDEVICEINFO_H
#define CORE_CONNECTION_RESULTDEVICEINFO_H

#include "core/misc/result.h"


namespace core
{

namespace connection
{

struct ResultDeviceInfo : core::ResultSpecificInfo
{
    enum class Error
    {
        NO_CONNECTION,       // transmission failed - no connection
        NO_RESPONSE,         // transmission failed - device not responded
        TRANSMISSION_FAILED, // transmission failed - received incomplete data or data link jamming or device/sw protocols are incompatible

        DEVICE_IS_BUSY,   // transmission ok, but device refused - device is busy => try again later
        ACCESS_DENIED,    // transmission ok, but device refused - you have insufficient credentials => try again with appropriate credentials
        INVALID_DATA,     // transmission ok, but device refused - invalid data sent into device => try again with different data
        INVALID_SETTINGS, // transmission ok, but device refused - valid data sent, but some value setting(s) prevents the operation => try change configuration
    };

    constexpr ~ResultDeviceInfo() override {}

    constexpr explicit ResultDeviceInfo(Error error) : error(error) {}

    constexpr bool isRecoverableError() const
    {
        switch (error)
        {
            case Error::NO_RESPONSE:
            case Error::TRANSMISSION_FAILED:
            case Error::DEVICE_IS_BUSY:
                return true;

            default:
                return false;
        }
    }

    const Error error {Error::NO_CONNECTION};
};

constexpr ResultDeviceInfo INFO_NO_CONNECTION       {ResultDeviceInfo::Error::NO_CONNECTION};
constexpr ResultDeviceInfo INFO_NO_RESPONSE         {ResultDeviceInfo::Error::NO_RESPONSE};
constexpr ResultDeviceInfo INFO_TRANSMISSION_FAILED {ResultDeviceInfo::Error::TRANSMISSION_FAILED};
constexpr ResultDeviceInfo INFO_DEVICE_IS_BUSY      {ResultDeviceInfo::Error::DEVICE_IS_BUSY};
constexpr ResultDeviceInfo INFO_ACCESS_DENIED       {ResultDeviceInfo::Error::ACCESS_DENIED};
constexpr ResultDeviceInfo INFO_INVALID_DATA        {ResultDeviceInfo::Error::INVALID_DATA};
constexpr ResultDeviceInfo INFO_INVALID_SETTINGS    {ResultDeviceInfo::Error::INVALID_SETTINGS};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_RESULTDEVICEINFO_H
