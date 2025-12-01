#include "core/wtc640/firmwarewtc640.h"

#include "core/utils.h"
#include "core/wtc640/memoryspacewtc640.h"

#include <zip.h>
#include <boost/bimap.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/regex.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <filesystem>


namespace core
{

FirmwareWtc640::FirmwareWtc640(const std::vector<UpdateData>& data, const boost::json::object& jsonConfig) :
    m_data(data),
    m_jsonConfig(jsonConfig)
{
}

Version FirmwareWtc640::getFirmwareVersion() const
{
    return getFirmwareVersionFromJson(m_jsonConfig).getValue();
}

FirmwareType::Item FirmwareWtc640::getFirmwareType() const
{
    return getFirmwareTypeFromJson(m_jsonConfig).getValue();
}

const std::vector<FirmwareWtc640::UpdateData>& FirmwareWtc640::getUpdateData() const
{
    return m_data;
}

void FirmwareWtc640::setUpdateData(std::vector<UpdateData> updateData)
{
    m_data = updateData;
}

VoidResult FirmwareWtc640::validateForPlugin(Plugin::Item pluginType) const
{
    static const auto ERROR = VoidResult::createError("Wrong plugin type");
    switch (getFirmwareType())
    {
        case FirmwareType::Item::CMOS_PLEORA:
            return (pluginType == Plugin::Item::CMOS || pluginType == Plugin::Item::PLEORA) ? VoidResult::createOk() : ERROR;

        case FirmwareType::Item::HDMI:
            return pluginType == Plugin::Item::HDMI ? VoidResult::createOk() : ERROR;

        case FirmwareType::Item::ANALOG:
            return pluginType == Plugin::Item::ANALOG ? VoidResult::createOk() : ERROR;

        case FirmwareType::Item::USB:
            return (pluginType == Plugin::Item::USB || pluginType == Plugin::Item::ONVIF) ? VoidResult::createOk() : ERROR;

        case FirmwareType::Item::ALL:
            return VoidResult::createOk();

        default:
            assert(false);
            return VoidResult::createError("Could not determine what plugin is firmware for.");
    }
}

VoidResult FirmwareWtc640::validateForMainVersion(const Version& mainVersion) const
{
    return validateForVersion(mainVersion, getMainRestrictionsFromJson(m_jsonConfig).getValue());
}

VoidResult FirmwareWtc640::validateForLoaderVersion(const Version& loaderVersion) const
{
    return validateForVersion(loaderVersion, getLoaderRestrictionsFromJson(m_jsonConfig).getValue());
}

VoidResult FirmwareWtc640::validateForVersion(const Version& version, const boost::json::array& restrictions) const
{
    using Comparator = std::function<bool (const Version&, const Version&)>;
    auto satisfiesCondition = [&](const boost::json::value& condition, const Comparator& exclusiveComparator, const Comparator& inclusiveComparator)
    {
        if (condition.is_null())
        {
            return true;
        }

        const auto conditionObject = condition.as_object();
        assert(conditionObject.contains(JSON_CONDITION_KEY_EXCLUSIVE) != conditionObject.contains(JSON_CONDITION_KEY_INCLUSIVE));
        const bool isExclusive = conditionObject.contains(JSON_CONDITION_KEY_EXCLUSIVE.data());

        const auto& comparator = isExclusive ? exclusiveComparator : inclusiveComparator;
        const auto conditionVersion = versionFromJsonString(isExclusive ? std::string{conditionObject.at(JSON_CONDITION_KEY_EXCLUSIVE.data()).as_string()} :
                                                                          std::string{conditionObject.at(JSON_CONDITION_KEY_INCLUSIVE.data()).as_string()});
        assert(conditionVersion.isOk());

        return comparator(conditionVersion.getValue(), version);
    };

    for (const auto& restrictionValue : restrictions)
    {
        assert(validateRestriction(restrictionValue).isOk());

        const auto restrictionObject = restrictionValue.as_object();
        if (satisfiesCondition(restrictionObject.at(JSON_RESTRICTION_KEY_FROM_CONDITION.data()), std::less<Version>(), std::less_equal<Version>()) &&
            satisfiesCondition(restrictionObject.at(JSON_RESTRICTION_KEY_TO_CONDITION.data()), std::greater<Version>(), std::greater_equal<Version>()))
        {
            return VoidResult::createError(std::string{restrictionObject.at(JSON_RESTRICTION_KEY_ERROR_MESSAGE.data()).as_string()});
        }
    }

    return VoidResult::createOk();
}

VoidResult FirmwareWtc640::validateForCore(const Version& loaderVersion, const Version& mainVersion, Plugin::Item pluginType) const
{
    for (const auto& result : {validateForPlugin(pluginType),
                               validateForLoaderVersion(loaderVersion),
                               validateForMainVersion(mainVersion)})
    {
        if (!result.isOk())
        {
            return result;
        }
    }

    return VoidResult::createOk();
}

void FirmwareWtc640::addMainRestrictionFrom(const Version& versionFrom, bool isFromInclusive,
                                            const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_MAIN_RESTRICTIONS,
                   createCondition(versionFrom, isFromInclusive),
                   boost::json::value(nullptr),
                   errorMessage);
}

void FirmwareWtc640::addMainRestrictionTo(const Version& versionTo, bool isToInclusive,
                                          const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_MAIN_RESTRICTIONS,
                   boost::json::value(nullptr),
                   createCondition(versionTo, isToInclusive),
                   errorMessage);
}

void FirmwareWtc640::addMainRestrictionRange(const Version& versionFrom, bool isFromInclusive,
                                             const Version& versionTo, bool isToInclusive,
                                             const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_MAIN_RESTRICTIONS,
                   createCondition(versionFrom, isFromInclusive),
                   createCondition(versionTo, isToInclusive),
                   errorMessage);
}

void FirmwareWtc640::addLoaderRestrictionFrom(const Version& versionFrom, bool isFromInclusive,
                                              const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_LOADER_RESTRICTIONS,
                   createCondition(versionFrom, isFromInclusive),
                   boost::json::value(nullptr),
                   errorMessage);
}

void FirmwareWtc640::addLoaderRestrictionTo(const Version& versionTo, bool isToInclusive,
                                            const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_LOADER_RESTRICTIONS,
                   boost::json::value(nullptr),
                   createCondition(versionTo, isToInclusive),
                   errorMessage);
}

void FirmwareWtc640::addLoaderRestrictionRange(const Version& versionFrom, bool isFromInclusive,
                                               const Version& versionTo, bool isToInclusive,
                                               const std::string& errorMessage)
{
    addRestriction(JSON_ROOT_KEY_LOADER_RESTRICTIONS,
                   createCondition(versionFrom, isFromInclusive),
                   createCondition(versionTo, isToInclusive),
                   errorMessage);
}

VoidResult FirmwareWtc640::validateRestriction(const boost::json::value& restriction)
{
    if (!restriction.is_object())
    {
        return VoidResult::createError("Invalid restriction!", utils::format("restriction not object, type: {}", boost::json::to_string(restriction.kind())));
    }

    const auto restrictionObject = restriction.as_object();

    if (restrictionObject.size() != JSON_RESTRICTION_ALL_KEYS.size())
    {
        return VoidResult::createError("Invalid restriction!", std::string("found {} json keys, expected: {}", restrictionObject.size(), JSON_RESTRICTION_ALL_KEYS.size()));
    }

    if (!restrictionObject.contains(JSON_RESTRICTION_KEY_ERROR_MESSAGE.data()))
    {
        return VoidResult::createError("Invalid restriction!", "missing error message");
    }

    for (const auto& conditionKey : {JSON_RESTRICTION_KEY_FROM_CONDITION, JSON_RESTRICTION_KEY_TO_CONDITION})
    {
        if (!restrictionObject.contains(conditionKey.data()))
        {
            return VoidResult::createError("Invalid restriction!", utils::format("missing condition: {}", conditionKey));
        }

        const auto conditionResult = validateCondition(restrictionObject.at(conditionKey.data()));
        if (!conditionResult.isOk())
        {
            return conditionResult;
        }
    }

    return VoidResult::createOk();
}

VoidResult FirmwareWtc640::validateCondition(const boost::json::value& condition)
{
    if (condition.is_null())
    {
        return VoidResult::createOk();
    }

    if (!condition.is_object())
    {
        return VoidResult::createError("Invalid condition!", utils::format("condition not object, type: {}", boost::json::to_string(condition.kind())));
    }

    const auto conditionObject = condition.as_object();

    if (conditionObject.contains(JSON_CONDITION_KEY_EXCLUSIVE.data()) == conditionObject.contains(JSON_CONDITION_KEY_INCLUSIVE.data()))
    {
        return VoidResult::createError("Invalid condition!", "condition must be inclusive or exclusive");
    }

    const auto conditionVersion = conditionObject.contains(JSON_CONDITION_KEY_EXCLUSIVE.data()) ? conditionObject.at(JSON_CONDITION_KEY_EXCLUSIVE.data()) :
                                                                                                  conditionObject.at(JSON_CONDITION_KEY_INCLUSIVE.data());
    if (!conditionVersion.is_string())
    {
        return VoidResult::createError("Invalid condition!", utils::format("condition version not string, type: {}", boost::json::to_string(conditionVersion.kind())));
    }

    return versionFromJsonString(std::string{conditionVersion.as_string()}).toVoidResult();
}

void FirmwareWtc640::addRestriction(const std::string_view& restrictionsJsonKey,
                                          const boost::json::value& conditionFrom,
                                          const boost::json::value& conditionTo,
                                          const std::string& errorMessage)
{
    boost::json::object newRestriction;
    newRestriction[JSON_RESTRICTION_KEY_FROM_CONDITION.data()] = conditionFrom;
    newRestriction[JSON_RESTRICTION_KEY_TO_CONDITION.data()] = conditionTo;
    newRestriction[JSON_RESTRICTION_KEY_ERROR_MESSAGE.data()] = errorMessage;
    assert(validateRestriction(newRestriction).isOk());

    auto restrictions = m_jsonConfig.at(restrictionsJsonKey.data()).as_array();
    restrictions.push_back(newRestriction);
    m_jsonConfig[restrictionsJsonKey.data()] = restrictions;
    assert(getRestrictionsFromJson(m_jsonConfig, restrictionsJsonKey).isOk());
}

boost::json::value FirmwareWtc640::createCondition(const Version& version, bool isInclusive)
{
    boost::json::object condition;
    const std::string& key = isInclusive ? JSON_CONDITION_KEY_INCLUSIVE.data() : JSON_CONDITION_KEY_EXCLUSIVE.data();
    condition[key] = boost::json::value_from(versionToJsonString(version));

    return condition;
}

ValueResult<FirmwareWtc640> FirmwareWtc640::createFirmware(std::vector<FirmwareWtc640::UpdateData> updateData,
                                                           const Version& firmwareVersion, FirmwareType::Item firmwareType)
{
    using ResultType = ValueResult<FirmwareWtc640>;

    size_t updateDataSize = 0;
    for(auto& singularUpdateData : updateData)
    {
        if(singularUpdateData.data.empty())
        {
            return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), utils::format("data in {}! part is empty", singularUpdateData.fileName));
        }
        while (singularUpdateData.data.size() % connection::MemorySpaceWtc640::FLASH_WORD_SIZE != 0)
        {
            singularUpdateData.data.push_back(0xFF);
        }
        updateDataSize += singularUpdateData.data.size();
    }

    std::sort(updateData.begin(), updateData.end(), [](const FirmwareWtc640::UpdateData& a, const FirmwareWtc640::UpdateData& b)
    {
        return a.startAddress < b.startAddress;
    });

    if(updateData.size() >= 2)
    {
        for(size_t i = 0; i < updateData.size()-1; i++)
        {
            if(updateData[i].startAddress + updateData[i].data.size() > updateData[i+1].startAddress)
            {
                return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), utils::format("updateData overlaps between {} and {}", updateData[i].fileName, updateData[i+1].fileName));
            }
        }
    }

    auto reverseBits = [](unsigned char b)
    {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
    };

    std::vector<FirmwareWtc640::UpdateData> processedUpdateData;
    processedUpdateData.reserve(updateData.size());
    // copy part of the file to data, switch endianity of words and also bit order
    for (const auto& update : updateData)
    {
        std::vector<uint8_t> reversedData;
        reversedData.reserve(update.data.size());

        for (int i = 0; i < update.data.size(); i += connection::MemorySpaceWtc640::FLASH_WORD_SIZE)
        {
            for (int offset = connection::MemorySpaceWtc640::FLASH_WORD_SIZE - 1; offset >= 0; offset--)
            {
                reversedData.push_back(reverseBits(update.data.at(i + offset)));
            }
        }

        processedUpdateData.emplace_back(FirmwareWtc640::UpdateData{getHashForData(reversedData), update.fileName, update.startAddress, std::move(reversedData)});
    }

    return FirmwareWtc640(processedUpdateData, createJsonConfig(firmwareType, firmwareVersion, processedUpdateData));
}

ValueResult<FirmwareWtc640> FirmwareWtc640::readFromFile(const std::string& filename)
{
    using ResultType = ValueResult<FirmwareWtc640>;

    if (filename.ends_with("uwtc"))
    {
        return readFromUwtcFile(filename);
    }
    else
    {
        return ResultType::createError("File is not .uwtc!");
    }
}

ValueResult<FirmwareWtc640> FirmwareWtc640::readFromUwtcFile(const std::string& filename)
{
    using ResultType = ValueResult<FirmwareWtc640>;
    namespace fs = std::filesystem;

    fs::path tempDir = fs::temp_directory_path() / "wtcupdate";

    if (fs::exists(tempDir))
    {
        fs::remove_all(tempDir);
    }
    fs::create_directory(tempDir);

    int error = 0;
    auto close_zip = [](zip_t* z) { zip_close(z); };
    std::unique_ptr<zip_t, decltype(close_zip)> archive(zip_open(filename.c_str(), 0, &error), close_zip);
    if(!archive.get())
    {
        return ResultType::createError("Failed to open UWTC file.", "libzip error: " + std::to_string(error));
    }

    zip_int64_t num_entries = zip_get_num_entries(archive.get(), 0);
    for (zip_int64_t i = 0; i < num_entries; ++i)
    {
        struct zip_stat st;
        zip_stat_index(archive.get(), i, 0, &st);

        fs::path outpath = tempDir / st.name;

        if (st.name[strlen(st.name) - 1] == '/')
        {
            fs::create_directory(outpath);
        }
        else
        {
            zip_file_t* zf = zip_fopen_index(archive.get(), i, 0);
            if (!zf)
            {
                return ResultType::createError("Failed to open file in zip archive.");
            }
            auto close_zip_file = [](zip_file_t* zfile) { zip_fclose(zfile); };
            std::unique_ptr<zip_file_t, decltype(close_zip_file)> zf_ptr(zf, close_zip_file);

            std::vector<char> buffer(st.size);
            zip_fread(zf, buffer.data(), st.size);

            std::ofstream of(outpath, std::ios::binary);
            of.write(buffer.data(), buffer.size());
        }
    }

    const auto jsonConfigResult = readJsonConfigFromFile((tempDir / "config.json").string());
    if (!jsonConfigResult.isOk())
    {
        return ResultType::createFromError(jsonConfigResult);
    }

    if (getDeviceNameFromJson(jsonConfigResult.getValue()).getValue() != JSON_WTC640_DEVICE_NAME)
    {
        return ResultType::createError("Invalid device type!", utils::format("device type in config: {} expected: {}", getDeviceNameFromJson(jsonConfigResult.getValue()).getValue(), JSON_WTC640_DEVICE_NAME));
    }

    const auto updateDataResult = getUpdateDataFromJson(jsonConfigResult.getValue());

    if (!updateDataResult.isOk())
    {
        return ResultType::createFromError(updateDataResult);
    }
    const auto updateData = updateDataResult.getValue();

    fs::remove_all(tempDir);

    return FirmwareWtc640(updateData, jsonConfigResult.getValue());
}

const VoidResult FirmwareWtc640::saveToFile(const std::string& filename) const
{
    static const std::string GENERAL_ERROR_MESSAGE("Creating update data file failed.");

    int error = 0;
    std::unique_ptr<zip_t, decltype(&zip_close)> archive(zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error), &zip_close);
    if (!archive.get())
    {
        return VoidResult::createError(GENERAL_ERROR_MESSAGE, "Failed to create zip file: " + std::to_string(error));
    }

    auto addBufferToZip = [&](const std::string& entryName, const char* buffer, size_t bufferSize) -> VoidResult
    {
        zip_source_t* source = zip_source_buffer(archive.get(), buffer, bufferSize, 0);
        if (!source)
        {
            return VoidResult::createError(GENERAL_ERROR_MESSAGE, "Failed to create zip source: " + std::string(zip_strerror(archive.get())));
        }

        if (zip_file_add(archive.get(), entryName.c_str(), source, ZIP_FL_OVERWRITE) < 0)
        {
            zip_source_free(source);
            return VoidResult::createError(GENERAL_ERROR_MESSAGE, "Failed to add file to zip: " + std::string(zip_strerror(archive.get())));
        }

        return VoidResult::createOk();
    };

    const std::string json = boost::json::serialize(m_jsonConfig);
    VoidResult result = addBufferToZip("config.json", json.c_str(), json.size());
    if (!result.isOk())
    {
        return result;
    }

    for (const auto& update : m_data)
    {
        result = addBufferToZip(update.fileName, reinterpret_cast<const char*>(update.data.data()), update.data.size());
        if (!result.isOk())
        {
            return result;
        }
    }

    return VoidResult::createOk();
}

ValueResult<std::string> FirmwareWtc640::getDeviceNameFromJson(const boost::json::object& jsonConfig)
{
    return getStringFromJson(jsonConfig, JSON_ROOT_KEY_DEVICE_NAME);
}

ValueResult<FirmwareType::Item> FirmwareWtc640::getFirmwareTypeFromJson(const boost::json::object& jsonConfig)
{
    using ResultType = ValueResult<FirmwareType::Item>;

    const auto jsonStringResult = getStringFromJson(jsonConfig, JSON_ROOT_KEY_FIRMWARE_TYPE);
    if (!jsonStringResult.isOk())
    {
        return ResultType::createFromError(jsonStringResult);
    }

    const auto& jsonStringToType = getFirmwareTypeToJsonStringBimap().right;
    const auto it = jsonStringToType.find(jsonStringResult.getValue());
    if (it != jsonStringToType.end())
    {
        return it->get_left();
    }

    return ResultType::createError("Read firmware error!", utils::format("unknown device type: {}", jsonStringResult.getValue()));
}

ValueResult<Version> FirmwareWtc640::getFirmwareVersionFromJson(const boost::json::object& jsonConfig)
{
    using ResultType = ValueResult<Version>;

    const auto jsonStringResult = getStringFromJson(jsonConfig, JSON_ROOT_KEY_FIRMWARE_VERSION);
    if (!jsonStringResult.isOk())
    {
        return ResultType::createFromError(jsonStringResult);
    }

    return versionFromJsonString(jsonStringResult.getValue());
}

ValueResult<boost::json::array> FirmwareWtc640::getMainRestrictionsFromJson(const boost::json::object& jsonConfig)
{
    return getRestrictionsFromJson(jsonConfig, JSON_ROOT_KEY_MAIN_RESTRICTIONS);
}

ValueResult<boost::json::array> FirmwareWtc640::getLoaderRestrictionsFromJson(const boost::json::object& jsonConfig)
{
    return getRestrictionsFromJson(jsonConfig, JSON_ROOT_KEY_LOADER_RESTRICTIONS);
}

ValueResult<std::vector<FirmwareWtc640::UpdateData>> FirmwareWtc640::getUpdateDataFromJson(const boost::json::object& jsonConfig)
{
    using ResultType = ValueResult<std::vector<FirmwareWtc640::UpdateData>>;
    const auto jsonArray = jsonConfig.at(JSON_ROOT_KEY_UPDATE_FILES).as_array();

    if(jsonArray.empty())
    {
        return ResultType::createError("Read firmware error!", "update_files is empty");
    }

    std::vector<FirmwareWtc640::UpdateData> result;
    result.reserve(jsonArray.size());

    for(const auto& item : jsonArray)
    {
        const auto conversionResult = getUpdateDataFromJsonValue(item.as_object());
        if(!conversionResult.isOk())
        {
            return ResultType::createError("Read firmware error!", conversionResult.toString());
        }
        result.push_back(conversionResult.getValue());
    }

    return result;
}

ValueResult<FirmwareWtc640::UpdateData> FirmwareWtc640::getUpdateDataFromJsonValue(const boost::json::value& updateData)
{
    using ResultType = ValueResult<UpdateData>;

    if(!updateData.is_object())
    {
        return ResultType::createError("Read firmware error!", "update_files is not an array of json objects");
    }

    const auto object = updateData.as_object();

    if(object.size() != JSON_UPDATE_FILES_ALL_KEYS.size())
    {
        return ResultType::createError("Read firmware error!", utils::format("update_files has an object whose keys are not equal to {}, found: {}, expected: {}", JSON_UPDATE_FILES_ALL_KEYS.size(), object.size(), JSON_UPDATE_FILES_ALL_KEYS.size()));
    }

    for(const auto& key : JSON_UPDATE_FILES_ALL_KEYS)
    {
        if(auto result = getStringFromJson(object, key); !result.isOk())
        {
            return ResultType::createFromError(result.toVoidResult());
        }
    }

    const std::string filename = getStringFromJson(object, JSON_UPDATE_FILES_KEY_FILENAME).getValue();
    std::filesystem::path tmpPath = std::filesystem::temp_directory_path();
    tmpPath /= "wtcupdate";
    tmpPath /= filename.c_str();

    std::ifstream updateDataFile(tmpPath, std::ios::binary);

    if(!updateDataFile.is_open())
    {
        return ResultType::createError("Read firmware error!", "unable to read extracted update data");
    }

    updateDataFile.seekg(0, std::ios::end);
    std::streampos fileSize = updateDataFile.tellg();
    updateDataFile.seekg(0, std::ios::beg);

    if (fileSize == 0)
    {
        return ResultType::createError("Read firmware error!", utils::format("file {} is of size 0, or does not exist in the .uwtc file", filename));
    }

    std::vector<uint8_t> data(fileSize);
    updateDataFile.read(reinterpret_cast<char*>(data.data()), fileSize);
    const auto hashFromJsonObject = getStringFromJson(object, JSON_UPDATE_FILES_KEY_DATA_HASH).getValue();
    const auto hashFromFile = getHashForData(data);
    updateDataFile.close();

    if(utils::stringToUpperTrimmed(hashFromJsonObject) != utils::stringToUpperTrimmed(hashFromFile))
    {
        return ResultType::createError("Read firmware error!", utils::format("hashes do not match between {} and config, expected: {}, got: {}", filename, hashFromJsonObject, hashFromFile));
    }

    if (data.size() % connection::MemorySpaceWtc640::FLASH_WORD_SIZE != 0)
    {
        return ResultType::createError("Read firmware data error!", utils::format("invalid alignment: {} must be {} multiple", data.size(), connection::MemorySpaceWtc640::FLASH_WORD_SIZE));
    }

    const auto addressRangeFromJsonObject = core::connection::AddressRange::fromHexString(getStringFromJson(object, JSON_UPDATE_FILES_KEY_ADDRESS).getValue(), data.size());
    if(!addressRangeFromJsonObject.isOk())
    {
        return ResultType::createError("Read firmware error!", utils::format("file {} is of size 0, or does not exist in the .uwtc file", filename));
    }

    return FirmwareWtc640::UpdateData(hashFromJsonObject, filename, addressRangeFromJsonObject.getValue().getFirstAddress(), data);
}

ValueResult<std::string> FirmwareWtc640::getStringFromJson(const boost::json::object& jsonConfig, const std::string_view& jsonKey)
{
    using ResultType = ValueResult<std::string>;

    if (!jsonConfig.contains(jsonKey.data()))
    {
        return ResultType::createError("Read firmware error!", utils::format("key: {} not found", jsonKey.data()));
    }

    const auto value = jsonConfig.at(jsonKey.data());
    if (!value.is_string())
    {
        return ResultType::createError("Read firmware error!", utils::format("key: {} not string, is type: {}", jsonKey.data(), boost::json::to_string(value.kind())));
    }

    return std::string{value.as_string()};
}

ValueResult<boost::json::array> FirmwareWtc640::getRestrictionsFromJson(const boost::json::object& jsonConfig, const std::string_view& jsonKey)
{
    using ResultType = ValueResult<boost::json::array>;

    if (!jsonConfig.contains(jsonKey.data()))
    {
        return ResultType::createError("Read firmware error!", utils::format("key: {} not found", jsonKey));
    }

    const auto value = jsonConfig.at(jsonKey.data());
    if (!value.is_array())
    {
        return ResultType::createError("Read firmware error!", utils::format("key: {} not array, is type: {}", jsonKey.data(), boost::json::to_string(value.kind())));
    }

    const auto restrictions = value.as_array();

    for (const auto& restrictionValue : restrictions)
    {
        if (const auto result = validateRestriction(restrictionValue); !result.isOk())
        {
            return ResultType::createFromError(result);
        }
    }

    return restrictions;
}

const ValueResult<Version> FirmwareWtc640::versionFromJsonString(const std::string& versionString)
{
    using ResultType = ValueResult<Version>;

    core::Version version(0, 0, 0);
    std::vector<std::string> splitStrings(3);
    boost::split(splitStrings, versionString, boost::is_any_of(JSON_FIRMWARE_VERSION_DELIMITER));

    if (splitStrings.size() != core::Version::VERSION_SIZE)
    {
        return ResultType::createError("Invalid version format!",
                                       utils::format("parts: {} expected: {}", splitStrings.size(), core::Version::VERSION_SIZE));
    }

    std::vector<unsigned> versionNumbers;
    for (const auto& str : splitStrings)
    {
        try
        {
            versionNumbers.push_back(std::stoul(str));
        }
        catch (const std::exception&)
        {
            return ResultType::createError("Invalid version format!", utils::format("not valid integer: '{}'", str));
        }
    }

    version = core::Version(versionNumbers[0], versionNumbers[1], versionNumbers[2]);

    return ResultType(version);
}
const std::string FirmwareWtc640::versionToJsonString(const Version& version)
{
    return utils::format("{}{}{}{}{}", version.major, JSON_FIRMWARE_VERSION_DELIMITER, version.minor, JSON_FIRMWARE_VERSION_DELIMITER, version.minor2);
}

const boost::json::object FirmwareWtc640::createJsonConfig(FirmwareType::Item firmwareType, const core::Version& firmwareVersion, std::vector<FirmwareWtc640::UpdateData> updateData)
{
    boost::json::object jsonConfig;

    jsonConfig[JSON_ROOT_KEY_FILE_VERSION] = JSON_FILE_VERSION;
    jsonConfig[JSON_ROOT_KEY_DEVICE_NAME] = JSON_WTC640_DEVICE_NAME;
    jsonConfig[JSON_ROOT_KEY_FIRMWARE_TYPE] = getFirmwareTypeToJsonStringBimap().left.at(firmwareType);
    jsonConfig[JSON_ROOT_KEY_FIRMWARE_VERSION] = versionToJsonString(firmwareVersion);

    jsonConfig[JSON_ROOT_KEY_MAIN_RESTRICTIONS.data()] = boost::json::array();
    jsonConfig[JSON_ROOT_KEY_LOADER_RESTRICTIONS.data()] = boost::json::array();
    jsonConfig[JSON_ROOT_KEY_UPDATE_FILES] = boost::json::array();

    for(const auto& item : updateData)
    {
        auto appendedJsonArray = jsonConfig[JSON_ROOT_KEY_UPDATE_FILES].as_array();
        auto object = boost::json::object();
        object[JSON_UPDATE_FILES_KEY_DATA_HASH] = item.hash;
        object[JSON_UPDATE_FILES_KEY_ADDRESS] = core::connection::AddressRange::addressToHexString(item.startAddress);
        object[JSON_UPDATE_FILES_KEY_FILENAME] = item.fileName;
        appendedJsonArray.emplace_back(object);
        jsonConfig[JSON_ROOT_KEY_UPDATE_FILES] = appendedJsonArray;
    }

    return jsonConfig;
}

ValueResult<std::vector<uint8_t>> FirmwareWtc640::createRawUpdateDataFromJic(const std::string& inputFilename, uint32_t startAddress, uint32_t endAddress)
{
    using ResultType = ValueResult<std::vector<uint8_t>>;

    if (!inputFilename.ends_with(".jic"))
    {
        return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), "Data is not .jic file format.");
    }

    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile.is_open())
    {
        return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), utils::format("File {} is not accessible for read.", inputFilename));
    }

    const int QUARTUS_OFFSET = 0x9f;
    const int fileStartPosition = startAddress + QUARTUS_OFFSET;
    const int fileEndPosition = endAddress + QUARTUS_OFFSET;

    if (startAddress > endAddress)
    {
        return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), "Start address is bigger than end address.");
    }

    inputFile.seekg(0, std::ios::end);
    std::streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize < fileEndPosition)
    {
        return ResultType::createError(CREATE_FIRMWARE_ERROR_MESSAGE.data(), "Not enough data in input file!");
    }

    inputFile.seekg(fileStartPosition);

    std::vector<uint8_t> data(fileEndPosition - fileStartPosition);
    inputFile.read(reinterpret_cast<char*>(data.data()), data.size());

    return data;
}

std::optional<core::connection::AddressRange> FirmwareWtc640::getAddressRangeFromMapFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    static const std::string SEPARATOR = "\t\t";
    static const std::vector<std::string> HEADER = {"BLOCK", "START ADDRESS", "END ADDRESS"};

    std::string line;
    if (!std::getline(file, line))
    {
        return std::nullopt;
    }

    std::vector<std::string> splitLine;
    boost::trim_if(line, boost::is_any_of(SEPARATOR));
    boost::split(splitLine, line, boost::is_any_of(SEPARATOR), boost::token_compress_on);

    if (splitLine != HEADER)
    {
        return std::nullopt;
    }

    std::vector<std::string> dataLine;
    while (std::getline(file, line))
    {
        boost::split(splitLine, line, boost::is_any_of(SEPARATOR), boost::token_compress_on);
        if (splitLine.empty() || (splitLine.size() == 1 && splitLine[0].empty()))
        {
            if (dataLine.empty())
            {
                continue;
            }
            else
            {
                break;
            }
        }

        static const boost::regex ADDRESS_REGEX("^0x[0-9A-F]{8}$");
        if (splitLine.size() != HEADER.size() || !boost::regex_match(splitLine[1], ADDRESS_REGEX) || !boost::regex_match(splitLine[2], ADDRESS_REGEX))
        {
            break;
        }

        if (dataLine.empty())
        {
            dataLine = splitLine;
        }
        else
        {
            return std::nullopt;
        }
    }

    if (dataLine.empty())
    {
        return std::nullopt;
    }

    try
    {
        auto start = std::stoul(dataLine[1], nullptr, 16);
        auto end = std::stoul(dataLine[2], nullptr, 16);
        return core::connection::AddressRange::firstToLast(start, end);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}


ValueResult<boost::json::object> FirmwareWtc640::readJsonConfigFromFile(std::string jsonFileName)
{
    using ResultType = ValueResult<boost::json::object>;

    std::ifstream jsonFile(jsonFileName);
    if (!jsonFile.is_open())
    {
        return ResultType::createError("Read firmware data error!", utils::format("unable to open file {}", jsonFileName));
    }

    const boost::json::object obj = boost::json::parse(std::string{std::istreambuf_iterator<char>(jsonFile), std::istreambuf_iterator<char>()}).as_object();

    if (obj.size() != JSON_ROOT_ALL_KEYS.size() || obj.at(JSON_ROOT_KEY_FILE_VERSION).as_int64() > 1)
    {
        return ResultType::createError("Read firmware data error! Please update application to newest version.", utils::format("found {} json keys, expected: {}", obj.size(), JSON_ROOT_ALL_KEYS.size()));
    }

    for (const auto& result : {getDeviceNameFromJson(obj).toVoidResult(),
                               getFirmwareTypeFromJson(obj).toVoidResult(),
                               getFirmwareVersionFromJson(obj).toVoidResult(),
                               getMainRestrictionsFromJson(obj).toVoidResult(),
                               getLoaderRestrictionsFromJson(obj).toVoidResult(),
                               getUpdateDataFromJson(obj).toVoidResult()})
    {
        if (!result.isOk())
        {
            return ResultType::createFromError(result);
        }
    }

    return obj;
}

const FirmwareWtc640::FirmwareTypeToJsonStringBimap& FirmwareWtc640::getFirmwareTypeToJsonStringBimap()
{
    static const FirmwareTypeToJsonStringBimap firmwareTypeToJsonStringBimap = []()
    {
        FirmwareTypeToJsonStringBimap bimap;

        bimap.insert({FirmwareType::Item::CMOS_PLEORA, "CMOS_PLEORA"});
        bimap.insert({FirmwareType::Item::HDMI, "HDMI"});
        bimap.insert({FirmwareType::Item::ANALOG, "ANALOG"});
        bimap.insert({FirmwareType::Item::USB, "USB"});
        bimap.insert({FirmwareType::Item::ALL, "ALL"});

        assert(FirmwareType::ALL_ITEMS.size() == bimap.size());
        return bimap;
    }();

    return firmwareTypeToJsonStringBimap;
}

std::string FirmwareWtc640::getHashForData(const std::vector<uint8_t>& data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);

    std::string result;
    result.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        result += utils::numberToHex(hash[i], false);
    }

    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace core
