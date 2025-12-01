#include "core/properties/valueadapterusbstringdescriptor.h"
#include "core/logging.h"
#include "core/connection/datalinkuart.h"
#include "core/connection/serialportinfo.h"

#include <libusb-1.0/libusb.h>
#include <string>
#include <memory>

namespace core
{

class ValueAdapterUsbStringDescriptorImplUnix : public ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl
{
public:
    ValueAdapterUsbStringDescriptorImplUnix(uint8_t stringDescriptorIndex,
                                            std::shared_ptr<connection::IDataLinkInterface> datalink)
        : m_stringDescriptorIndex(stringDescriptorIndex)
        , m_datalink(datalink)
    {}

    ValueResult<std::string> getValue() override;

private:
    uint8_t m_stringDescriptorIndex;
    std::shared_ptr<connection::IDataLinkInterface> m_datalink;
};

ValueResult<std::string> ValueAdapterUsbStringDescriptorImplUnix::getValue()
{
    using ResultType = ValueResult<std::string>;

    const auto* datalinkUart = dynamic_cast<const connection::DataLinkUart*>(m_datalink.get());
    if (!datalinkUart)
    {
        return ResultType::createError("Datalink is not a UART, cannot get serial number.");
    }
    const auto portInfo = datalinkUart->getPortInfo();

    libusb_context* ctx = nullptr;
    if (libusb_init(&ctx) != 0)
    {
        return ResultType::createError("Failed to initialize libusb");
    }

    libusb_device** devs;
    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0)
    {
        libusb_exit(ctx);
        return ResultType::createError("Failed to get device list");
    }

    ResultType result = ResultType::createError("Device not found or serial number mismatch!");

    for (ssize_t i = 0; i < cnt; i++)
    {
        libusb_device* dev = devs[i];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0)
            continue;

        if (desc.idVendor == portInfo.vendorIdentifier && desc.idProduct == portInfo.productIdentifier)
        {
            libusb_device_handle* handle = nullptr;
            if (libusb_open(dev, &handle) != 0)
                continue;

            // Read serial number (string index 3)
            unsigned char serialBuf[256];
            int len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serialBuf, sizeof(serialBuf));
            if (len < 0)
            {
                libusb_close(handle);
                continue;
            }

            std::string deviceSerial(reinterpret_cast<char*>(serialBuf), len);

            WW_LOG_PROPERTIES_DEBUG << "Device serial number: " << deviceSerial
                                    << ", port serial number: " << portInfo.serialNumber;

            if (deviceSerial == portInfo.serialNumber)
            {
                WW_LOG_PROPERTIES_INFO << "Serial numbers match. This is the correct device.";

                unsigned char strBuf[256];
                len = libusb_get_string_descriptor_ascii(handle, m_stringDescriptorIndex, strBuf, sizeof(strBuf));
                if (len >= 0)
                {
                    result = ResultType(std::string(reinterpret_cast<char*>(strBuf), len));
                    WW_LOG_PROPERTIES_INFO << "Read USB string descriptor " << int(m_stringDescriptorIndex)
                                           << ": " << result.getValue();
                }
                else
                {
                    result = ResultType::createError("Failed to get USB string descriptor",
                                                     utils::format("libusb error: {}", len));
                }

                libusb_close(handle);
                break;
            }

            libusb_close(handle);
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);

    return result;
}

ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl*
createValueAdapterUsbStringDescriptorImpl(uint8_t stringDescriptorIndex,
                                          std::shared_ptr<connection::IDataLinkInterface> datalink)
{
    return new ValueAdapterUsbStringDescriptorImplUnix(stringDescriptorIndex, datalink);
}

} // namespace core
