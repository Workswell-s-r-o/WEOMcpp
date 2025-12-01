#ifndef CORE_PROPERTIESWTC640_H
#define CORE_PROPERTIESWTC640_H

#include "core/wtc640/devicewtc640.h"
#include "core/wtc640/memoryspacewtc640.h"
#include "core/properties/properties.h"
#include "core/properties/propertyadaptervaluedevicesimple.h"
#include "core/properties/propertyadaptervaluedeviceprogress.h"
#include "core/connection/stats.h"
#include "core/connection/iebusplugin.h"

#include "core/connection/datalinkuart.h"

#include "core/misc/palette.h"

#include <boost/signals2/signal.hpp>

namespace wtilib
{
struct CoreImage;
}

namespace core
{
class FirmwareWtc640;
namespace connection
{
class IDataLinkInterface;
class DeviceInterfaceWtc640;
}

class CommonTrigger
{
public:
    enum class Item
    {
        // user
        NUC_OFFSET_UPDATE,
        CLEAN_USER_DP,
        SET_SELECTED_PRESET,
        MOTORFOCUS_CALIBRATION,
        FRAME_CAPTURE_START,
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;

    /**
     * @brief Gets the address range for a given trigger and device type.
     * @param trigger The trigger item.
     * @param deviceType The device type.
     * @return A ValueResult containing the address range.
     */
    [[nodiscard]] static ValueResult<connection::AddressRange> getAddressRange(Item trigger, DeviceType deviceType);
    /**
     * @brief Gets the mask for a given trigger.
     * @param trigger The trigger item.
     * @return The mask.
     */
    [[nodiscard]] static uint32_t getMask(Item trigger);
};

class ResetTrigger
{
public:
    enum class Item
    {
        // loader
        RESET_FROM_LOADER,
        STAY_IN_LOADER,

        // user
        SOFTWARE_RESET, // for main resets main, for loader resets to main
        RESET_TO_LOADER,
        RESET_TO_FACTORY_DEFAULT,
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;

    /**
     * @brief Gets the address range for a given trigger and device type.
     * @param trigger The trigger item.
     * @param deviceType The device type.
     * @return A ValueResult containing the address range.
     */
    [[nodiscard]] static ValueResult<connection::AddressRange> getAddressRange(Item trigger, DeviceType deviceType);

    /**
     * @brief Gets the mask for a given trigger.
     * @param trigger The trigger item.
     * @return The mask.
     */
    [[nodiscard]] static uint32_t getMask(Item trigger);
};

struct PostProcessingMatrices
{
    std::vector<float> nuc;
    std::vector<int16_t> onuc;
    std::vector<int16_t> offset;

    PostProcessingMatrices() = default;
};

class PropertiesWtc640 : public Properties
{
    using BaseClass = Properties;

    friend class VideoFormatAdapter;

    explicit PropertiesWtc640(const std::shared_ptr<connection::IDeviceInterface>& deviceInterface, Mode mode, const std::shared_ptr<IMainThreadIndicator>& indicator, connection::IEbusPlugin* ebusPlugin);

public:
    /**
     * @brief Creates an instance of PropertiesWtc640.
     * @param mode The mode of operation.
     * @param indicator The main thread indicator.
     * @param ebusPlugin The eBus plugin.
     * @return A shared pointer to the new PropertiesWtc640 instance.
     */
    static std::shared_ptr<PropertiesWtc640> createInstance(Mode mode, const std::shared_ptr<IMainThreadIndicator>& indicator, connection::IEbusPlugin* ebusPlugin);

    /**
     * @brief Gets the eBus plugin.
     * @return A pointer to the eBus plugin.
     */
    connection::IEbusPlugin* getEbusPlugin() const;

    class ConnectionInfoTransaction;

    /**
     * @brief Creates a connection info transaction.
     * @return A ConnectionInfoTransaction object.
     */
    [[nodiscard]] ConnectionInfoTransaction createConnectionInfoTransaction();

    /**
     * @brief Tries to create a connection info transaction.
     * @param timeout The timeout duration.
     * @return An optional ConnectionInfoTransaction object.
     */
    [[nodiscard]] std::optional<ConnectionInfoTransaction> tryCreateConnectionInfoTransaction(const std::chrono::steady_clock::duration& timeout);

    class ConnectionStateTransaction;
    class ConnectionExclusiveTransactionWtc640;

    /**
     * @brief Creates a connection state transaction.
     * @return A ConnectionStateTransaction object.
     */
    [[nodiscard]] ConnectionStateTransaction createConnectionStateTransaction();

    /**
     * @brief Creates a connection exclusive transaction for Wtc640.
     * @param cancelRunningTasks Whether to cancel running tasks.
     * @return A ConnectionExclusiveTransactionWtc640 object.
     */
    [[nodiscard]] ConnectionExclusiveTransactionWtc640 createConnectionExclusiveTransactionWtc640(bool cancelRunningTasks);

    /**
     * @brief Gets the size in pixels.
     * @param transaction The properties transaction.
     * @return The size in pixels.
     */
    const core::Size& getSizeInPixels(const PropertiesTransaction& transaction) const;

    /**
     * @brief Gets the current baudrate.
     * @param transaction The properties transaction.
     * @return The current baudrate.
     */
    [[nodiscard]] std::optional<Baudrate::Item> getCurrentBaudrate(const PropertiesTransaction& transaction) const;

    /**
     * @brief Gets the current port info.
     * @param transaction The properties transaction.
     * @return The current port info.
     */
    [[nodiscard]] std::optional<core::connection::SerialPortInfo> getCurrentPortInfo(const PropertiesTransaction& transaction) const;

    /**
     * @brief Gets or creates a stream.
     * @param transaction The connection exclusive transaction.
     * @return A result containing a shared pointer to the stream.
     */
    [[nodiscard]] ValueResult<std::shared_ptr<IStream>> getOrCreateStream(const ConnectionExclusiveTransaction& transaction);

    /**
     * @brief Gets the stream.
     * @param transaction The connection exclusive transaction.
     * @return A result containing a shared pointer to the stream.
     */
    [[nodiscard]] ValueResult<std::shared_ptr<IStream>> getStream(const ConnectionExclusiveTransaction& transaction);

    enum PresetAttribute : uint8_t
    {
        // this enum is persisted - do not change values
        PRESETATTRIBUTE__BEGIN = 1,
        PRESETATTRIBUTE_TIMESTAMP = PRESETATTRIBUTE__BEGIN,
        PRESETATTRIBUTE_PRESET_ID,
        PRESETATTRIBUTE_GAIN_MATRIX,
        PRESETATTRIBUTE_ONUC_MATRIX,
        PRESETATTRIBUTE_SNUC_TABLE,
        PRESETATTRIBUTE__END
    };

    static constexpr int16_t  ONUC_MIN_VALUE = std::numeric_limits<int16_t>::min();
    static constexpr int16_t  ONUC_MAX_VALUE = std::numeric_limits<int16_t>::max();
    static constexpr float    NUC_MIN_VALUE  = 0;
    static constexpr float    NUC_MAX_VALUE  = 4;

    /**
     * @brief Gets the preset attribute IDs.
     * @param transaction The properties transaction.
     * @return A vector of maps of preset attributes to property IDs.
     */
    const std::vector<std::map<PresetAttribute, PropertyId>>& getPresetAttributeIds(const PropertiesTransaction& transaction) const;

    /**
     * @brief Gets the property source address ranges.
     * @param transaction The properties transaction.
     * @return A map of property IDs to address ranges.
     */
    std::map<PropertyId, connection::AddressRanges> getPropertySourceAddressRanges(const PropertiesTransaction& transaction) const;

    /**
     * @brief Refreshes the properties.
     * @param properties The set of property IDs to refresh.
     * @param transaction The properties transaction.
     */
    virtual void refreshProperties(const std::set<PropertyId>& properties, std::optional<PropertiesTransaction>& transaction) override;

    struct PresetId final
    {
        Lens::Item lens {Lens::Item::NOT_DEFINED};
        LensVariant::Item lensVariant {LensVariant::Item::A};
        PresetVersion::Item version {PresetVersion::Item::PRESET_VERSION_WITH_ONUC};
        Range::Item range {Range::Item::NOT_DEFINED};

        auto operator<=>(const PresetId& other) const = default;

        /**
         * @brief Checks if the preset ID is defined.
         * @return True if the preset ID is defined, false otherwise.
         */
        bool isDefined() const;
    };

    /**
     * @brief Sets the current lens range.
     * @param presetId The preset ID.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult setLensRangeCurrent(PresetId presetId, ProgressController progressController);

    /**
     * @brief Sets the current lens range.
     * @param presetIndex The preset index.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult setLensRangeCurrent(unsigned presetIndex, ProgressController progressController);

    /**
     * @brief Resets the core.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult resetCore(ProgressController progressController);

    /**
     * @brief Resets to factory default.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult resetToFactoryDefault(ProgressController progressController);

    /**
     * @brief Resets the core implementation.
     * @param trigger The reset trigger.
     * @param taskName The task name.
     * @param oldBaudrate The old baudrate.
     * @param progressController The progress controller.
     * @param exclusiveTransaction The exclusive transaction.
     * @return A void result.
     */
    [[nodiscard]] VoidResult resetCoreImpl(ResetTrigger::Item trigger, const std::string& taskName,
                                           const std::optional<Baudrate::Item>& oldBaudrate,
                                           ProgressController progressController,
                                           ConnectionExclusiveTransactionWtc640& exclusiveTransaction);

    /**
     * @brief Updates the firmware.
     * @param firmware The firmware to update.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult updateFirmware(const FirmwareWtc640& firmware, ProgressController progressController);

    /**
     * @brief Resets from the loader.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult resetFromLoader(ProgressController progressController);

    /**
     * @brief Gets the dependency validation ignore state.
     * @return The dependency validation ignore state.
     */
    const bool& getDependencyValidationIgnoreState();

    /**
     * @brief Sets the dependency validation ignore state.
     * @param state The new state.
     */
    void setDepedencyValidationIgnoreState(const bool& state);

    /**
     * @brief Checks if the video format is valid.
     * @param pluginType The plugin type.
     * @param videoFormat The video format.
     * @return True if the video format is valid, false otherwise.
     */
    [[nodiscard]] static bool isValidVideoFormat(Plugin::Item pluginType, VideoFormat::Item videoFormat);

    struct ImageFlip
    {
        ValueResult<bool> flipImageVertically {false};
        ValueResult<bool> flipImageHorizontally {false};

        bool operator==(const ImageFlip& other) const = default;
    };

    struct ArticleNumber
    {
        ValueResult<Sensor::Item> sensor {Sensor::Item::PICO_640};
        ValueResult<Core::Item> coreType {Core::Item::NON_RADIOMETRIC};
        ValueResult<DetectorSensitivity::Item> detectorSensitivity {DetectorSensitivity::Item::PERFORMANCE_NETD_50MK};
        ValueResult<Focus::Item> focus {Focus::Item::MANUAL_H25};
        ValueResult<Framerate::Item> maxFramerate {Framerate::Item::FPS_8_57};

        bool operator==(const ArticleNumber& other) const = default;

        /**
     * @brief Checks if all components of the article number are valid.
     * @return True if all components are valid, false otherwise.
     */
    bool isAllComponentsOk() const;

    /**
     * @brief Converts the article number to a string.
     * @return A ValueResult containing the string representation of the article number.
     */
    ValueResult<std::string> toString() const;

    /**
     * @brief Creates an ArticleNumber object from a string.
     * @param string The string to create the ArticleNumber from.
     * @return A ValueResult containing the created ArticleNumber.
     */
    static ValueResult<ArticleNumber> createFromString(const std::string& string);
    };

    struct NucMatrix
    {
        std::vector<float> data;

        bool operator==(const NucMatrix& other) const = default;
    };

    using LowerUpperPair = std::pair<int16_t, int16_t>;
    struct SnucMatrix
    {
        std::vector<LowerUpperPair> data;

        bool operator==(const SnucMatrix& other) const = default;
    };

    struct GskTable
    {
        std::vector<uint16_t> data;

        bool operator==(const GskTable& other) const = default;
    };

    struct Conbright
    {
        ValueResult<unsigned> contrast {0};
        ValueResult<unsigned> brightness {0};

        bool operator==(const Conbright& other) const = default;
    };

    static constexpr unsigned LINEAR_GAIN_WEIGHT_MIN_VALUE = 0;
    static constexpr unsigned LINEAR_GAIN_WEIGHT_MAX_VALUE = 10;

public:
    /**
     * @brief A signal that is emitted when the connection is lost.
     */
    boost::signals2::signal<void()> connectionLost;

protected:
    /**
     * @brief Called when the current device type changes.
     */
    virtual void onCurrentDeviceTypeChanged() override;

    /**
     * @brief Called when a transaction is finished.
     * @param transactionSummary The summary of the transaction.
     */
    virtual void onTransactionFinished(const TransactionSummary& transactionSummary) override;

private:
    using AddressRange = connection::AddressRange;
    using MemorySpaceWtc640 = connection::MemorySpaceWtc640;

    [[nodiscard]] std::optional<Baudrate::Item> getCurrentBaudrateImpl() const;

    const connection::DeviceInterfaceWtc640* getDeviceInterfaceWtc640() const;
    connection::DeviceInterfaceWtc640* getDeviceInterfaceWtc640();

    [[nodiscard]] static ValueResult<std::vector<uint8_t>> passwordToAdminKey(const std::string& password);

    [[nodiscard]] static ValueResult<DeviceType> testDeviceType(const ConnectionExclusiveTransaction& transaction);

    void createAdapters();

    void addControlAdapters();
    void addGeneralAdapters();
    void addVideoAdapters();
    void addNUCAdapters();
    void addConnectionAdapters();
    void addFiltersAdapters();
    void addDPRAdapters();
    void addFocusAdapters();
    void addPresetsAdapters();

    void addMotorFocusConstraints();
    void addImageFreezeConstraints();
    void addConnectionConstraints();
    void addPluginConstraints();

    void addPropertyConstraints(PropertyId sourcePropertyId,
                                std::function<PropertyAdapterBase::Status (const PropertyValues::Transaction& transaction)> constraintFuncion,
                                const std::initializer_list<PropertyId>& targetPropertyIds);

    void addCoreSerialNumberAdapters();
    void addArticleNumberAdapters();
    void addPalettesAdapters();
    void addImageFlipAdapters(PropertyId compositeProperty, PropertyId flipImageHorizontallyProperty, PropertyId flipImageVerticallyProperty,
                              const AddressRange& addressRange);
    void addCintAdapter(PropertyId propertyId, const AddressRange& addressRange, bool isEditable);
    void addVersionAdapter(PropertyId propertyId, const AddressRange& addressRange);

    void addDynamicPresetAdapters(connection::DeviceInterfaceWtc640* deviceInterface);
    void addDynamicUsbAdapters();
    void addDummyDynamicUsbAdapters();
    [[nodiscard]] VoidResult checkPresetAdapterAddressRange(const connection::AddressRange& addressRange, PropertyId propertyId) const;
    [[nodiscard]] ValueResult<PresetVersion::Item> getPresetVersion(uint8_t presetIndex);
    template<class T>
    VoidResult addNucMatrixAdapter(PropertyId propertyId, uint32_t address,
                                   const std::string& matrixName,
                                   const std::function<ValueResult<float> (T)>& toFloatFunction,
                                   const std::function<ValueResult<T> (float)>& fromFloatFunction);

    VoidResult addSnucMatrixAdapter(PropertyId propertyId, uint32_t address,
                                    const std::string& matrixName);

    [[nodiscard]]VoidResult addScucMatrixAdapter(PropertyId propertyId, uint32_t address,
                                                const std::string& matrixName);

    void removeDynamicPresetAdapters();
    void removeDynamicUsbAdapters();
    void onPresetAdaptersChanged();

    enum class UpdateGroup
    {
        NUC,
        BOLOMETER,
        FOCUS,
        PRESETS,
    };

    enum class DeviceFlags
    {
        NONE       = 0,
        MAIN_640   = 1 << 0,
        LOADER_640 = 1 << 1,
        ALL_640    = MAIN_640 | LOADER_640,
    };

    enum class ModeFlags
    {
        NONE = 0,
        USER = 1 << 1,
    };

    static PropertyAdapterBase::GetStatusForDeviceFunction createStatusFunction(DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags);

    void addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                                       DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                       unsigned minValue, unsigned maxValue);
    void addSignedDeviceValueSimpleAdapterArithmetic(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                                     DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                     signed minValue, signed maxValue);

    void addStringDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                           DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                           const std::function<VoidResult (const std::string&)>& validationFunction = nullptr,
                                           const std::function<std::string (const std::string&, const PropertyValues::Transaction&)>& transformFunction = nullptr);

    void addBoolDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                         DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags);

    template<class EnumDefinitionClass>
    void addEnumDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                         DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                         const std::function<VoidResult (typename EnumDefinitionClass::Item)>& validationFunction = nullptr,
                                         const std::function<typename EnumDefinitionClass::Item (typename EnumDefinitionClass::Item, const PropertyValues::Transaction&)>& transformFunction  = nullptr);

    template<class EnumDefinitionClass>
    std::shared_ptr<PropertyValueEnum<typename EnumDefinitionClass::Item>> createPropertyValueEnum(PropertyId propertyId,
                                                                                                    const std::function<VoidResult (const typename EnumDefinitionClass::Item&)>& validationFunction = nullptr);

    void addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                                      DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                      bool signedFormat, std::optional<double> minimum = std::nullopt, std::optional<double> maximum = std::nullopt);

    void addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                               DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                               double step, unsigned fractionalBits, unsigned totalBits, unsigned valueShift,
                                               double minimum, double maximum);

    PropertyAdapterValueDeviceSimple<unsigned>::ValueReader createUnsignedReader(const AddressRange& addressRange, uint32_t mask);
    PropertyAdapterValueDeviceSimple<unsigned>::ValueWriter createUnsignedWriter(const AddressRange& addressRange, uint32_t mask);

    PropertyAdapterValueDeviceSimple<std::string>::ValueReader createStringReader(const AddressRange& addressRange);
    PropertyAdapterValueDeviceSimple<std::string>::ValueWriter createStringWriter(const AddressRange& addressRange);
    PropertyAdapterValueDeviceSimple<signed>::ValueReader createSignedReader(const AddressRange& addressRange, uint32_t mask);
    PropertyAdapterValueDeviceSimple<signed>::ValueWriter createSignedWriter(const AddressRange& addressRange, uint32_t mask);

    PropertyAdapterValueDeviceProgress<core::Palette>::ValueReader createPaletteReader(const AddressRange& rangeName, const AddressRange& rangeData, const std::string& paletteName);
    PropertyAdapterValueDeviceProgress<core::Palette>::ValueWriter createPaletteWriter(const AddressRange& rangeName, const AddressRange& rangeData, const std::string& paletteName);

    static std::shared_ptr<PropertyValue<PresetId>> createPresetIdValueAdapter(PropertyId propertyId);

    enum ArticleSection
    {
        ARTICLESECTION__BEGIN = 0,
        ARTICLESECTION_SENSOR_TYPE = ARTICLESECTION__BEGIN,
        ARTICLESECTION_CORE_TYPE,
        ARTICLESECTION_DETECTOR_TYPE,
        ARTICLESECTION_FOCUS_TYPE,
        ARTICLESECTION_MAX_FRAMERATE,
        ARTICLESECTION__END,
    };
    /**
     * @brief Gets the article number section from the article number.
     * @param articleNumber The article number.
     * @param articleSection The article section to get.
     * @return The article number section.
     */
    static std::string getArticleNumberSection(const std::string& articleNumber, ArticleSection articleSection);
    /**
     * @brief Gets the regex pattern for the serial number.
     * @return The regex pattern for the serial number.
     */
    static std::string getSerialNumberRegexPattern();
    /**
     * @brief Gets the regex pattern for the article number.
     * @return The regex pattern for the article number.
     */
    static std::string getArticleNumberRegexPattern();
    /**
     * @brief Gets the sensor from the article number.
     * @param articleNumber The article number.
     * @return A ValueResult containing the sensor.
     */
    static ValueResult<Sensor::Item> getSensorFromArticleNumber(const std::string& articleNumber);
    /**
     * @brief Gets the core type from the article number.
     * @param articleNumber The article number.
     * @return A ValueResult containing the core type.
     */
    static ValueResult<Core::Item> getCoreTypeFromArticleNumber(const std::string& articleNumber);
    /**
     * @brief Gets the detector sensitivity from the article number.
     * @param articleNumber The article number.
     * @return A ValueResult containing the detector sensitivity.
     */
    static ValueResult<DetectorSensitivity::Item> getDetectorSensitivityFromArticleNumber(const std::string& articleNumber);
    /**
     * @brief Gets the focus from the article number.
     * @param articleNumber The article number.
     * @return A ValueResult containing the focus.
     */
    static ValueResult<Focus::Item> getFocusFromArticleNumber(const std::string& articleNumber);
    /**
     * @brief Gets the max framerate from the article number.
     * @param articleNumber The article number.
     * @return A ValueResult containing the max framerate.
     */
    static ValueResult<Framerate::Item> getMaxFramerateFromArticleNumber(const std::string& articleNumber);
    /**
     * @brief Gets the date from the serial number.
     * @param serialNumber The serial number.
     * @return A ValueResult containing the date.
     */
    static ValueResult<boost::posix_time::ptime> getDateFromSerialNumber(const std::string& serialNumber);

    /**
     * @brief Converts data to a string.
     * @param data The data to convert.
     * @return The converted string.
     */
    static std::string dataToString(const std::vector<uint8_t>& data);
    /**
     * @brief Converts a string to data.
     * @param string The string to convert.
     * @return The converted data.
     */
    static std::vector<uint8_t> stringToData(const std::string& string);

    /**
     * @brief Gets the attribute property ID string.
     * @param presetIndex The preset index.
     * @param attribute The preset attribute.
     * @return The attribute property ID string.
     */
    static std::string getAttributePropertyIdString(unsigned presetIndex, PresetAttribute attribute);
    /**
     * @brief Creates a composite value string.
     * @param components The components to create the string from.
     * @return The composite value string.
     */
    static std::string createCompositeValueString(const std::vector<std::pair<std::string, VoidResult>>& components);

    template<class ValueType>
    [[nodiscard]] VoidResult setLensRangeCurrent(PropertyId propertyId, PropertyId activePropertyId, const ValueType& newValue, ProgressController progressController);

    ValueResult<std::shared_ptr<IStream>> getOrCreateStreamImpl();
    ValueResult<std::shared_ptr<IStream>> getStreamImpl();

    template<class ValueType, typename... Args>
    void addValueAdapterImpl(const std::shared_ptr<PropertyValueEnum<ValueType>>& value, Args&&... args);

    template<typename... Args>
    void addValueAdapterImpl(const std::shared_ptr<PropertyValueEnum<VideoFormat::Item>>& value, Args&&... args);

    static bool isUpdateGroupChanged(const StatusWtc640& status, const UpdateGroup updateGroup);

private:
    static const std::map<Sensor::Item, std::string> ARTICLE_NUMBER_SENSORS;
    static const std::map<Core::Item, std::string> ARTICLE_NUMBER_CORE_TYPES;
    static const std::map<DetectorSensitivity::Item, std::string> ARTICLE_NUMBER_DETECTOR_SENSITIVITIES;
    static const std::map<Focus::Item, std::string> ARTICLE_NUMBER_FOCUSES;
    static const std::map<Framerate::Item, std::string> ARTICLE_NUMBER_FRAMERATES;

    static constexpr unsigned UINT_14_BIT_MAX_VALUE = (1 << 14) - 1;
    static constexpr unsigned UINT_12_BIT_MAX_VALUE = (1 << 12) - 1;
    static constexpr unsigned MOTOR_FOCUS_MIN_VALUE = 0;
    static constexpr unsigned MOTOR_FOCUS_MAX_VALUE = 3000;

    std::shared_ptr<connection::IDataLinkInterface> m_dataLinkInterface;
    bool m_connectionLostSent {false};
    std::optional<core::connection::SerialPortInfo> m_lastConnectedUartPort;
    std::optional<connection::EbusDevice> m_lastConnectedEbusDevice;
    connection::IEbusPlugin* m_ebusPlugin {nullptr};

    core::Size m_sizeInPixels;

    std::vector<std::map<PresetAttribute, PropertyId>> m_presetAttributeIds;
    uint8_t m_currentPresetsCount {0};

    std::map<UpdateGroup, std::vector<PropertyId>> m_volatileProperties;
    std::set<PropertyId> m_instantlyVolatileProperties;

    bool m_dependencyValidationIgnoreState{false};
    bool m_oldLoaderUpdateInProgress{false};
};


class PropertiesWtc640::ConnectionInfoTransaction
{
    explicit ConnectionInfoTransaction(const PropertiesTransaction& propertiesTransaction);
    friend class PropertiesWtc640;
public:

    /**
     * @brief Gets the connection statistics.
     * @return The connection statistics.
     */
    const connection::Stats& getConnectionStats() const;

private:
    /**
     * @brief Gets the properties.
     * @return A pointer to the properties.
     */
    PropertiesWtc640* getProperties() const;

    PropertiesTransaction m_propertiesTransaction;

    mutable std::optional<connection::Stats> m_connectionStats;
};


class PropertiesWtc640::ConnectionStateTransaction
{
    explicit ConnectionStateTransaction(const std::shared_ptr<ConnectionStateTransactionData>& connectionTransactionData);
    friend class PropertiesWtc640;
public:

    /**
     * @brief Connects to the UART.
     * @param portInfo The serial port info.
     * @param baudrate The baudrate.
     * @return A void result.
     */
    [[nodiscard]] VoidResult connectUart(const core::connection::SerialPortInfo& portInfo, Baudrate::Item baudrate) const;

    /**
     * @brief Connects to the UART automatically.
     * @param ports The vector of serial port info.
     * @param progressController The progress controller.
     * @return A void result.
     */
    [[nodiscard]] VoidResult connectUartAuto(const std::vector<core::connection::SerialPortInfo>& ports, ProgressController progressController) const;

    /**
     * @brief Connects to the eBus.
     * @param device The eBus device.
     * @return A void result.
     */
    [[nodiscard]] VoidResult connectEbus(const connection::EbusDevice& device) const;



    /**
     * @brief Disconnects the core.
     */
    void disconnectCore() const;

    /**
     * @brief Reconnects the core.
     * @return A void result.
     */
    [[nodiscard]] VoidResult reconnectCore() const;

    /**
     * @brief Reconnects the core after a reset.
     * @param oldBaudrate The old baudrate.
     * @return A void result.
     */
    [[nodiscard]] VoidResult reconnectCoreAfterReset(const std::optional<Baudrate::Item>& oldBaudrate) const;

    /**
     * @brief Gets the current device type.
     * @return The current device type.
     */
    [[nodiscard]] std::optional<DeviceType> getCurrentDeviceType() const;

    /**
     * @brief Gets the current baudrate.
     * @return The current baudrate.
     */
    [[nodiscard]] std::optional<Baudrate::Item> getCurrentBaudrate() const;

    /**
     * @brief Opens a connection exclusive transaction for Wtc640.
     * @return A ConnectionExclusiveTransactionWtc640 object.
     */
    [[nodiscard]] ConnectionExclusiveTransactionWtc640 openConnectionExclusiveTransactionWtc640() const;

private:
    /**
     * @brief Gets the properties.
     * @return A pointer to the properties.
     */
    PropertiesWtc640* getProperties() const;

    /**
     * @brief Sets the data link interface.
     * @param dataLinkInterface The data link interface.
     * @return A void result.
     */
    [[nodiscard]] VoidResult setDataLinkInterface(const std::shared_ptr<connection::IDataLinkInterface>& dataLinkInterface) const;

    std::shared_ptr<ConnectionStateTransactionData> m_connectionStateTransactionData;
};


class PropertiesWtc640::ConnectionExclusiveTransactionWtc640
{
    explicit ConnectionExclusiveTransactionWtc640(const ConnectionExclusiveTransaction& connectionExclusiveTransaction);
    friend class PropertiesWtc640;
public:

    /**
     * @brief Gets the connection exclusive transaction.
     * @return The connection exclusive transaction.
     */
    const ConnectionExclusiveTransaction& getConnectionExclusiveTransaction() const;

    /**
     * @brief Sets the core baudrate.
     * @param baudrate The baudrate to set.
     * @return A void result.
     */
    [[nodiscard]] VoidResult setCoreBaudrate(Baudrate::Item baudrate) const;

    /**
     * @brief Activates a common trigger and waits until it is finished.
     * @param trigger The common trigger to activate.
     * @return A void result.
     */
    [[nodiscard]] VoidResult activateCommonTriggerAndWaitTillFinished(CommonTrigger::Item trigger) const;

    /**
     * @brief Activates a reset trigger and waits until it is finished.
     * @param trigger The reset trigger to activate.
     * @return A void result.
     */
    [[nodiscard]] VoidResult activateResetTriggerAndWaitTillFinished(ResetTrigger::Item trigger) const;

    /**
     * @brief Captures images.
     * @param imagesCount The number of images to capture.
     * @param progressController The progress controller.
     * @return A result containing a vector of captured images.
     */
    [[nodiscard]] ValueResult<std::vector<std::vector<uint16_t>>> captureImages(int imagesCount, ProgressController progressController) const;
    /*!
     * @brief This data is read from RAM, if you want it call getPostProcessingMatrices(), do *NOT* copy/move this struct it cannot guarantee to be kept up to date.
     */

    /**
     * @brief Gets the post-processing matrices.
     * @param progressController The progress controller.
     * @return A result containing the post-processing matrices.
     */
    [[nodiscard]] ValueResult<core::PostProcessingMatrices> getPostProcessingMatrices(ProgressController progressController) const;

    /**
     * @brief Opens a connection state transaction.
     * @return A connection state transaction.
     */
    [[nodiscard]] ConnectionStateTransaction openConnectionStateTransaction() const;

    /**
     * @brief Gets the properties.
     * @return A pointer to the properties.
     */
    [[nodiscard]] PropertiesWtc640* getProperties() const;

    /**
     * @brief Captures images and reads an address.
     * @param framesToCaptureCount The number of frames to capture.
     * @return A result containing the address.
     */
    [[nodiscard]] ValueResult<uint32_t> captureImagesAndReadAddress(uint8_t framesToCaptureCount) const;

    /**
     * @brief Reads the captured frames.
     * @param framesToCaptureCount The number of frames to capture.
     * @param captureAddress The capture address.
     * @param progressController The progress controller.
     * @return A result containing a vector of captured frames.
     */
    [[nodiscard]] ValueResult<std::vector<std::vector<uint16_t>>> readCapturedFrames(uint8_t framesToCaptureCount, uint32_t captureAddress, ProgressController progressController) const;

    /**
     * @brief Activates a trigger.
     * @tparam TriggerType The type of the trigger.
     * @param trigger The trigger to activate.
     * @return A void result.
     */
    template<class TriggerType>
    [[nodiscard]] VoidResult activateTrigger(typename TriggerType::Item trigger) const;

    /**
     * @brief Waits until a trigger is finished.
     * @tparam TriggerType The type of the trigger.
     * @param trigger The trigger to wait for.
     * @return A void result.
     */
    template<class TriggerType>
    [[nodiscard]] VoidResult waitTillTriggerFinished(typename TriggerType::Item trigger) const;

    /**
     * @brief Gets the trigger address range.
     * @tparam TriggerType The type of the trigger.
     * @param trigger The trigger.
     * @return A result containing the address range.
     */
    template<class TriggerType>
    [[nodiscard]] ValueResult<connection::AddressRange> getTriggerAddressRange(typename TriggerType::Item trigger) const;

    ConnectionExclusiveTransaction m_connectionExclusiveTransaction;
};

} // namespace core

#endif // CORE_PROPERTIESWTC640_H
