#include "core/properties/valueadapterusbstringdescriptor.h"
#include "core/logging.h"
#include "core/connection/datalinkuart.h"
#include "core/connection/serialportinfo.h"

#include <windows.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

// Define the GUID here to avoid linking against Usbhub.lib, which may not be available.
DEFINE_GUID(GUID_DEVINTERFACE_USB_HUB, 0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8);

namespace core
{

class ValueAdapterUsbStringDescriptorImplWin : public ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl
{
public:
    ValueAdapterUsbStringDescriptorImplWin(uint8_t stringDescriptorIndex, std::shared_ptr<connection::IDataLinkInterface> datalink)
        : m_stringDescriptorIndex(stringDescriptorIndex), m_datalink(datalink)
    {}

    ValueResult<std::string> getValue() override;

private:
    uint8_t m_stringDescriptorIndex;
    std::shared_ptr<connection::IDataLinkInterface> m_datalink;
};

ValueResult<std::string> ValueAdapterUsbStringDescriptorImplWin::getValue()
{
    using ResultType = ValueResult<std::string>;

    const auto* datalinkUart = dynamic_cast<const connection::DataLinkUart*>(m_datalink.get());
    if (!datalinkUart)
    {
        return ResultType::createError("Datalink is not a UART, cannot get serial number.");
    }
    const auto portInfo = datalinkUart->getPortInfo();

    // Enumerate all hubs
    HDEVINFO deviceInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_HUB,
                                               nullptr,
                                               nullptr,
                                               DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        return ResultType::createError("Failed to get hubs", utils::format("SetupDiGetClassDevs for hubs failed, error: {}", std::to_string(GetLastError())));
    }

    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    DWORD index = 0;
    for (index = 0; SetupDiEnumDeviceInterfaces(deviceInfo, nullptr,
                                                      &GUID_DEVINTERFACE_USB_HUB,
                                                      index, &interfaceData); ++index)
    {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData,
                                         nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0)
        {
            continue;
        }

        std::vector<BYTE> detailBuf(requiredSize);
        auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData,
                                              detail, requiredSize, nullptr, nullptr))
        {
            WW_LOG_PROPERTIES_CRITICAL << "SetupDiGetDeviceInterfaceDetailW for hub failed, error: " << GetLastError();
            continue;
        }

        HANDLE hubHandle = CreateFileW(detail->DevicePath,
                                       GENERIC_WRITE | GENERIC_READ,
                                       FILE_SHARE_WRITE | FILE_SHARE_READ,
                                       nullptr, OPEN_EXISTING, 0, nullptr);
        if (hubHandle == INVALID_HANDLE_VALUE)
        {
            WW_LOG_PROPERTIES_CRITICAL << "CreateFileW for hub failed, error: " << GetLastError();
            continue;
        }

        USB_NODE_INFORMATION hubInfo = {};
        DWORD bytes = 0;
        if (!DeviceIoControl(hubHandle, IOCTL_USB_GET_NODE_INFORMATION,
                             &hubInfo, sizeof(hubInfo),
                             &hubInfo, sizeof(hubInfo),
                             &bytes, nullptr))
        {
            WW_LOG_PROPERTIES_CRITICAL << "IOCTL_USB_GET_NODE_INFORMATION failed, error: " << GetLastError();
            CloseHandle(hubHandle);
            continue;
        }

        ULONG portCount = hubInfo.u.HubInformation.HubDescriptor.bNumberOfPorts;

        auto readStringDescriptor = [&](ULONG port, uint8_t descriptorIndex) -> ResultType
        {
            std::vector<BYTE> descBuf(sizeof(USB_DESCRIPTOR_REQUEST) + 256);
            auto* descReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(descBuf.data());
            memset(descReq, 0, descBuf.size());

            descReq->ConnectionIndex = port;
            descReq->SetupPacket.bmRequest = 0x80; // device-to-host, standard, device
            descReq->SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
            descReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8) | descriptorIndex;
            descReq->SetupPacket.wIndex = 0x0409; // English (US)
            descReq->SetupPacket.wLength = 256;

            DWORD descBytes = 0;
            if (DeviceIoControl(hubHandle,
                                IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                                descReq, sizeof(USB_DESCRIPTOR_REQUEST),
                                descReq, descBuf.size(),
                                &descBytes, nullptr))
            {
                auto* usbStrDesc = reinterpret_cast<PUSB_STRING_DESCRIPTOR>(
                    descBuf.data() + sizeof(USB_DESCRIPTOR_REQUEST));

                if (usbStrDesc->bLength > 2)
                {
                    std::wstring wstr(usbStrDesc->bString,
                                      (usbStrDesc->bLength - 2) / sizeof(WCHAR));
                    return ResultType(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wstr));
                }
                return ResultType::createError("Empty USB string descriptor");
            }
            return ResultType::createError("Failed to get USB string descriptor", utils::format("error: {}", GetLastError()));
        };

        for (ULONG port = 1; port <= portCount; ++port)
        {
            std::vector<BYTE> connInfoBuf(sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + sizeof(USB_PIPE_INFO) * 30);
            auto* connInfo = reinterpret_cast<PUSB_NODE_CONNECTION_INFORMATION_EX>(connInfoBuf.data());
            memset(connInfo, 0, connInfoBuf.size());
            connInfo->ConnectionIndex = port;

            if (!DeviceIoControl(hubHandle,
                                 IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                                 connInfo, connInfoBuf.size(),
                                 connInfo, connInfoBuf.size(),
                                 &bytes, nullptr) || connInfo->DeviceIsHub)
            {
                continue;
            }

            if (connInfo->DeviceDescriptor.idVendor == portInfo.vendorIdentifier &&
                connInfo->DeviceDescriptor.idProduct == portInfo.productIdentifier)
            {
                WW_LOG_PROPERTIES_INFO << "Found matching device on port " << port;

                // First, read descriptor 3 to get the serial number
                constexpr uint8_t SERIAL_NUMBER_DESCRIPTOR_INDEX = 3;
                ResultType serialNumberResult = readStringDescriptor(port, SERIAL_NUMBER_DESCRIPTOR_INDEX);

                if (!serialNumberResult.isOk())
                {
                    WW_LOG_PROPERTIES_CRITICAL << "Could not read serial number from matching device on port " << port << ". " << serialNumberResult.toString();
                    continue;
                }
                
                WW_LOG_PROPERTIES_DEBUG << "Device serial number: " << serialNumberResult.getValue() << ", port serial number: " << portInfo.serialNumber;

                if (serialNumberResult.getValue() == portInfo.serialNumber)
                {
                    WW_LOG_PROPERTIES_INFO << "Serial numbers match. This is the correct device.";
                    // Now read the requested string descriptor
                    ResultType finalResult = readStringDescriptor(port, m_stringDescriptorIndex);
                    if (finalResult.isOk())
                    {
                        WW_LOG_PROPERTIES_INFO << "Read USB string descriptor " << m_stringDescriptorIndex << ": " << finalResult.getValue();
                    }
                    CloseHandle(hubHandle);
                    SetupDiDestroyDeviceInfoList(deviceInfo);
                    return finalResult;
                }
            }
        }

        CloseHandle(hubHandle);
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);
    return ResultType::createError("Device not found or serial number mismatch!");
}

ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl* createValueAdapterUsbStringDescriptorImpl(uint8_t stringDescriptorIndex, std::shared_ptr<connection::IDataLinkInterface> datalink)
{
    return new ValueAdapterUsbStringDescriptorImplWin(stringDescriptorIndex, datalink);
}

}
