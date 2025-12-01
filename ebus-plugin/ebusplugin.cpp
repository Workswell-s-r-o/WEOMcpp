#include "ebusplugin.h"

#include "datalinkebus.h"
#include "core/utils.h"

#include <PvDeviceAdapter.h>
#include <PvDeviceInfoGEV.h>
#include <PvDeviceInfoU3V.h>
#include <PvDeviceGEV.h>
#include <PvSystem.h>
#include <PvDeviceSerialPort.h>
#include <PvDeviceEventSink.h>

#include <boost/polymorphic_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>

namespace core
{
namespace connection
{

std::vector<EbusDevice> EbusPlugin::findDevices()
{
    std::vector<EbusDevice> devices;

    auto addEbusDevice = [&](PvDeviceInfo* info)
    {
        EbusDevice ebusDevice;

        const auto deviceType = info->GetType();
        if (deviceType == PvDeviceInfoTypeGEV)
        {
            const auto gev = boost::polymorphic_downcast<const PvDeviceInfoGEV*>(info);
            ebusDevice.type = EbusDevice::Type::NETWORK;
            ebusDevice.mac =  gev->GetMACAddress().GetAscii();
            ebusDevice.gateway = gev->GetDefaultGateway().GetAscii();
            ebusDevice.subnet = gev->GetSubnetMask().GetAscii();
            ebusDevice.ip = gev->GetIPAddress().GetAscii();
        }
        else if(deviceType == PvDeviceInfoTypeUSB)
        {
            assert(dynamic_cast<const PvDeviceInfoUSB*>(info));
            ebusDevice.type = EbusDevice::Type::USB;
        }
        else if(deviceType == PvDeviceInfoTypeU3V)
        {
            assert(dynamic_cast<const PvDeviceInfoU3V*>(info));
            ebusDevice.type = EbusDevice::Type::USB;
        }

        ebusDevice.serialNumber = info->GetUserDefinedName().GetAscii();
        ebusDevice.valid = info->IsConfigurationValid();
        ebusDevice.connectionID = info->GetConnectionID();

        devices.push_back(ebusDevice);
    };

    std::set<std::string> found;

    PvSystem pvSystem;
    pvSystem.Find();

    for (uint32_t i = 0; i < pvSystem.GetInterfaceCount(); i++)
    {
        const auto pvInterface = pvSystem.GetInterface(i);

        for (uint32_t d = 0; d < pvInterface->GetDeviceCount(); d++)
        {
            const auto deviceInfo = pvInterface->GetDeviceInfo(d);

            if (!found.contains(deviceInfo->GetUniqueID().GetAscii()))
            {
                found.insert(deviceInfo->GetUniqueID().GetAscii());
                addEbusDevice(deviceInfo->Copy());
            }
        }
    }

    return devices;
}

VoidResult EbusPlugin::setIpAddress(const EbusDevice& device, const NetworkSettings& settings)
{
    if (device.type != EbusDevice::Type::NETWORK)
    {
        return VoidResult::createError("Device is not a GigE Vision device");
    }

    if (!device.mac.has_value())
    {
        return VoidResult::createError("Device MAC address is missing");
    }

    PvResult result = PvDeviceGEV::SetIPConfiguration(PvString(device.mac->c_str()), PvString(settings.address.c_str()), PvString(settings.mask.c_str()), PvString(settings.gateway.c_str()));

    if (!result.IsOK())
    {
        return VoidResult::createError("Failed to set network settings!", utils::format("{} - {}", result.GetCodeString().GetAscii(), result.GetDescription().GetAscii()));
    }

    return VoidResult::createOk();
}

ValueResult<std::shared_ptr<IDataLinkWithBaudrateAndStreamSource>> EbusPlugin::createConnection(const EbusDevice& device, Baudrate::Item baudrate, EbusSerialPort port)
{
    using ResultType = ValueResult<std::shared_ptr<IDataLinkWithBaudrateAndStreamSource>>;

    static const std::map<EbusSerialPort, PvDeviceSerial> serialPortMap
    {
        {EBUS_SERIAL_PORT_INVALID, PvDeviceSerialInvalid},
        {EBUS_SERIAL_PORT_0,       PvDeviceSerial0},
        {EBUS_SERIAL_PORT_1,       PvDeviceSerial1},
        {EBUS_SERIAL_PORT_BULK_0,  PvDeviceSerialBulk0},
        {EBUS_SERIAL_PORT_BULK_1,  PvDeviceSerialBulk1},
        {EBUS_SERIAL_PORT_BULK_2,  PvDeviceSerialBulk2},
        {EBUS_SERIAL_PORT_BULK_3,  PvDeviceSerialBulk3},
        {EBUS_SERIAL_PORT_BULK_4,  PvDeviceSerialBulk4},
        {EBUS_SERIAL_PORT_BULK_5,  PvDeviceSerialBulk5},
        {EBUS_SERIAL_PORT_BULK_6,  PvDeviceSerialBulk6},
        {EBUS_SERIAL_PORT_BULK_7,  PvDeviceSerialBulk7},
    };
    static_assert(EBUS_SERIAL_PORT__END - EBUS_SERIAL_PORT__BEGIN == 11);
    assert(serialPortMap.contains(port));

    const auto result = DataLinkEbus::createConnection(device, baudrate, serialPortMap.at(port));
    if (result.isOk())
    {
        return std::static_pointer_cast<IDataLinkWithBaudrateAndStreamSource>(result.getValue());
    }
    else
    {
        return ResultType::createFromError(result);
    }
}

std::shared_ptr<IEbusPlugin> EbusPlugin::create(logging::ChannelFilters* parentChannelFilters)
{
    logging::initLogging(parentChannelFilters);

    return std::shared_ptr<EbusPlugin>(new EbusPlugin());;
}

} // namespace connection
} // namespace core

BOOST_DLL_ALIAS(core::connection::EbusPlugin::create, create)
