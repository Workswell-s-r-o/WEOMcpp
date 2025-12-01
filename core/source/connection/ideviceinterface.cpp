#include "core/connection/ideviceinterface.h"


namespace core
{

namespace connection
{

IDeviceInterface::IDeviceInterface(DeviceEndianity deviceEndianity) :
    m_deviceEndianity(deviceEndianity)
{
}

IDeviceInterface::~IDeviceInterface()
{
}

ValueResult<std::vector<uint8_t>> IDeviceInterface::readAddressRange(const AddressRange& addressRange, ProgressTask progress)
{
    using ResultType = ValueResult<std::vector<uint8_t>>;

    std::vector<uint8_t> data(addressRange.getSize(), 0);
    std::span<uint8_t> dataRange(data);

    const auto readResult = readData(dataRange, addressRange.getFirstAddress(), progress);
    if (!readResult.isOk())
    {
        return ResultType::createFromError(readResult);
    }

    return data;
}

} // namespace connection

} // namespace core
