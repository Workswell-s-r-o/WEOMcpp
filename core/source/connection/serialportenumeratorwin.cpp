#include "core/connection/serialportenumerator.h"

#include <windows.h>
#include <setupapi.h>
#include <usbioctl.h>
#include <devguid.h>
#include <regstr.h>
#include <cfgmgr32.h>
#include <vector>
#include <string>
#include <cstdint>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace
{
    std::string ws2s(const std::wstring& w)
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }

    void parseHardwareId(const std::string& hwid, uint16_t& vid, uint16_t& pid)
    {
        auto findHex = [&](const char* key) -> uint16_t {
            auto pos = hwid.find(key);
            if (pos == std::string::npos) return 0;
            return static_cast<uint16_t>(std::stoul(hwid.substr(pos + 4, 4), nullptr, 16));
        };
        vid = findHex("VID_");
        pid = findHex("PID_");
    }

    std::wstring GetParentDeviceInstanceId(const std::wstring& childInstanceId)
    {
        DEVINST devInst;
        if (CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(childInstanceId.c_str()), CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
            return L"";

        DEVINST parentDevInst;
        if (CM_Get_Parent(&parentDevInst, devInst, 0) != CR_SUCCESS)
            return childInstanceId; // no parent, return self

        WCHAR parentId[MAX_DEVICE_ID_LEN];
        if (CM_Get_Device_IDW(parentDevInst, parentId, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS)
            return std::wstring(parentId);

        return L"";
    }

    std::string ExtractSerialNumberFromDeviceId(const std::wstring& deviceInstanceId)
    {
        size_t pos = deviceInstanceId.rfind(L'\\');
        if (pos != std::wstring::npos && pos + 1 < deviceInstanceId.size())
            return ws2s(deviceInstanceId.substr(pos + 1));
        return "";
    }

    std::string getUsbSerial(const std::string& comPort)
    {
        HDEVINFO devInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
        if (devInfoSet == INVALID_HANDLE_VALUE) return "";

        SP_DEVINFO_DATA devInfo{};
        devInfo.cbSize = sizeof(devInfo);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfoSet, i, &devInfo); ++i)
        {
            char friendlyName[256];
            if (SetupDiGetDeviceRegistryPropertyA(devInfoSet, &devInfo, SPDRP_FRIENDLYNAME, nullptr,
                                                  (PBYTE)friendlyName, sizeof(friendlyName), nullptr))
            {
                std::string fn(friendlyName);
                if (fn.find(comPort) != std::string::npos)
                {
                    WCHAR instanceId[MAX_DEVICE_ID_LEN];
                    if (SetupDiGetDeviceInstanceIdW(devInfoSet, &devInfo, instanceId, MAX_DEVICE_ID_LEN, nullptr))
                    {
                        std::wstring parentId = GetParentDeviceInstanceId(instanceId);
                        return ExtractSerialNumberFromDeviceId(parentId);
                    }
                }
            }
        }

        SetupDiDestroyDeviceInfoList(devInfoSet);
        return "";
    }
}

namespace core::connection
{

std::vector<core::connection::SerialPortInfo> enumerateSerialPorts()
{
    std::vector<core::connection::SerialPortInfo> ports;

    HDEVINFO devInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) return ports;

    SP_DEVINFO_DATA devInfo{};
    devInfo.cbSize = sizeof(devInfo);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfoSet, i, &devInfo); ++i)
    {
        core::connection::SerialPortInfo info;

        HKEY hKey = SetupDiOpenDevRegKey(devInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (!hKey) continue;

        char portName[64];
        DWORD size = sizeof(portName);
        if (RegQueryValueExA(hKey, "PortName", nullptr, nullptr, (LPBYTE)portName, &size) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            continue;
        }
        RegCloseKey(hKey);

        info.systemLocation = "\\\\.\\" + std::string(portName);

        char hardwareId[512];
        if (SetupDiGetDeviceRegistryPropertyA(devInfoSet, &devInfo, SPDRP_HARDWAREID, nullptr, (PBYTE)hardwareId, sizeof(hardwareId), nullptr))
        {
            parseHardwareId(hardwareId, info.vendorIdentifier, info.productIdentifier);
        }

        info.serialNumber = getUsbSerial(portName);

        ports.push_back(std::move(info));
    }

    SetupDiDestroyDeviceInfoList(devInfoSet);
    return ports;
}

}
