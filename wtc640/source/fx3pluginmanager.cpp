#include "core/wtc640/fx3pluginmanager.h"

#include <zip.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>

namespace core
{
const char* const FX3_UPLOADER_ID = "FX3_UPLOADER";
const char* const FX3_FIRMWARE_IMAGE_FILE_NAME = "wtc640-fx3-uart-uvc.img";

std::uint8_t Fx3PluginManager::getDeviceCount()
{
    return core::FX3Device().getDeviceCount();
}

ValueResult<FX3Device::SerialVersion> Fx3PluginManager::getSerialVersion(std::uint8_t index)
{
    core::FX3Device fx3Device;
    if (!fx3Device.open(index))
    {
        return ValueResult<FX3Device::SerialVersion>::createError("Cannot connect to FX3 plugin");
    }

    if (fx3Device.getFirmwareId() != FX3_UPLOADER_ID && fx3Device.isBootLoaderRunning())
    {
        if (!fx3Device.uploadFw(FX3Device::getUloaderFw(), true))
        {
            return ValueResult<FX3Device::SerialVersion>::createError("Cannot initialize FX3 plugin uploader");
        }
        fx3Device.close();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!fx3Device.open(index))
        {
            return ValueResult<FX3Device::SerialVersion>::createError("Cannot connect to FX3 plugin");
        }
    }

    if (fx3Device.getFirmwareId() != FX3_UPLOADER_ID)
    {
        return ValueResult<FX3Device::SerialVersion>::createError("Cannot connect to FX3 plugin uploader");
    }
    return fx3Device.getSerialVersion();
}

VoidResult Fx3PluginManager::uploadFirmware(const std::string& fileName, std::uint8_t index, const std::string& serialNumber, const std::string& electronicsVersion)
{
    if (fileName.empty())
    {
        return VoidResult::createError("Plugin file is not selected.");
    }

    int error = 0;
    zip_t* archive = zip_open(fileName.c_str(), 0, &error);
    if (!archive)
    {
        return VoidResult::createError("Failed to open plugin file.", "libzip error: " + std::to_string(error));
    }
    auto close_zip = [](zip_t* z) { zip_close(z); };
    std::unique_ptr<zip_t, decltype(close_zip)> archive_ptr(archive, close_zip);

    struct zip_stat st;
    if (zip_stat(archive, FX3_FIRMWARE_IMAGE_FILE_NAME, 0, &st) != 0)
    {
        return VoidResult::createError("Firmware image not found in plugin file.");
    }

    zip_file_t* zf = zip_fopen(archive, FX3_FIRMWARE_IMAGE_FILE_NAME, 0);
    if (!zf)
    {
        return VoidResult::createError("Failed to open firmware image in plugin file.");
    }
    auto close_zip_file = [](zip_file_t* zfile) { zip_fclose(zfile); };
    std::unique_ptr<zip_file_t, decltype(close_zip_file)> zf_ptr(zf, close_zip_file);

    std::vector<uint8_t> fwImg(st.size);
    zip_fread(zf, fwImg.data(), fwImg.size());

    core::FX3Device fx3Device;
    if (!fx3Device.open(index) || fx3Device.getFirmwareId() != FX3_UPLOADER_ID)
    {
        return VoidResult::createError("Cannot connect to FX3 plugin uploader");
    }
    if (!fx3Device.uploadFw(fwImg))
    {
        return VoidResult::createError("Error occurred during firmware upload");
    }
#ifdef SERVICE_VERSION
    auto serialVersion = FX3Device::SerialVersion{serialNumber, electronicsVersion};
    if(serialVersion != fx3Device.getSerialVersion() && !fx3Device.setSerialVersion(serialVersion))
    {
        return VoidResult::createError("Error occurred during serial number setup");
    }
#endif
    return VoidResult::createOk();
}

#ifdef SERVICE_VERSION
VoidResult Fx3PluginManager::lockSerialNumber(std::uint8_t index)
{
    core::FX3Device fx3Device;
    if (!fx3Device.open(index) || fx3Device.getFirmwareId() != FX3_UPLOADER_ID)
    {
        return VoidResult::createError("Cannot connect to FX3 plugin uploader");
    }
    if (!fx3Device.lockSerialNumber())
    {
        return VoidResult::createError("Error occurred during serial number lock");
    }
    return VoidResult::createOk();
}
#endif
} // namespace core
