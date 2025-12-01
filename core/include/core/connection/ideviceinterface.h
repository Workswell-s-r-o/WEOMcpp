#ifndef CORE_CONNECTION_IDEVICEINTERFACE_H
#define CORE_CONNECTION_IDEVICEINTERFACE_H

#include "core/connection/addressrange.h"
#include "core/misc/progresscontroller.h"
#include "core/misc/result.h"

#include <boost/endian.hpp>

#include <vector>
#include <span>


namespace core
{

namespace connection
{

class IDeviceInterface
{
public:
    enum class DeviceEndianity
    {
        LITTLE,
        BIG
    };

    explicit IDeviceInterface(DeviceEndianity deviceEndianity);
    virtual ~IDeviceInterface();

    [[nodiscard]] virtual VoidResult readData(std::span<uint8_t> data, uint32_t address, ProgressTask progress) = 0;
    [[nodiscard]] virtual VoidResult writeData(std::span<const uint8_t> data, uint32_t address, ProgressTask progress) = 0;

    // reads maximum possible data using one packet
    [[nodiscard]] virtual ValueResult<std::vector<uint8_t>> readSomeData(uint32_t address, ProgressTask progress) = 0;

    [[nodiscard]] ValueResult<std::vector<uint8_t>> readAddressRange(const AddressRange& addressRange, ProgressTask progress);

    template<class T>
    [[nodiscard]] VoidResult readTypedData(std::span<T> data, uint32_t address, ProgressTask progress);
    template<class T>
    [[nodiscard]] ValueResult<std::vector<T>> readTypedDataFromRange(const AddressRange& addressRange, ProgressTask progress);
    template<class T>
    [[nodiscard]] VoidResult writeTypedData(std::span<const T> data, uint32_t address, ProgressTask progress);

    template<class T>
    [[nodiscard]] T fromDeviceEndianity(const T& value) const;

    template<class T>
    [[nodiscard]] T toDeviceEndianity(const T& value) const;

    template<class T>
    [[nodiscard]] std::vector<uint8_t> toByteData(std::span<const T> data) const;

private:
    DeviceEndianity m_deviceEndianity {DeviceEndianity::LITTLE};
};

// Impl

template<class T>
VoidResult IDeviceInterface::readTypedData(std::span<T> data, uint32_t address, ProgressTask progress)
{
    std::vector<uint8_t> byteData(data.size() * sizeof(T), 0);
    const auto result = readData(byteData, address, progress);
    if (!result.isOk())
    {
        return result;
    }

    const auto* byteDataPtr = reinterpret_cast<const T*>(byteData.data());
    for (auto& value : data)
    {
        assert(reinterpret_cast<const uint8_t*>(byteDataPtr) + sizeof(T) <= byteData.data() + byteData.size());
        value = fromDeviceEndianity(*byteDataPtr);

        ++byteDataPtr;
    }

    return VoidResult::createOk();
}

template<class T>
T IDeviceInterface::fromDeviceEndianity(const T& value) const
{
    if (m_deviceEndianity == DeviceEndianity::LITTLE)
    {
        return boost::endian::little_to_native(value);
    }
    else
    {
        return boost::endian::big_to_native(value);
    }
}

template<class T>
T IDeviceInterface::toDeviceEndianity(const T& value) const
{
    if (m_deviceEndianity == DeviceEndianity::LITTLE)
    {
        return boost::endian::native_to_little(value);
    }
    else
    {
        return boost::endian::native_to_big(value);
    }
}

template<class T>
ValueResult<std::vector<T>> IDeviceInterface::readTypedDataFromRange(const AddressRange& addressRange, ProgressTask progress)
{
    using ResultType = ValueResult<std::vector<T>>;

    assert(addressRange.getSize() % sizeof(T) == 0 && "invalid range size");
    std::vector<T> data(addressRange.getSize() / sizeof(T), 0);
    if (const auto result = readTypedData<T>(data, addressRange.getFirstAddress(), progress); !result.isOk())
    {
        return ResultType::createFromError(result);
    }

    return data;
}

template<class T>
VoidResult IDeviceInterface::writeTypedData(std::span<const T> data, uint32_t address, ProgressTask progress)
{
    return writeData(toByteData(data), address, progress);
}

template<class T>
std::vector<uint8_t> IDeviceInterface::toByteData(std::span<const T> data) const
{
    std::vector<uint8_t> byteData(data.size() * sizeof(T), 0);

    auto* byteDataPtr = reinterpret_cast<T*>(byteData.data());
    for (auto& value : data)
    {
        assert(reinterpret_cast<const uint8_t*>(byteDataPtr) + sizeof(T) <= byteData.data() + byteData.size());
        *byteDataPtr = toDeviceEndianity(value);

        ++byteDataPtr;
    }

    return byteData;
}

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_IDEVICEINTERFACE_H
