#ifndef FIRMWAREWTC640_H
#define FIRMWAREWTC640_H

#include "core/wtc640/devicewtc640.h"
#include "core/connection/addressrange.h"
#include "core/misc/result.h"

#include <boost/bimap.hpp>
#include <boost/json.hpp>

namespace core
{
class FirmwareWtc640
{
public:
    struct UpdateData
    {
        std::string hash;
        std::string fileName;
        uint32_t startAddress;
        std::vector<uint8_t> data;
    };

private:
    explicit FirmwareWtc640(const std::vector<UpdateData>& data, const boost::json::object& jsonConfig);

public:
    Version getFirmwareVersion() const;
    FirmwareType::Item getFirmwareType() const;
    const std::vector<UpdateData>& getUpdateData() const;
    void setUpdateData(std::vector<UpdateData> updateData);

    [[nodiscard]] VoidResult validateForPlugin(Plugin::Item pluginType) const;
    [[nodiscard]] VoidResult validateForMainVersion(const Version& mainVersion) const;
    [[nodiscard]] VoidResult validateForLoaderVersion(const Version& loaderVersion) const;
    [[nodiscard]] VoidResult validateForCore(const Version& loaderVersion, const Version& mainVersion, Plugin::Item pluginType) const;

    void addMainRestrictionFrom(const Version& versionFrom, bool isFromInclusive,
                                const std::string& errorMessage);
    void addMainRestrictionTo(const Version& versionTo, bool isToInclusive,
                              const std::string& errorMessage);
    void addMainRestrictionRange(const Version& versionFrom, bool isFromInclusive,
                                 const Version& versionTo, bool isToInclusive,
                                 const std::string& errorMessage);

    void addLoaderRestrictionFrom(const Version& versionFrom, bool isFromInclusive,
                                  const std::string& errorMessage);
    void addLoaderRestrictionTo(const Version& versionTo, bool isToInclusive,
                                const std::string& errorMessage);
    void addLoaderRestrictionRange(const Version& versionFrom, bool isFromInclusive,
                                   const Version& versionTo, bool isToInclusive,
                                   const std::string& errorMessage);

    static ValueResult<std::vector<uint8_t>> createRawUpdateDataFromJic(const std::string& inputFilename,
                                                              uint32_t startAddress, uint32_t endAddress);
    static std::optional<core::connection::AddressRange> getAddressRangeFromMapFile(const std::string& filename);

    [[nodiscard]] static ValueResult<FirmwareWtc640> readFromFile(const std::string& filename);
    [[nodiscard]] const VoidResult saveToFile(const std::string& filename) const;

    static std::string getHashForData(const std::vector<uint8_t>& data);

    [[nodiscard]] static ValueResult<FirmwareWtc640> createFirmware(std::vector<UpdateData> updateData,
                                                                    const Version& firmwareVersion, FirmwareType::Item firmwareType);
private:    
    [[nodiscard]] VoidResult validateForVersion(const Version& version, const boost::json::array& restrictions) const;

    [[nodiscard]] static VoidResult validateRestriction(const boost::json::value& restriction);
    [[nodiscard]] static VoidResult validateCondition(const boost::json::value& condition);
    void addRestriction(const std::string_view& restrictionsJsonKey,
                        const boost::json::value& conditionFrom,
                        const boost::json::value& conditionTo,
                        const std::string& errorMessage);
    static boost::json::value createCondition(const Version& version, bool isInclusive);

    [[nodiscard]] static ValueResult<FirmwareWtc640> readFromUwtcFile(const std::string& filename);
    [[nodiscard]] static ValueResult<FirmwareWtc640> readFromWtcFile(const std::string& filename);

    [[nodiscard]] static ValueResult<std::string> getDeviceNameFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<FirmwareType::Item> getFirmwareTypeFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<Version> getFirmwareVersionFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<boost::json::array> getMainRestrictionsFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<boost::json::array> getLoaderRestrictionsFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<std::vector<FirmwareWtc640::UpdateData>> getUpdateDataFromJson(const boost::json::object& jsonConfig);
    [[nodiscard]] static ValueResult<UpdateData> getUpdateDataFromJsonValue(const boost::json::value& updateData);

    [[nodiscard]] static ValueResult<std::string> getStringFromJson(const boost::json::object& jsonConfig, const std::string_view& jsonKey);
    [[nodiscard]] static ValueResult<boost::json::array> getRestrictionsFromJson(const boost::json::object& jsonConfig, const std::string_view& jsonKey);

    [[nodiscard]] static const ValueResult<Version> versionFromJsonString(const std::string& versionString);
    static const std::string versionToJsonString(const Version& version);

    static constexpr std::string_view CREATE_FIRMWARE_ERROR_MESSAGE = "Creating update data file failed.";

    static constexpr uint8_t JSON_FILE_VERSION = 1;

    static constexpr std::string_view JSON_ROOT_KEY_FILE_VERSION = "uwtc_version";
    static constexpr std::string_view JSON_ROOT_KEY_DEVICE_NAME = "device";
    static constexpr std::string_view JSON_ROOT_KEY_FIRMWARE_TYPE = "plugin";
    static constexpr std::string_view JSON_ROOT_KEY_FIRMWARE_VERSION = "version";
    static constexpr std::string_view JSON_ROOT_KEY_MAIN_RESTRICTIONS = "main_restrictions";
    static constexpr std::string_view JSON_ROOT_KEY_LOADER_RESTRICTIONS = "loader_restrictions";

    static constexpr std::string_view JSON_ROOT_KEY_UPDATE_FILES = "update_files";
    static constexpr std::string_view JSON_UPDATE_FILES_KEY_DATA_HASH = "hash";
    static constexpr std::string_view JSON_UPDATE_FILES_KEY_FILENAME = "filename";
    static constexpr std::string_view JSON_UPDATE_FILES_KEY_ADDRESS = "address";
    static constexpr std::array<std::string_view, 3> JSON_UPDATE_FILES_ALL_KEYS = {JSON_UPDATE_FILES_KEY_DATA_HASH, JSON_UPDATE_FILES_KEY_FILENAME, JSON_UPDATE_FILES_KEY_ADDRESS};

    static constexpr std::array<std::string_view, 7> JSON_ROOT_ALL_KEYS = {JSON_ROOT_KEY_DEVICE_NAME, JSON_ROOT_KEY_FIRMWARE_TYPE, JSON_ROOT_KEY_UPDATE_FILES, JSON_ROOT_KEY_FIRMWARE_VERSION, JSON_ROOT_KEY_MAIN_RESTRICTIONS, JSON_ROOT_KEY_LOADER_RESTRICTIONS, JSON_ROOT_KEY_FILE_VERSION};

    static constexpr std::string_view JSON_RESTRICTION_KEY_FROM_CONDITION = "from";
    static constexpr std::string_view JSON_RESTRICTION_KEY_TO_CONDITION = "to";
    static constexpr std::string_view JSON_RESTRICTION_KEY_ERROR_MESSAGE = "error";
    static constexpr std::array<std::string_view, 3> JSON_RESTRICTION_ALL_KEYS = {JSON_RESTRICTION_KEY_FROM_CONDITION, JSON_RESTRICTION_KEY_TO_CONDITION, JSON_RESTRICTION_KEY_ERROR_MESSAGE};

    static constexpr std::string_view JSON_CONDITION_KEY_EXCLUSIVE = "exclusive";
    static constexpr std::string_view JSON_CONDITION_KEY_INCLUSIVE = "inclusive";

    static constexpr std::string_view JSON_WTC640_DEVICE_NAME = "WTC640";
    static constexpr std::string_view JSON_FIRMWARE_VERSION_DELIMITER = ".";

    static const boost::json::object createJsonConfig(FirmwareType::Item firmwareType, const core::Version& firmwareVersion, std::vector<FirmwareWtc640::UpdateData> updateData);
    [[nodiscard]] static ValueResult<boost::json::object> readJsonConfigFromFile(std::string jsonFileName);

    using FirmwareTypeToJsonStringBimap = boost::bimap<FirmwareType::Item, std::string>;
    static const FirmwareTypeToJsonStringBimap& getFirmwareTypeToJsonStringBimap();

    using FileVersionType = uint16_t;
    using FileFirmwareType = uint16_t;
    static constexpr uint16_t FIRMWARE_FORMAT_ID = 0x4657;

    std::vector<UpdateData> m_data;
    boost::json::object m_jsonConfig;
};

} // namespace core

#endif // FIRMWAREWTC640_H
