#include "core/wtc640/propertieswtc640.h"

#include "core/wtc640/propertyadapterlensrange.h"
#include "core/wtc640/firmwarewtc640.h"
#include "core/wtc640/deviceinterfacewtc640.h"
#include "core/wtc640/deadpixels.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/wtc640/videoformatadapter.h"
#include "core/properties/valueadapterusbstringdescriptor.h"
#include "core/properties/properties.inl"
#include "core/properties/propertyadaptervaluederivedfrom1.h"
#include "core/properties/propertyadaptervaluederivedfrom2.h"
#include "core/properties/propertyadaptervaluedevicecomposite.h"
#include "core/properties/propertyadaptervaluecomponent.h"
#include "core/properties/propertydependencyvalidatorfor2.h"
#include "core/connection/datalinkuart.h"
#include "core/connection/protocolinterfacetcsi.h"

#include "core/misc/buffereddatareader.h"
#include "core/misc/elapsedtimer.h"
#include "core/logging.h"
#include "core/prtutils.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/scope_exit.hpp>
#include <boost/polymorphic_cast.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/regex.hpp>

#include <ranges>
#include <algorithm>


namespace core
{

const std::uint16_t WTC_640_VENDOR_IDS[] =
{
    0x04B4, //FX3
    0x0403  //FTDI
};

const std::uint16_t WTC_640_PRODUCT_IDS[] =
{
    0x00F8, 0x00F9, //FX3
    0x6015          //FTDI
};

const std::map<CommonTrigger::Item, EnumValueDescription> CommonTrigger::ALL_ITEMS
{
    {Item::NUC_OFFSET_UPDATE,        {"NUC_OFFSET_UPDATE", "NUC_OFFSET_UPDATE"}},
    {Item::CLEAN_USER_DP,            {"CLEAN_DP", "CLEAN_DP"}},
    {Item::SET_SELECTED_PRESET,      {"SET_SELECTED_PRESET", "SET_SELECTED_PRESET"}},
    {Item::MOTORFOCUS_CALIBRATION,   {"MOTORFOCUS_CALIBRATION", "MOTORFOCUS_CALIBRATION"}},
    {Item::FRAME_CAPTURE_START,      {"FRAME_CAPTURE_START", "FRAME_CAPTURE_START"}},
};

const std::map<ResetTrigger::Item, EnumValueDescription> ResetTrigger::ALL_ITEMS
{
    {Item::RESET_FROM_LOADER,        {"RESET_FROM_LOADER", "RESET_FROM_LOADER"}},
    {Item::SOFTWARE_RESET,           {"SOFTWARE_RESET", "SOFTWARE_RESET"}},
    {Item::RESET_TO_LOADER,          {"RESET_TO_LOADER", "RESET_TO_LOADER"}},
    {Item::RESET_TO_FACTORY_DEFAULT, {"RESET_TO_FACTORY_DEFAULT", "RESET_TO_FACTORY_DEFAULT"}},
    {Item::STAY_IN_LOADER,           {"STAY_IN_LOADER", "STAY_IN_LOADER"}},
};

const std::map<Sensor::Item, std::string> PropertiesWtc640::ARTICLE_NUMBER_SENSORS
{
    { Sensor::Item::PICO_640, "WTC640" },
};

const std::map<Core::Item, std::string> PropertiesWtc640::ARTICLE_NUMBER_CORE_TYPES
{
    { Core::Item::RADIOMETRIC,     "R" },
    { Core::Item::NON_RADIOMETRIC, "N" },
};

const std::map<DetectorSensitivity::Item, std::string> PropertiesWtc640::ARTICLE_NUMBER_DETECTOR_SENSITIVITIES
{
    { DetectorSensitivity::Item::PERFORMANCE_NETD_50MK, "P" },
    { DetectorSensitivity::Item::SUPERIOR_NETD_30MK,    "S" },
    { DetectorSensitivity::Item::ULTIMATE_NETD_30MK,    "U" },
};

const std::map<Focus::Item, std::string> PropertiesWtc640::ARTICLE_NUMBER_FOCUSES
{
    { Focus::Item::MANUAL_H25,               "H25" },
    { Focus::Item::MANUAL_H34,               "H34" },
    { Focus::Item::MOTORIC_E25,              "E25" },
    { Focus::Item::MOTORIC_E34,              "E34" },
    { Focus::Item::MOTORIC_WITH_BAYONET_B25, "B25" },
    { Focus::Item::MOTORIC_WITH_BAYONET_B34, "B34" },
};

const std::map<Framerate::Item, std::string> PropertiesWtc640::ARTICLE_NUMBER_FRAMERATES
{
    { Framerate::Item::FPS_8_57, "9" },
    { Framerate::Item::FPS_30,  "30" },
    { Framerate::Item::FPS_60,  "60" },
};

const std::array<PropertyId, 2>& getDynamicPropertyAdapters()
{
    static const std::array<PropertyId, 2> DYNAMIC_PROPERTY_ADAPTERS
    {
        core::PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION,
        core::PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER,
    };
    return DYNAMIC_PROPERTY_ADAPTERS;
}

PropertiesWtc640::PropertiesWtc640(const std::shared_ptr<connection::IDeviceInterface>& deviceInterface, Mode mode, const std::shared_ptr<IMainThreadIndicator>& indicator, connection::IEbusPlugin* ebusPlugin) :
    BaseClass(deviceInterface, mode, indicator),
    m_ebusPlugin(ebusPlugin)
{
}

std::shared_ptr<PropertiesWtc640> PropertiesWtc640::createInstance(Mode mode, const std::shared_ptr<IMainThreadIndicator>& indicator, connection::IEbusPlugin* ebusPlugin)
{
    const auto connectionStatus = std::make_shared<connection::Status>();
    const auto protocolInterface = std::make_shared<connection::ProtocolInterfaceTCSI>(connectionStatus);
    const auto device = std::make_shared<connection::DeviceInterfaceWtc640>(protocolInterface, connectionStatus);
    device->setMemorySpace(MemorySpaceWtc640::getDeviceSpace(std::nullopt));

    const auto instance = std::shared_ptr<PropertiesWtc640>(new PropertiesWtc640(device, mode, indicator, ebusPlugin));
    instance->m_weakThis = instance;
    instance->createAdapters();

    return instance;
}

connection::IEbusPlugin* PropertiesWtc640::getEbusPlugin() const
{
    return m_ebusPlugin;
}



PropertiesWtc640::ConnectionInfoTransaction PropertiesWtc640::createConnectionInfoTransaction()
{
    return ConnectionInfoTransaction(createPropertiesTransaction());
}

std::optional<PropertiesWtc640::ConnectionInfoTransaction> PropertiesWtc640::tryCreateConnectionInfoTransaction(const std::chrono::steady_clock::duration& timeout)
{
    const auto propertiesTransaction = tryCreatePropertiesTransaction(timeout);
    if (!propertiesTransaction.has_value())
    {
        return std::nullopt;
    }

    return ConnectionInfoTransaction(propertiesTransaction.value());
}

PropertiesWtc640::ConnectionStateTransaction PropertiesWtc640::createConnectionStateTransaction()
{
    const ConnectionStateTransaction stateTransaction(createConnectionStateTransactionData());
    stateTransaction.disconnectCore();

    return stateTransaction;
}

PropertiesWtc640::ConnectionExclusiveTransactionWtc640 PropertiesWtc640::createConnectionExclusiveTransactionWtc640(bool cancelRunningTasks)
{
    return ConnectionExclusiveTransactionWtc640(createConnectionExclusiveTransaction(cancelRunningTasks));
}

const core::Size& PropertiesWtc640::getSizeInPixels(const PropertiesTransaction& transaction) const
{
    assert(transaction.getProperties().get() == this);

    return m_sizeInPixels;
}

std::optional<Baudrate::Item> PropertiesWtc640::getCurrentBaudrate(const PropertiesTransaction& transaction) const
{
    return getCurrentBaudrateImpl();
}

std::optional<connection::SerialPortInfo> PropertiesWtc640::getCurrentPortInfo(const PropertiesTransaction& transaction) const
{
    assert(transaction.getProperties().get() == this);

    if (const auto* datalinkUart = dynamic_cast<const connection::DataLinkUart*>(m_dataLinkInterface.get()))
    {
        return datalinkUart->getPortInfo();
    }

    return std::nullopt;
}

ValueResult<std::shared_ptr<IStream>> PropertiesWtc640::getOrCreateStream(const ConnectionExclusiveTransaction& transaction)
{
    assert(transaction.getPropertiesTransaction().getProperties().get() == this);

    return getOrCreateStreamImpl();
}

ValueResult<std::shared_ptr<IStream>> PropertiesWtc640::getOrCreateStreamImpl()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    if (auto* streamSource = dynamic_cast<IStreamSource*>(m_dataLinkInterface.get()))
    {
        return streamSource->getOrCreateStream();
    }

    return ResultType::createError("Stream source not available!");
}

ValueResult<std::shared_ptr<IStream>> PropertiesWtc640::getStream(const ConnectionExclusiveTransaction& transaction)
{
    assert(transaction.getPropertiesTransaction().getProperties().get() == this);

    return getStreamImpl();
}

ValueResult<std::shared_ptr<IStream>> PropertiesWtc640::getStreamImpl()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    if (auto* streamSource = dynamic_cast<IStreamSource*>(m_dataLinkInterface.get()))
    {
        return streamSource->getStream();
    }

    return ResultType::createError("Stream source not available!");
}

const std::vector<std::map<PropertiesWtc640::PresetAttribute, PropertyId>>& PropertiesWtc640::getPresetAttributeIds(const PropertiesTransaction& transaction) const
{
    return m_presetAttributeIds;
}

std::map<PropertyId, connection::AddressRanges> PropertiesWtc640::getPropertySourceAddressRanges(const PropertiesTransaction& transaction) const
{
    std::map<PropertyId, connection::AddressRanges> propertyAddressRanges;

    for (const auto& [propertyId, adapter] : getPropertyAdapters())
    {
        auto addressRanges = adapter->getAddressRanges();

        for (const auto& sourcePropertyId : adapter->getSourcePropertyIds())
        {
            const auto& sourceAdapter = getPropertyAdapters().at(sourcePropertyId);

            addressRanges = connection::AddressRanges(addressRanges, sourceAdapter->getAddressRanges());
        }

        propertyAddressRanges.emplace(propertyId, addressRanges);
    }

    return propertyAddressRanges;
}

bool PropertiesWtc640::isUpdateGroupChanged(const StatusWtc640& status, const UpdateGroup updateGroup)
{
    switch (updateGroup)
    {
    case UpdateGroup::NUC:
        return status.nucRegistersChanged();

    case UpdateGroup::BOLOMETER:
        return status.bolometerRegistersChanged();

    case UpdateGroup::FOCUS:
        return status.focusRegistersChanged();

    case UpdateGroup::PRESETS:
        return status.presetsRegistersChanged();
    }
    assert(false);
    return false;
}


void PropertiesWtc640::refreshProperties(const std::set<PropertyId>& properties, std::optional<PropertiesTransaction>& transaction)
{
    assert(transaction.has_value());
    if (!getCurrentDeviceType(transaction.value()).has_value())
    {
        return;
    }

    transaction->refreshValue(PropertyIdWtc640::STATUS);

    std::set<PropertyId> changedVolatileProperties = m_instantlyVolatileProperties;

    const auto accumulatedRegisterChanges = getDeviceInterfaceWtc640()->getAccumulatedRegisterChangesAndReset();
    if (accumulatedRegisterChanges.has_value())
    {
        const StatusWtc640 status(accumulatedRegisterChanges.value());
        if (status.isCameraNotReady() || status.getDeviceType() != getCurrentDeviceType(transaction.value()))
        {
            transaction = std::nullopt;

            m_connectionLostSent = true;

            connectionLost();

            return;
        }

        for (const auto updateGroup : {UpdateGroup::NUC, UpdateGroup::BOLOMETER, UpdateGroup::FOCUS, UpdateGroup::PRESETS})
        {
            if (!isUpdateGroupChanged(status, updateGroup))
            {
                continue;
            }
            const auto& properties = m_volatileProperties.at(updateGroup);
            for (const auto& property : properties)
            {
                transaction->resetValue(property);
            }
            changedVolatileProperties.insert(properties.begin(), properties.end());
        }
    }

    auto invalidateIfChanged = [&](PropertyId propertyId)
    {
        if (!changedVolatileProperties.contains(propertyId))
        {
            return false;
        }

        transaction->invalidateValue(propertyId);
        return true;
    };

    for (const auto& propertyId : properties)
    {
        if (!invalidateIfChanged(propertyId))
        {
            const auto& sourceProperties = derefPtr(getPropertyAdapters().at(propertyId)).getSourcePropertyIds();
            for (const auto& sourcePropertyId : sourceProperties)
            {
                invalidateIfChanged(sourcePropertyId);
            }
        }
    }

    for (const auto& propertyToTouch : properties)
    {
        transaction->touch(propertyToTouch);
    }
}

VoidResult PropertiesWtc640::setLensRangeCurrent(PresetId presetId, ProgressController progressController)
{
    return setLensRangeCurrent(core::PropertyIdWtc640::SELECTED_LENS_RANGE_CURRENT, core::PropertyIdWtc640::ACTIVE_LENS_RANGE, presetId, progressController);
}

VoidResult PropertiesWtc640::setLensRangeCurrent(unsigned presetIndex, ProgressController progressController)
{
    return setLensRangeCurrent(core::PropertyIdWtc640::SELECTED_PRESET_INDEX_CURRENT, core::PropertyIdWtc640::CURRENT_PRESET_INDEX, presetIndex, progressController);
}

template<class ValueType>
VoidResult PropertiesWtc640::setLensRangeCurrent(PropertyId propertyId, PropertyId activePropertyId, const ValueType& newValue, ProgressController progressController)
{
    auto task = progressController.createTaskUnbound("Set preset", false);

    const auto transaction = createConnectionExclusiveTransactionWtc640(false);

    const auto activeValueResult = transaction.getConnectionExclusiveTransaction().getPropertiesTransaction().getValue<ValueType>(activePropertyId);
    if (activeValueResult.containsValue() && activeValueResult.getValue() == newValue)
    {
        return core::VoidResult::createOk();
    }

    const auto setValueResult = transaction.getConnectionExclusiveTransaction().getPropertiesTransaction().setValue(propertyId, newValue);
    if (!setValueResult.isOk())
    {
        task.sendErrorMessage(setValueResult.toString());
        return setValueResult;
    }

    const auto setPresetResult = transaction.activateCommonTriggerAndWaitTillFinished(core::CommonTrigger::Item::SET_SELECTED_PRESET);
    if (!setPresetResult.isOk())
    {
        task.sendErrorMessage(setPresetResult.toString());
        return setPresetResult;
    }
    return VoidResult::createOk();
}

VoidResult PropertiesWtc640::resetCore(ProgressController progressController)
{
    auto exclusiveTransaction = createConnectionExclusiveTransactionWtc640(false);

    return resetCoreImpl(ResetTrigger::Item::SOFTWARE_RESET, "Resetting core...",
                         std::nullopt,
                         progressController,
                         exclusiveTransaction);
}

VoidResult PropertiesWtc640::resetToFactoryDefault(ProgressController progressController)
{
    auto exclusiveTransaction = createConnectionExclusiveTransactionWtc640(false);

    return resetCoreImpl(ResetTrigger::Item::RESET_TO_FACTORY_DEFAULT, "Resetting to factory default...",
                         std::nullopt,
                         progressController,
                         exclusiveTransaction);
}



VoidResult PropertiesWtc640::resetCoreImpl(ResetTrigger::Item trigger, const std::string& taskName,
                                           const std::optional<Baudrate::Item>& oldBaudrate,
                                           ProgressController progressController,
                                           ConnectionExclusiveTransactionWtc640& exclusiveTransaction)
{
    std::optional<PropertiesWtc640::ConnectionStateTransaction> stateTransaction;
    {
        if(auto stream = getStreamImpl(); stream.isOk())
        {
            if(auto stopStream = stream.getValue()->stopStream(); !stopStream.isOk())
            {
                WW_LOG_PROPERTIES_FATAL << "Failed to stop stream while running reset trigger! error - " << stopStream.toString();
            }
        }
    }

    {
        auto task = progressController.createTaskUnbound(taskName, false);

        if (const auto resetResult = exclusiveTransaction.activateResetTriggerAndWaitTillFinished(trigger); !resetResult.isOk())
        {
            task.sendErrorMessage(resetResult.toString());
            return resetResult;
        }

        stateTransaction = exclusiveTransaction.openConnectionStateTransaction();
    }

    {
        auto task = progressController.createTaskUnbound("Resetting core...", false);

        if (m_lastConnectedEbusDevice.has_value())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(3'000));
        }


        if (const auto reconnectResult = stateTransaction->reconnectCoreAfterReset(oldBaudrate); !reconnectResult.isOk())
        {
            task.sendErrorMessage(reconnectResult.toString());
            return reconnectResult;
        }

        task.sendProgressMessage("Successful reset.");
    }

    return VoidResult::createOk();
}



VoidResult PropertiesWtc640::updateFirmware(const FirmwareWtc640& firmware, ProgressController progressController)
{
    std::optional<Baudrate::Item> oldBaudrate;
    std::optional<DeviceType> deviceType;
    {
        const auto transaction = createPropertiesTransaction();

        oldBaudrate = getCurrentBaudrate(transaction);
        deviceType = getCurrentDeviceType(transaction);
    }

    auto transaction = createConnectionExclusiveTransactionWtc640(false);
    if (deviceType != DevicesWtc640::LOADER)
    {
        const auto result = resetCoreImpl(ResetTrigger::Item::RESET_TO_LOADER, "Restarting to loader...",
                                          oldBaudrate,
                                          progressController,
                                          transaction);
        if (!result.isOk())
        {
            return result;
        }
    }

    if (!m_oldLoaderUpdateInProgress)
    {
        const auto trigger = ResetTrigger::Item::STAY_IN_LOADER;
        if(const auto result = resetCoreImpl(trigger, "Resetting loader...", oldBaudrate, progressController, transaction); !result.isOk())
        {
            progressController.sendErrorMessage(result.toString());
        }
    }


    const auto& exclusiveTransaction = transaction.getConnectionExclusiveTransaction();

    if (getCurrentDeviceType(exclusiveTransaction.getPropertiesTransaction()) != DevicesWtc640::LOADER)
    {
        const auto RESULT = VoidResult::createError("Unable to connect to loader!");
        progressController.sendErrorMessage(RESULT.toString());
        return RESULT;
    }

    for(const auto& item : firmware.getUpdateData())
    {
        {
            auto progress = progressController.createTaskBound("Updating part of firmware, please do not close the application or disconnect the device.", item.data.size(), false);

            if (const auto result = exclusiveTransaction.writeDataWithProgress<uint8_t>(item.data, item.startAddress, progress); !result.isOk())
            {
                progress.sendErrorMessage(result.toString());
                return result;
            }
        }

        {
            auto progress = progressController.createTaskBound("Checking updated part of firmware, please do not close the application or disconnect the device.", item.data.size(), false);

            const auto writtenFirmwareData = exclusiveTransaction.readDataWithProgress<uint8_t>(item.startAddress, item.data.size(), progress);
            if (!writtenFirmwareData.isOk())
            {
                progress.sendErrorMessage(writtenFirmwareData.toString());
                return writtenFirmwareData.toVoidResult();
            }

            if (writtenFirmwareData.getValue() != item.data)
            {
                const auto result = VoidResult::createError("Incorrect data uploaded!", "possibly flash memory corrupted");
                progress.sendErrorMessage(result.toString());
                return result;
            }
        }
    }

    core::Version loaderVersion(0, 0, 0);
    if(auto value = transaction.getConnectionExclusiveTransaction().getPropertiesTransaction().getValue<core::Version>(core::PropertyIdWtc640::LOADER_FIRMWARE_VERSION); value.containsValue())
    {
        loaderVersion = value.getValue();
        if((loaderVersion >= core::Version(2, 1, 8) && firmware.getFirmwareType() == core::FirmwareType::Item::ALL) || m_oldLoaderUpdateInProgress) //the trigger below is only for loaders >= 2.1.8
        {
           static_cast<void>(resetCoreImpl(ResetTrigger::Item::RESET_TO_LOADER, "Restarting core...", oldBaudrate, progressController, transaction));
        }
    }

    return resetCoreImpl(ResetTrigger::Item::RESET_FROM_LOADER, "Restarting core...",
                         oldBaudrate,
                         progressController,
                         transaction);
}

VoidResult PropertiesWtc640::resetFromLoader(ProgressController progressController)
{
    auto transaction = createConnectionExclusiveTransactionWtc640(false);

    return resetCoreImpl(ResetTrigger::Item::RESET_FROM_LOADER, "Restarting core...",
                         std::nullopt,
                         progressController,
                         transaction);
}

const bool& PropertiesWtc640::getDependencyValidationIgnoreState()
{
    return m_dependencyValidationIgnoreState;
}

void PropertiesWtc640::setDepedencyValidationIgnoreState(const bool& state)
{
    m_dependencyValidationIgnoreState = state;
}

bool PropertiesWtc640::isValidVideoFormat(Plugin::Item pluginType, VideoFormat::Item videoFormat)
{
    assert(VideoFormat::ALL_ITEMS.size() == 3 && "video formats changed - update this code!");
    switch (pluginType)
    {
        case Plugin::Item::USB:
            return (videoFormat == VideoFormat::Item::PRE_IGC || videoFormat == VideoFormat::Item::POST_COLORING);

        case Plugin::Item::PLEORA:
        case Plugin::Item::CMOS:
            return (videoFormat == VideoFormat::Item::PRE_IGC || videoFormat == VideoFormat::Item::POST_IGC);

        case Plugin::Item::CVBS:
        case Plugin::Item::HDMI:
        case Plugin::Item::ANALOG:
            return (videoFormat == VideoFormat::Item::POST_COLORING);

        case Plugin::Item::ONVIF:
            return (videoFormat == VideoFormat::Item::PRE_IGC);

        default:
            assert(false && "unknown firmware!");
            return false;
    }
}

void PropertiesWtc640::createAdapters()
{
    addControlAdapters();
    m_instantlyVolatileProperties.insert(PropertyIdWtc640::STATUS);

    addGeneralAdapters();
    m_instantlyVolatileProperties.insert(PropertyIdWtc640::SHUTTER_TEMPERATURE);

    addNUCAdapters();
    m_volatileProperties.emplace(UpdateGroup::NUC, std::vector<PropertyId>{PropertyIdWtc640::INTERNAL_SHUTTER_POSITION});
    m_instantlyVolatileProperties.insert(PropertyIdWtc640::TIME_FROM_LAST_NUC_OFFSET_UPDATE);

    addConnectionAdapters();

    addVideoAdapters();

    addFiltersAdapters();

    addDPRAdapters();

    addFocusAdapters();
    m_volatileProperties.emplace(UpdateGroup::FOCUS, std::vector<PropertyId>{PropertyIdWtc640::LENS_SERIAL_NUMBER, PropertyIdWtc640::LENS_ARTICLE_NUMBER});
    m_instantlyVolatileProperties.insert(PropertyIdWtc640::CURRENT_MF_POSITION);

    addPresetsAdapters();
    m_volatileProperties.emplace(UpdateGroup::PRESETS, std::vector<PropertyId>{PropertyIdWtc640::CURRENT_PRESET_INDEX});

    addMotorFocusConstraints();
    addImageFreezeConstraints();
    addConnectionConstraints();
    addPluginConstraints();

    const auto& adapters = getPropertyAdapters();
    for (const auto& propertyId : PropertyId::getAllPropertyIds())
    {
        if (std::find(getDynamicPropertyAdapters().begin(), getDynamicPropertyAdapters().end(), propertyId) == getDynamicPropertyAdapters().end())
        {
            assert(adapters.find(propertyId) != adapters.end() && "Missing property adapter!");
        }
    }
}

void PropertiesWtc640::addControlAdapters()
{
    {
        const auto propertyId = PropertyIdWtc640::STATUS;
        const auto addressRange = MemorySpaceWtc640::STATUS;

        auto reader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<StatusWtc640>
        {
            const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
            if (!result.isOk())
            {
                return ValueResult<StatusWtc640>::createFromError(result);
            }

            assert(result.getValue().size() == 1);
            return StatusWtc640(result.getValue().front());
        };

        auto valueAdapter = std::make_shared<PropertyValue<StatusWtc640>>(propertyId);
        valueAdapter->setCustomConvertToStringFunction([](const StatusWtc640& value)
        {
            return value.toString();
        });

        addValueAdapter(valueAdapter,
                        std::make_shared<PropertyAdapterValueDeviceSimple<StatusWtc640>>(
                            propertyId,
                            createStatusFunction(DeviceFlags::ALL_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                            AdapterTaskCreator(m_weakThis), addressRange,
                            reader,
                            nullptr));
    }

    {
        const auto propertyId = PropertyIdWtc640::LOGIN_ROLE;

        const auto statusAdapter = std::dynamic_pointer_cast<PropertyAdapterValue<StatusWtc640>>(getPropertyAdapters().at(PropertyIdWtc640::STATUS));

        const auto deviceTypeAdapter = std::make_shared<PropertyAdapterValueDerivedFrom1<LoginRole::Item, StatusWtc640>>(
                    propertyId, createStatusFunction(DeviceFlags::ALL_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                    getPropertyValues(), statusAdapter,
                    [=](const OptionalResult<StatusWtc640>& status, const PropertyValues::Transaction& ) -> OptionalResult<LoginRole::Item>
                    {
                        using ResultType = OptionalResult<LoginRole::Item>;

                        if (status.containsError())
                        {
                            return ResultType::createError("Status error!", "");
                        }
                        else if (status.containsValue())
                        {
                            if (status.getValue().getDeviceType().has_value())
                            {
                                const auto deviceType = status.getValue().getDeviceType().value();
                                if (deviceType == DevicesWtc640::LOADER)
                                {
                                    return LoginRole::Item::LOADER;
                                }
                                
                                else if (deviceType == DevicesWtc640::MAIN_USER)
                                {
                                    return LoginRole::Item::USER;
                                }
                                else
                                {
                                    assert(false);
                                }
                            }

                            return LoginRole::Item::NONE;
                        }

                        return std::nullopt;
                    });

        addValueAdapter(createPropertyValueEnum<LoginRole>(propertyId), deviceTypeAdapter);
    }
}

void PropertiesWtc640::addGeneralAdapters()
{
    addVersionAdapter(PropertyIdWtc640::MAIN_FIRMWARE_VERSION, MemorySpaceWtc640::MAIN_FIRMWARE_VERSION);

    addVersionAdapter(PropertyIdWtc640::LOADER_FIRMWARE_VERSION, MemorySpaceWtc640::LOADER_FIRMWARE_VERSION);

    addEnumDeviceValueSimpleAdapter<Plugin>(PropertyIdWtc640::PLUGIN_TYPE, MemorySpaceWtc640::PLUGIN_TYPE,
                                                  DeviceFlags::ALL_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE);

    addEnumDeviceValueSimpleAdapter<FirmwareType>(PropertyIdWtc640::MAIN_FIRMWARE_TYPE, MemorySpaceWtc640::MAIN_FIRMWARE_TYPE,
                                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE);

    addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyIdWtc640::FPGA_BOARD_TEMPERATURE, MemorySpaceWtc640::FPGA_BOARD_TEMPERATURE, DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE, true);
    addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyIdWtc640::SHUTTER_TEMPERATURE, MemorySpaceWtc640::SHUTTER_TEMPERATURE, DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE, true);

    addCoreSerialNumberAdapters();
    addArticleNumberAdapters();

    const auto addLedBrightnessAdapters = [this](PropertyId propertyIdCurrent, const AddressRange& addressRangeCurrent,
                                                 PropertyId propertyIdInFlash, const AddressRange& addressRangeInFlash,
                                                 int min, int max)
    {
        static constexpr unsigned MASK = 0b111;

        addUnsignedDeviceValueSimpleAdapterArithmetic(propertyIdCurrent, addressRangeCurrent, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      min, max);

        addUnsignedDeviceValueSimpleAdapterArithmetic(propertyIdInFlash, addressRangeInFlash, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      min, max);
    };

    addLedBrightnessAdapters(PropertyIdWtc640::LED_R_BRIGHTNESS_CURRENT, MemorySpaceWtc640::LED_R_BRIGHTNESS_CURRENT,
                             PropertyIdWtc640::LED_R_BRIGHTNESS_IN_FLASH, MemorySpaceWtc640::LED_R_BRIGHTNESS_IN_FLASH,
                             1, 7);

    addLedBrightnessAdapters(PropertyIdWtc640::LED_G_BRIGHTNESS_CURRENT, MemorySpaceWtc640::LED_G_BRIGHTNESS_CURRENT,
                             PropertyIdWtc640::LED_G_BRIGHTNESS_IN_FLASH, MemorySpaceWtc640::LED_G_BRIGHTNESS_IN_FLASH,
                             0, 7);

    addLedBrightnessAdapters(PropertyIdWtc640::LED_B_BRIGHTNESS_CURRENT, MemorySpaceWtc640::LED_B_BRIGHTNESS_CURRENT,
                             PropertyIdWtc640::LED_B_BRIGHTNESS_IN_FLASH, MemorySpaceWtc640::LED_B_BRIGHTNESS_IN_FLASH,
                             0, 7);
}

void PropertiesWtc640::addVideoAdapters()
{
    addPalettesAdapters();

    const auto addFramerateAdapter = [this](PropertyId frameratePropertyId, const AddressRange& addressRange, PropertyId maxFrameratePropertyId)
    {
        addEnumDeviceValueSimpleAdapter<Framerate>(frameratePropertyId, addressRange,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        auto framerateMaxFramerateValidation = [](const OptionalResult<Framerate::Item>& framerate, const OptionalResult<Framerate::Item>& maxFramerate)
        {
            if (framerate.containsValue() && framerate == Framerate::Item::FPS_8_57)
            {
                return RankedValidationResult::createOk();
            }

            if (!maxFramerate.hasResult() || !framerate.hasResult())
            {
                return RankedValidationResult::createDataForValidationNotReady(!maxFramerate.hasResult() ? "unknown max framerate" : "unknown framerate");
            }

            if (maxFramerate.containsError())
            {
                return RankedValidationResult::createError("Invalid max framerate!");
            }

            if (framerate.containsError())
            {
                return RankedValidationResult::createOk(); // necessary - if both values are invalid and someone is trying to set valid maxFramerate
            }

            const auto framerateValue = Framerate::getFramerateValue(framerate.getValue());
            const auto maxFramerateValue = Framerate::getFramerateValue(maxFramerate.getValue());
            if (framerateValue > maxFramerateValue)
            {
                return RankedValidationResult::createError("Invalid framerate!", utils::format("value: {} max: {}", framerateValue, maxFramerateValue));
            }

            return RankedValidationResult::createOk();
        };

        addPropertyDependencyValidator(std::make_shared<PropertyDependencyValidatorFor2<Framerate::Item, Framerate::Item>>(
            frameratePropertyId, maxFrameratePropertyId, framerateMaxFramerateValidation,
            getPropertyValues(), std::bind(&PropertiesWtc640::getDependencyValidationIgnoreState, this)));
};

    auto fpsLockMaxFramerateValidation = [](const OptionalResult<bool>& fpsLock, const OptionalResult<Framerate::Item>& maxFramerate)
    {
        if (maxFramerate.containsValue() && maxFramerate.getValue() == Framerate::Item::FPS_8_57)
        {
            return RankedValidationResult::createOk();
        }

        if (!maxFramerate.hasResult() || !fpsLock.hasResult())
        {
            return RankedValidationResult::createDataForValidationNotReady(!maxFramerate.hasResult() ? "unknown max framerate" : "unknown fps lock");
        }

        if (maxFramerate.containsError() || fpsLock.containsError())
        {
            return RankedValidationResult::createError(maxFramerate.containsError() ? "Invalid max framerate!" : "Invalid FPS lock!");
        }

        const auto fpsLockValue = fpsLock.getValue();
        const auto maxFramerateValue = Framerate::getFramerateValue(maxFramerate.getValue());
        if (fpsLockValue && maxFramerate.getValue() != Framerate::Item::FPS_8_57)
        {
            return RankedValidationResult::createError("Cannot set higher framerate while lock is active!", utils::format("FPS_LOCK is true but framerate value is {}", maxFramerateValue));
        }

        return RankedValidationResult::createOk();
    };

    addFramerateAdapter(PropertyIdWtc640::FRAMERATE_CURRENT, MemorySpaceWtc640::FRAME_RATE_CURRENT, PropertyIdWtc640::MAX_FRAMERATE_CURRENT);
    addFramerateAdapter(PropertyIdWtc640::FRAMERATE_IN_FLASH, MemorySpaceWtc640::FRAME_RATE_IN_FLASH, PropertyIdWtc640::MAX_FRAMERATE_IN_FLASH);

    addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::FPS_LOCK, MemorySpaceWtc640::FPS_LOCK, 0b1,
                                    DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    addPropertyDependencyValidator(std::make_shared<PropertyDependencyValidatorFor2<bool, Framerate::Item>>(
        PropertyIdWtc640::FPS_LOCK, PropertyIdWtc640::MAX_FRAMERATE_IN_FLASH, fpsLockMaxFramerateValidation,
        getPropertyValues(), std::bind(&PropertiesWtc640::getDependencyValidationIgnoreState, this)));

    addImageFlipAdapters(PropertyIdWtc640::IMAGE_FLIP_CURRENT, PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_CURRENT, PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_CURRENT,
                         MemorySpaceWtc640::IMAGE_FLIP_CURRENT);

    addImageFlipAdapters(PropertyIdWtc640::IMAGE_FLIP_IN_FLASH, PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_IN_FLASH, PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_IN_FLASH,
                         MemorySpaceWtc640::IMAGE_FLIP_IN_FLASH);

    addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::IMAGE_FREEZE, MemorySpaceWtc640::IMAGE_FREEZE, 0b1,
                                    DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyIdWtc640::GAMMA_CORRECTION_CURRENT, MemorySpaceWtc640::GAMMA_CORRECTION_CURRENT,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0.25, 2, 5, 2, 0.25, 4.0);
    addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyIdWtc640::GAMMA_CORRECTION_IN_FLASH, MemorySpaceWtc640::GAMMA_CORRECTION_IN_FLASH,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0.25, 2, 5, 2, 0.25, 4.0);

    addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyIdWtc640::MAX_AMPLIFICATION_CURRENT, MemorySpaceWtc640::MAX_AMPLIFICATION_CURRENT,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0.125, 3, 6, 0, 0.25, 4.0);
    addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyIdWtc640::MAX_AMPLIFICATION_IN_FLASH, MemorySpaceWtc640::MAX_AMPLIFICATION_IN_FLASH,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0.125, 3, 6, 0, 0.25, 4.0);

    auto validationFunction = [this](ImageGenerator::Item value)
    {
        const auto device = getDeviceType();
        if (device.has_value())
        {
            switch (value)
            {
                case ImageGenerator::Item::SENSOR:
                case ImageGenerator::Item::ADC_1:
                case ImageGenerator::Item::INTERNAL_DYNAMIC:
                    return VoidResult::createOk();

                default:
                    return VoidResult::createError("Invalid value!", utils::format("in user mode value: {} is invalid", static_cast<int>(value)));
            }
        }

        return VoidResult::createOk();
    };

    addEnumDeviceValueSimpleAdapter<ImageGenerator>(PropertyIdWtc640::TEST_PATTERN, MemorySpaceWtc640::TEST_PATTERN,
                                                          DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                          validationFunction);

    const auto addVideoFormatAdapter = [this](const PropertyId& videoFormatPropertyId, const AddressRange& addressRange)
    {
        addEnumDeviceValueSimpleAdapter<VideoFormat>(videoFormatPropertyId, addressRange,
                                                     DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        auto videoFormatFirmwareTypeValidation = [](const OptionalResult<Plugin::Item>& pluginType, const OptionalResult<VideoFormat::Item>& videoFormat)
        {
            if (!videoFormat.hasResult() || !pluginType.hasResult())
            {
                return RankedValidationResult::createDataForValidationNotReady(videoFormat.hasResult() ? "unknown video format" : "unknown firmware type");
            }

            if (videoFormat.containsError() || pluginType.containsError())
            {
                return RankedValidationResult::createError(videoFormat.containsError() ? "Invalid video format!" : "Invalid firmware type!");
            }

            const auto videoFormatValue = videoFormat.getValue();
            const auto pluginTypeValue = pluginType.getValue();

            if (!isValidVideoFormat(pluginTypeValue, videoFormatValue))
            {
                return RankedValidationResult::createError(utils::format("Invalid video format!"), utils::format("video format: {} plugin type: {}", static_cast<int>(videoFormatValue), static_cast<int>(pluginTypeValue)));
            }

            return RankedValidationResult::createOk();
        };

        addPropertyDependencyValidator(std::make_shared<PropertyDependencyValidatorFor2<Plugin::Item, VideoFormat::Item>>(
            PropertyIdWtc640::PLUGIN_TYPE, videoFormatPropertyId, videoFormatFirmwareTypeValidation,
            getPropertyValues(), std::bind(&PropertiesWtc640::getDependencyValidationIgnoreState, this)));
    };

    addVideoFormatAdapter(PropertyIdWtc640::VIDEO_FORMAT_CURRENT, MemorySpaceWtc640::VIDEO_FORMAT_CURRENT);
    addVideoFormatAdapter(PropertyIdWtc640::VIDEO_FORMAT_IN_FLASH, MemorySpaceWtc640::VIDEO_FORMAT_IN_FLASH);

    addEnumDeviceValueSimpleAdapter<ReticleMode>(PropertyIdWtc640::RETICLE_MODE_CURRENT, MemorySpaceWtc640::RETICLE_MODE_CURRENT, DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    addEnumDeviceValueSimpleAdapter<ReticleMode>(PropertyIdWtc640::RETICLE_MODE_IN_FLASH, MemorySpaceWtc640::RETICLE_MODE_IN_FLASH, DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    addSignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_CURRENT, MemorySpaceWtc640::CROSS_SHIFT_X_AXIS_CURRENT, 0xFFFF,
                                                DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                -200, 200);
    addSignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_IN_FLASH, MemorySpaceWtc640::CROSS_SHIFT_X_AXIS_IN_FLASH, 0xFFFF,
                                                DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                -200, 200);
    addSignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_CURRENT, MemorySpaceWtc640::CROSS_SHIFT_Y_AXIS_CURRENT, 0xFFFF,
                                                DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                -100, 100);
    addSignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_IN_FLASH, MemorySpaceWtc640::CROSS_SHIFT_Y_AXIS_IN_FLASH, 0xFFFF,
                                                DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                -100, 100);
}

void PropertiesWtc640::addNUCAdapters()
{
    {
        const uint32_t SHUTTER_MASK = (1 << 11) - 1;
        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::TIME_FROM_LAST_NUC_OFFSET_UPDATE, MemorySpaceWtc640::TIME_FROM_LAST_NUC_OFFSET_UPDATE, SHUTTER_MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                                      0, SHUTTER_MASK);
    }

    {
        addEnumDeviceValueSimpleAdapter<ShutterUpdateMode>(PropertyIdWtc640::NUC_UPDATE_MODE_CURRENT, MemorySpaceWtc640::NUC_UPDATE_MODE_CURRENT,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addEnumDeviceValueSimpleAdapter<ShutterUpdateMode>(PropertyIdWtc640::NUC_UPDATE_MODE_IN_FLASH, MemorySpaceWtc640::NUC_UPDATE_MODE_IN_FLASH,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }

    addEnumDeviceValueSimpleAdapter<InternalShutterState>(PropertyIdWtc640::INTERNAL_SHUTTER_POSITION, MemorySpaceWtc640::INTERNAL_SHUTTER_POSITION,
                                    DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    {
        const uint32_t PERIOD_MASK = 0xFFFF;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::NUC_MAX_PERIOD_CURRENT, MemorySpaceWtc640::NUC_MAX_PERIOD_CURRENT, PERIOD_MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      120, 7200);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::NUC_MAX_PERIOD_IN_FLASH, MemorySpaceWtc640::NUC_MAX_PERIOD_IN_FLASH, PERIOD_MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      120, 7200);
    }

    addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_CURRENT, MemorySpaceWtc640::NUC_ADAPTIVE_THRESHOLD_CURRENT,
                                                 DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                 false, 0.25, 10.0);
    addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_IN_FLASH, MemorySpaceWtc640::NUC_ADAPTIVE_THRESHOLD_IN_FLASH,
                                                 DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                 false, 0.25, 10.0);
}

void PropertiesWtc640::addConnectionAdapters()
{
    addEnumDeviceValueSimpleAdapter<BaudrateWtc>(PropertyIdWtc640::UART_BAUDRATE_CURRENT, MemorySpaceWtc640::UART_BAUDRATE_CURRENT,
                                                 DeviceFlags::ALL_640, ModeFlags::USER, DeviceFlags::ALL_640, ModeFlags::USER);

    addEnumDeviceValueSimpleAdapter<BaudrateWtc>(PropertyIdWtc640::UART_BAUDRATE_IN_FLASH, MemorySpaceWtc640::UART_BAUDRATE_IN_FLASH,
                                                 DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::BOOT_TO_LOADER_IN_FLASH, MemorySpaceWtc640::BOOT_TO_LOADER_IN_FLASH, 0b1,
                                    DeviceFlags::LOADER_640, ModeFlags::USER, DeviceFlags::LOADER_640, ModeFlags::USER);
}

void PropertiesWtc640::addFiltersAdapters()
{
    {
        addEnumDeviceValueSimpleAdapter<TimeDomainAveraging>(PropertyIdWtc640::TIME_DOMAIN_AVERAGE_CURRENT, MemorySpaceWtc640::TIME_DOMAIN_AVERAGE_CURRENT,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addEnumDeviceValueSimpleAdapter<TimeDomainAveraging>(PropertyIdWtc640::TIME_DOMAIN_AVERAGE_IN_FLASH, MemorySpaceWtc640::TIME_DOMAIN_AVERAGE_IN_FLASH,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }

    {
        addEnumDeviceValueSimpleAdapter<ImageEqualizationType>(PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_CURRENT, MemorySpaceWtc640::IMAGE_EQUALIZATION_TYPE_CURRENT,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addEnumDeviceValueSimpleAdapter<ImageEqualizationType>(PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_IN_FLASH, MemorySpaceWtc640::IMAGE_EQUALIZATION_TYPE_IN_FLASH,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }

    {
        const uint32_t MASK = 0b1111;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::LINEAR_GAIN_WEIGHT_CURRENT, MemorySpaceWtc640::LINEAR_GAIN_WEIGHT_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      LINEAR_GAIN_WEIGHT_MIN_VALUE, LINEAR_GAIN_WEIGHT_MAX_VALUE);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::LINEAR_GAIN_WEIGHT_IN_FLASH, MemorySpaceWtc640::LINEAR_GAIN_WEIGHT_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      LINEAR_GAIN_WEIGHT_MIN_VALUE, LINEAR_GAIN_WEIGHT_MAX_VALUE);
    }

    {
        static constexpr unsigned CLIP_LIMIT_MIN_VALUE = 1;
        static constexpr unsigned CLIP_LIMIT_MAX_VALUE = 100;

        const uint32_t MASK = 0b1111'111;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::CLIP_LIMIT_CURRENT, MemorySpaceWtc640::CLIP_LIMIT_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      CLIP_LIMIT_MIN_VALUE, CLIP_LIMIT_MAX_VALUE);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::CLIP_LIMIT_IN_FLASH, MemorySpaceWtc640::CLIP_LIMIT_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      CLIP_LIMIT_MIN_VALUE, CLIP_LIMIT_MAX_VALUE);
    }

    {
        static constexpr unsigned PLATEAU_TAIL_REJECTION_MIN_VALUE = 0;
        static constexpr unsigned PLATEAU_TAIL_REJECTION_MAX_VALUE = 49;

        const uint32_t MASK = 0b1111'11;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::PLATEAU_TAIL_REJECTION_CURRENT, MemorySpaceWtc640::PLATEAU_TAIL_REJECTION_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      PLATEAU_TAIL_REJECTION_MIN_VALUE, PLATEAU_TAIL_REJECTION_MAX_VALUE);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::PLATEAU_TAIL_REJECTION_IN_FLASH, MemorySpaceWtc640::PLATEAU_TAIL_REJECTION_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      PLATEAU_TAIL_REJECTION_MIN_VALUE, PLATEAU_TAIL_REJECTION_MAX_VALUE);
    }

    {
        static constexpr unsigned SMART_MEDIAN_THRESHOLD_MIN_VALUE = 0;
        static constexpr unsigned SMART_MEDIAN_THRESHOLD_MAX_VALUE = 31;

        const uint32_t MASK = 0b1111'1;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_CURRENT, MemorySpaceWtc640::SMART_MEDIAN_THRESHOLD_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      SMART_MEDIAN_THRESHOLD_MIN_VALUE, SMART_MEDIAN_THRESHOLD_MAX_VALUE);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_IN_FLASH, MemorySpaceWtc640::SMART_MEDIAN_THRESHOLD_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      SMART_MEDIAN_THRESHOLD_MIN_VALUE, SMART_MEDIAN_THRESHOLD_MAX_VALUE);
    }

    {
        static constexpr unsigned SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MIN_VALUE = 0;
        static constexpr unsigned SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MAX_VALUE = 31;

        const uint32_t MASK = 0b1111'1;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT, MemorySpaceWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MIN_VALUE, SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MAX_VALUE);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH, MemorySpaceWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MIN_VALUE, SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_MAX_VALUE);
    }

    auto addConbrightAdapters = [this](PropertyId compositeProperty, const AddressRange& addressRange,
            PropertyId contrastProperty, PropertyId brightnessProperty,
            DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags)
    {
        const auto statusFunction = createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags);

        constexpr uint32_t MASK = (1 << 14) - 1;

        auto compositeReader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<Conbright>
        {
            const auto data = device->readTypedDataFromRange<uint16_t>(addressRange, ProgressTask());
            if (!data.isOk())
            {
                return ValueResult<Conbright>::createFromError(data);
            }
            assert(data.getValue().size() == 2);

            Conbright conbright;

            conbright.contrast   = (data.getValue().at(0) & MASK);
            conbright.brightness = (data.getValue().at(1) & MASK);

            return conbright;
        };

        auto compositeWriter = [addressRange](connection::IDeviceInterface* device, const Conbright& conbright) -> VoidResult
        {
            if (!conbright.contrast.isOk())
            {
                return conbright.contrast.toVoidResult();
            }

            if (!conbright.brightness.isOk())
            {
                return conbright.brightness.toVoidResult();
            }

            std::vector<uint16_t> data(2, 0);
            data.at(0) = (conbright.contrast.getValue() & MASK);
            data.at(1) = (conbright.brightness.getValue() & MASK);

            return device->writeTypedData<uint16_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };

        using CompositeAdapterType = PropertyAdapterValueDeviceComposite<Conbright, PropertyAdapterValueDeviceSimple>;
        const auto compositeAdapter = std::make_shared<CompositeAdapterType>(
                                          compositeProperty, statusFunction,
                                          AdapterTaskCreator(m_weakThis), addressRange,
                                          compositeReader,
                                          compositeWriter);

        auto valueAdapter = std::make_shared<PropertyValue<Conbright>>(compositeProperty);
        valueAdapter->setCustomConvertToStringFunction([](const Conbright& value)
        {
            return createCompositeValueString(
                        {
                            {"contrast", value.contrast.toVoidResult()},
                            {"brightness", value.brightness.toVoidResult()},
                        });
        });

        addValueAdapter(valueAdapter, compositeAdapter);

        addValueAdapter(std::make_shared<PropertyValueArithmetic<unsigned>>(contrastProperty, 0, MASK),
                        std::make_shared<PropertyAdapterValueComponent<unsigned, CompositeAdapterType>>(
                            contrastProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &Conbright::contrast,
                            0));

        addValueAdapter(std::make_shared<PropertyValueArithmetic<unsigned>>(brightnessProperty, 0, MASK),
                        std::make_shared<PropertyAdapterValueComponent<unsigned, CompositeAdapterType>>(
                            brightnessProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &Conbright::brightness,
                            0));
    };
    addConbrightAdapters(PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_CURRENT, MemorySpaceWtc640::MGC_CONTRAST_BRIGHTNESS_CURRENT,
                         PropertyIdWtc640::MGC_CONTRAST_CURRENT, PropertyIdWtc640::MGC_BRIGHTNESS_CURRENT,
                         DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    addConbrightAdapters(PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_IN_FLASH, MemorySpaceWtc640::MGC_CONTRAST_BRIGHTNESS_IN_FLASH,
                         PropertyIdWtc640::MGC_CONTRAST_IN_FLASH, PropertyIdWtc640::MGC_BRIGHTNESS_IN_FLASH,
                         DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    addConbrightAdapters(PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONBRIGHT, MemorySpaceWtc640::FRAME_BLOCK_MEDIAN_CONBRIGHT,
                         PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONTRAST, PropertyIdWtc640::FRAME_BLOCK_MEDIAN_BRIGHTNESS,
                         DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    {
        const uint32_t MASK = 0b111;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::AGC_NH_SMOOTHING_CURRENT, MemorySpaceWtc640::AGC_NH_SMOOTHING_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      0, 4);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::AGC_NH_SMOOTHING_IN_FLASH, MemorySpaceWtc640::AGC_NH_SMOOTHING_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      0, 4);
    }

    {
        const uint32_t MASK = 0b1111111;

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::PLATEAU_SMOOTHING_CURRENT, MemorySpaceWtc640::PLATEAU_SMOOTHING_CURRENT, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      0, 100);

        addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::PLATEAU_SMOOTHING_IN_FLASH, MemorySpaceWtc640::PLATEAU_SMOOTHING_IN_FLASH, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      0, 100);
    }

    {
        static constexpr uint32_t MASK = 0b1;

        addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT, MemorySpaceWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT, MASK,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH, MemorySpaceWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH, MASK,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }
}

void PropertiesWtc640::addDPRAdapters()
{
    {
        auto createDeadPixelsWithReplacementsAdapter = [this](PropertyId property, const AdapterTaskCreator& taskCreator,
                const AddressRange& deadPixelsAddressRange, const AddressRange& replacementsAddressRange)
        {
            const auto sizeInPixels = DeadPixels().getResolutionInPixels();

            auto deadPixelsReader = [=](connection::IDeviceInterface* device, ProgressController progressController) -> ValueResult<DeadPixels>
            {
                using ReturnType = ValueResult<DeadPixels>;

                auto readDataFunction = [device](uint32_t address)
                {
                    return device->readSomeData(address, ProgressTask());
                };

                std::vector<DeadPixel> deserializedDeadPixels;
                std::vector<ReplacementPixel> deserializedReplacements;
                {
                    const auto progress = progressController.createTaskUnbound("reading dead pixels", true);

                    {
                        BufferedDataReader dataReader(readDataFunction, deadPixelsAddressRange.getFirstAddress(), deadPixelsAddressRange.getLastAddress());

                        const auto deserializedDeadPixelsResult = DeadPixel::deserializeDeadPixels([&](size_t requiredDataSize) { return dataReader.getData(requiredDataSize); }, sizeInPixels, progress);
                        if (!deserializedDeadPixelsResult.isOk())
                        {
                            return ReturnType::createFromError(deserializedDeadPixelsResult);
                        }

                        deserializedDeadPixels = deserializedDeadPixelsResult.getValue();
                    }

                    {
                        BufferedDataReader dataReader(readDataFunction, replacementsAddressRange.getFirstAddress(), replacementsAddressRange.getLastAddress());

                        const auto deserializedReplacementsResult = ReplacementPixel::deserializeReplacements([&](size_t requiredDataSize) { return dataReader.getData(requiredDataSize); }, sizeInPixels, progress);
                        if (!deserializedReplacementsResult.isOk())
                        {
                            return ReturnType::createFromError(deserializedReplacementsResult);
                        }

                        deserializedReplacements = deserializedReplacementsResult.getValue();
                    }
                }

                return DeadPixels::createDeadPixels(deserializedDeadPixels, deserializedReplacements);
            };

            auto deadPixelsWriter = [=](connection::IDeviceInterface* device, const DeadPixels& deadPixels, ProgressController progressController) -> VoidResult
            {
                auto createError = [](const std::string& detailErrorMessage)
                {
                    return VoidResult::createError("Unable to write dead pixels", detailErrorMessage);
                };

                if (deadPixels.getSize() > MemorySpaceWtc640::MAX_DEADPIXELS_COUNT)
                {
                    return createError(utils::format("Too many dead pixels! pixels size: {} max size: {}", deadPixels.getSize(), MemorySpaceWtc640::MAX_DEADPIXELS_COUNT));
                }

                const auto deadPixelsData = deadPixels.serializeDeadPixels();
                if (deadPixelsData.size() > deadPixelsAddressRange.getSize())
                {
                    return createError(utils::format("Too many dead pixels! data size: {} max size: {}", deadPixelsData.size(), deadPixelsAddressRange.getSize()));
                }

                const auto replacementsData = deadPixels.serializeReplacements();
                if (replacementsData.size() > replacementsAddressRange.getSize())
                {
                    return createError(utils::format("Too many dead pixels replacements! data size: {} max size: {}", replacementsData.size(), replacementsAddressRange.getSize()));
                }

                {
                    auto progress = progressController.createTaskBound("writing dead pixels", deadPixelsData.size() + replacementsData.size(), false);

                    if (const auto result = device->writeData(deadPixelsData, deadPixelsAddressRange.getFirstAddress(), progress); !result.isOk())
                    {
                        return createError(result.getDetailErrorMessage());
                    }

                    if (const auto result = device->writeData(replacementsData, replacementsAddressRange.getFirstAddress(), progress); !result.isOk())
                    {
                        return createError(result.getDetailErrorMessage());
                    }
                }

                return VoidResult::createOk();
            };

            const auto deadPixelsAdapter = std::make_shared<PropertyAdapterValueDeviceProgress<DeadPixels>>(
                        property, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER),
                        taskCreator, connection::AddressRanges{deadPixelsAddressRange, replacementsAddressRange},
                        deadPixelsReader,
                        deadPixelsWriter);

            return deadPixelsAdapter;
        };

        auto createDeadPixelsWithReplacementsValueAdapter = [](PropertyId propertyId)
        {
            auto valueAdapter = std::make_shared<PropertyValue<DeadPixels>>(propertyId);
            valueAdapter->setCustomConvertToStringFunction([](const DeadPixels& value)
            {
                return utils::format("Dead pixels size: {}", value.getDeadPixelToReplacementsMap().size());
            });

            return valueAdapter;
        };

        addValueAdapter(createDeadPixelsWithReplacementsValueAdapter(PropertyIdWtc640::DEAD_PIXELS_CURRENT),
                        createDeadPixelsWithReplacementsAdapter(PropertyIdWtc640::DEAD_PIXELS_CURRENT, AdapterTaskCreator(m_weakThis),
                                                                MemorySpaceWtc640::DEAD_PIXELS_CURRENT, MemorySpaceWtc640::DEAD_PIXELS_REPLACEMENTS_CURRENT));

        addValueAdapter(createDeadPixelsWithReplacementsValueAdapter(PropertyIdWtc640::DEAD_PIXELS_IN_FLASH),
                        createDeadPixelsWithReplacementsAdapter(PropertyIdWtc640::DEAD_PIXELS_IN_FLASH, AdapterTaskCreator(m_weakThis),
                                                                MemorySpaceWtc640::DEAD_PIXELS_IN_FLASH, MemorySpaceWtc640::DEAD_PIXELS_REPLACEMENTS_IN_FLASH));
    }

    {
        static constexpr uint32_t MASK = 0b1;
        

        addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_CURRENT, MemorySpaceWtc640::ENABLE_DP_REPLACEMENT_CURRENT, MASK,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addBoolDeviceValueSimpleAdapter(PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_IN_FLASH, MemorySpaceWtc640::ENABLE_DP_REPLACEMENT_IN_FLASH, MASK,
                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }
}

void PropertiesWtc640::addFocusAdapters()
{
    addEnumDeviceValueSimpleAdapter<MotorFocusMode>(PropertyIdWtc640::MOTOR_FOCUS_MODE, MemorySpaceWtc640::MOTOR_FOCUS_MODE,
                                                        DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    static constexpr uint32_t POSITION_MASK = 0b1111'1111'1111;
    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::CURRENT_MF_POSITION, MemorySpaceWtc640::CURRENT_MF_POSITION, POSITION_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                                  MOTOR_FOCUS_MIN_VALUE, MOTOR_FOCUS_MAX_VALUE);
    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::TARGET_MF_POSITION, MemorySpaceWtc640::TARGET_MF_POSITION, POSITION_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  MOTOR_FOCUS_MIN_VALUE, MOTOR_FOCUS_MAX_VALUE);
    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::MAXIMAL_MF_POSITION, MemorySpaceWtc640::MAXIMAL_MF_POSITION, POSITION_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                                  MOTOR_FOCUS_MIN_VALUE, MOTOR_FOCUS_MAX_VALUE);

    auto lensSerialNumberValidation = [=](const std::string& serialNumber) -> VoidResult
    {
        if (serialNumber.length() > static_cast<int>(MemorySpaceWtc640::LENS_SERIAL_NUMBER.getSize()))
        {
            return VoidResult::createError("Serial number too long", utils::format("{}\nMax length: {}", serialNumber, MemorySpaceWtc640::LENS_SERIAL_NUMBER.getSize()));
        }

        boost::basic_regex regex(getSerialNumberRegexPattern());
        if (!boost::regex_search(serialNumber, regex))
        {
            return VoidResult::createError("Invalid format", utils::format("{}\nExpected pattern: {}", serialNumber, regex.expression()));
        }

        if (const auto dateResult = getDateFromSerialNumber(serialNumber); !dateResult.isOk())
        {
            return dateResult.toVoidResult();
        }

        return VoidResult::createOk();
    };

    addStringDeviceValueSimpleAdapter(PropertyIdWtc640::LENS_SERIAL_NUMBER, MemorySpaceWtc640::LENS_SERIAL_NUMBER,
                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                      lensSerialNumberValidation);

    auto lensArticleNumberValidation = [=](const std::string& serialNumber) -> VoidResult
    {
        if (serialNumber.length() > static_cast<int>(MemorySpaceWtc640::LENS_ARTICLE_NUMBER.getSize()))
        {
            return VoidResult::createError("Serial number too long", utils::format("{}\nMax length: {}", serialNumber, MemorySpaceWtc640::LENS_ARTICLE_NUMBER.getSize()));
        }

        boost::basic_regex regex("^L-WTC-(35|25|14|7)-WB-(11|12)$");
        if (!boost::regex_search(serialNumber, regex))
        {
            return VoidResult::createError("Invalid format", utils::format("{}\nExpected pattern: {}", serialNumber, regex.expression()));
        }

        return VoidResult::createOk();
    };

    addStringDeviceValueSimpleAdapter(PropertyIdWtc640::LENS_ARTICLE_NUMBER, MemorySpaceWtc640::LENS_ARTICLE_NUMBER,
                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                      lensArticleNumberValidation);
}

void PropertiesWtc640::addPresetsAdapters()
{
    static constexpr uint32_t INDEX_MASK = 0xFF;

    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SELECTED_PRESET_INDEX_CURRENT, MemorySpaceWtc640::SELECTED_PRESET_INDEX_CURRENT, INDEX_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0, INDEX_MASK);

    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::SELECTED_PRESET_INDEX_IN_FLASH, MemorySpaceWtc640::SELECTED_PRESET_INDEX_IN_FLASH, INDEX_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                  0, INDEX_MASK);

    addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyIdWtc640::CURRENT_PRESET_INDEX, MemorySpaceWtc640::CURRENT_PRESET_INDEX, INDEX_MASK,
                                                  DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE,
                                                  0, INDEX_MASK);

    auto addSelectedLensRangeAdapter = [this](PropertyId lensRangePropertyId, PropertyId presetIndexPropertyId)
    {
        const auto selectedPresetIndexAdapter = std::dynamic_pointer_cast<PropertyAdapterValue<unsigned>>(getPropertyAdapters().at(presetIndexPropertyId));
        assert(selectedPresetIndexAdapter != nullptr);

        addValueAdapter(createPresetIdValueAdapter(lensRangePropertyId),
                        std::make_shared<PropertyAdapterSelectedLensRange>(lensRangePropertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER),
                                                                           AdapterTaskCreator(m_weakThis),
                                                                           getPropertyValues(),
                                                                           selectedPresetIndexAdapter));
    };
    addSelectedLensRangeAdapter(PropertyIdWtc640::SELECTED_LENS_RANGE_CURRENT, PropertyIdWtc640::SELECTED_PRESET_INDEX_CURRENT);
    addSelectedLensRangeAdapter(PropertyIdWtc640::SELECTED_LENS_RANGE_IN_FLASH, PropertyIdWtc640::SELECTED_PRESET_INDEX_IN_FLASH);

    {
        const auto propertyId = PropertyIdWtc640::ACTIVE_LENS_RANGE;

        const auto currentPresetIndexAdapter = std::dynamic_pointer_cast<PropertyAdapterValue<unsigned>>(getPropertyAdapters().at(PropertyIdWtc640::CURRENT_PRESET_INDEX));
        assert(currentPresetIndexAdapter != nullptr);

        addValueAdapter(createPresetIdValueAdapter(propertyId),
                        std::make_shared<PropertyAdapterCurrentLensRange>(propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                                                          AdapterTaskCreator(m_weakThis),
                                                                          getPropertyValues(),
                                                                          currentPresetIndexAdapter));
    }

    {
        const auto propertyId = PropertyIdWtc640::ALL_VALID_LENS_RANGES;

        auto valueAdapter = std::make_shared<PropertyValue<std::vector<PresetId>>>(propertyId);

        valueAdapter->setCustomConvertToStringFunction([](const std::vector<PresetId>& value)
        {
            return utils::format("Presets count: {}", value.size());
        });

        addValueAdapter(valueAdapter,
                        std::make_shared<PropertyAdapterAllValidLensRanges>(propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                                                            AdapterTaskCreator(m_weakThis),
                                                                            getPropertyValues()));
    }
}

void PropertiesWtc640::addMotorFocusConstraints()
{
    const auto focusTypePropertyId = PropertyIdWtc640::FOCUS_TYPE_CURRENT;
    {
        auto constraintFuncion = [focusTypePropertyId](const PropertyValues::Transaction& transaction)
        {
            const auto focusTypeResult = transaction.getValue<Focus::Item>(focusTypePropertyId);
            if (focusTypeResult.containsValue() && Focus::isMotoric(focusTypeResult.getValue()))
            {
                return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
            }

            return PropertyAdapterBase::Status::DISABLED;
        };
        addPropertyConstraints(focusTypePropertyId, std::move(constraintFuncion), {PropertyIdWtc640::MOTOR_FOCUS_MODE,
                                                                                   PropertyIdWtc640::CURRENT_MF_POSITION,
                                                                                   PropertyIdWtc640::TARGET_MF_POSITION,
                                                                                   PropertyIdWtc640::MAXIMAL_MF_POSITION});
    }

    {
        auto constraintFuncion = [focusTypePropertyId](const PropertyValues::Transaction& transaction)
        {
            const auto focusTypeResult = transaction.getValue<Focus::Item>(focusTypePropertyId);
            if (focusTypeResult.containsValue() && Focus::isWithBayonet(focusTypeResult.getValue()))
            {
                return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
            }

            return PropertyAdapterBase::Status::DISABLED;
        };
        addPropertyConstraints(focusTypePropertyId, std::move(constraintFuncion), {PropertyIdWtc640::LENS_SERIAL_NUMBER,
                                                                                   PropertyIdWtc640::LENS_ARTICLE_NUMBER});
    }
}

void PropertiesWtc640::addImageFreezeConstraints()
{
    const auto imageFreezePropertyId = PropertyIdWtc640::IMAGE_FREEZE;

    auto constraintFuncion = [imageFreezePropertyId](const PropertyValues::Transaction& transaction)
    {
        const auto imageFreezeResult = transaction.getValue<bool>(imageFreezePropertyId);
        if (!imageFreezeResult.containsValue() || imageFreezeResult.getValue() == true)
        {
            return PropertyAdapterBase::Status::ENABLED_READ_ONLY;
        }

        return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
    };
    addPropertyConstraints(imageFreezePropertyId, std::move(constraintFuncion), {PropertyIdWtc640::TEST_PATTERN});
}

void PropertiesWtc640::addConnectionConstraints()
{
    const auto pluginTypeId = PropertyIdWtc640::PLUGIN_TYPE;

    auto constraintFuncion = [pluginTypeId](const PropertyValues::Transaction& transaction)
    {
        const auto pluginTypeResult = transaction.getValue<Plugin::Item>(pluginTypeId);
        if (!pluginTypeResult.containsValue() || pluginTypeResult.getValue() == Plugin::Item::PLEORA || pluginTypeResult.getValue() == Plugin::Item::ONVIF)
        {
            return PropertyAdapterBase::Status::DISABLED;
        }
        return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
    };
    addPropertyConstraints(pluginTypeId, std::move(constraintFuncion), {PropertyIdWtc640::UART_BAUDRATE_CURRENT,
                                                                        PropertyIdWtc640::UART_BAUDRATE_IN_FLASH});
}

void PropertiesWtc640::addPluginConstraints()
{
    const auto pluginTypeId = PropertyIdWtc640::PLUGIN_TYPE;

    auto constraintFuncionCurrentVideoFormat = [pluginTypeId, this](const PropertyValues::Transaction& transaction)
    {
        const auto pluginTypeResult = transaction.getValue<Plugin::Item>(pluginTypeId);
        if (!pluginTypeResult.containsValue())
        {
            return PropertyAdapterBase::Status::ENABLED_READ_ONLY;
        }
        else if (pluginTypeResult.getValue() == Plugin::Item::USB)
        {
#ifndef __APPLE__
            auto stream = getStreamImpl();
            if (!stream.isOk() || !stream.getValue()->isRunning())
#endif
            {
                return PropertyAdapterBase::Status::ENABLED_READ_ONLY;
            }
        }
        return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
    };
    addPropertyConstraints(pluginTypeId, std::move(constraintFuncionCurrentVideoFormat), {PropertyIdWtc640::VIDEO_FORMAT_CURRENT});

    auto constraintFuncionVideoFormatInFlash = [pluginTypeId, this](const PropertyValues::Transaction& transaction)
    {
        const auto pluginTypeResult = transaction.getValue<Plugin::Item>(pluginTypeId);
        if (!pluginTypeResult.containsValue() || pluginTypeResult.getValue() == Plugin::Item::USB)
        {
            return PropertyAdapterBase::Status::ENABLED_READ_ONLY;
        }
        return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
    };

    addPropertyConstraints(pluginTypeId, std::move(constraintFuncionVideoFormatInFlash), {PropertyIdWtc640::VIDEO_FORMAT_IN_FLASH});

}

void PropertiesWtc640::addPropertyConstraints(PropertyId sourcePropertyId,
                                              std::function<PropertyAdapterBase::Status (const PropertyValues::Transaction& transaction)> constraintFuncion,
                                              const std::initializer_list<PropertyId>& targetPropertyIds)
{
    const auto sourcePropertyAdapter = getPropertyAdapters().at(sourcePropertyId);

    for(const auto& propertyId : targetPropertyIds)
    {
        const auto& adapter = getPropertyAdapters().at(propertyId);
        adapter->setStatusConstraintByValuesFunction(constraintFuncion, {sourcePropertyAdapter}, getPropertyValues().get());
    }
}

void PropertiesWtc640::onCurrentDeviceTypeChanged()
{
    BaseClass::onCurrentDeviceTypeChanged();

    m_sizeInPixels = core::Size();
    if (const auto deviceType = getDeviceType(); deviceType.has_value())
    {
        m_sizeInPixels = DevicesWtc640::getSizeInPixels(deviceType.value());
    }
}

void PropertiesWtc640::onTransactionFinished(const TransactionSummary& transactionSummary)
{
    BaseClass::onTransactionFinished(transactionSummary);

    if (!m_connectionLostSent)
    {
        const auto* deviceInterface = boost::polymorphic_downcast<const connection::DeviceInterfaceWtc640*>(getDeviceInterface());
        const auto* protocolInterface = boost::polymorphic_downcast<const connection::ProtocolInterfaceTCSI*>(deviceInterface->getProtocolInterface().get());

        if (protocolInterface->isConnectionLost() || (m_dataLinkInterface != nullptr && m_dataLinkInterface->isConnectionLost()))
        {
            m_connectionLostSent = true;

            connectionLost();
        }
    }
}

std::optional<Baudrate::Item> PropertiesWtc640::getCurrentBaudrateImpl() const
{
    if (const auto* datalinkUart = dynamic_cast<const connection::DataLinkUart*>(m_dataLinkInterface.get()))
    {
        if (const auto result = datalinkUart->getBaudrate(); result.isOk())
        {
            return result.getValue();
        }
    }

    return std::nullopt;
}

const connection::DeviceInterfaceWtc640* PropertiesWtc640::getDeviceInterfaceWtc640() const
{
    return boost::polymorphic_downcast<const connection::DeviceInterfaceWtc640*>(getDeviceInterface());
}

connection::DeviceInterfaceWtc640* PropertiesWtc640::getDeviceInterfaceWtc640()
{
    return boost::polymorphic_downcast<connection::DeviceInterfaceWtc640*>(getDeviceInterface());
}

ValueResult<DeviceType> PropertiesWtc640::testDeviceType(const ConnectionExclusiveTransaction& transaction)
{
    using ResultType = ValueResult<DeviceType>;

    std::optional<bool> isLoader;
    {
        static const std::vector<uint8_t> LOADER_ID {0x57, 0x06, 0x4C};
        static const std::vector<uint8_t> MAIN_ID   {0x57, 0x06, 0x4D};
        static const uint8_t CURRENT_VERSION = 0x06;

        const auto deviceIdResult = transaction.readData<uint8_t>(MemorySpaceWtc640::DEVICE_IDENTIFICATOR.getFirstAddress(), MemorySpaceWtc640::DEVICE_IDENTIFICATOR.getSize());
        if (!deviceIdResult.isOk())
        {
            return ResultType::createFromError(deviceIdResult);
        }
        assert(deviceIdResult.getValue().size() == 4);

        auto getDeviceIdAsString = [&]()
        {
            std::vector<std::string> list;
            for (const auto value : deviceIdResult.getValue())
            {
                list.push_back(utils::numberToHex(value, true));
            }

            return utils::joinStringVector(list, " ");
        };

        const std::ranges::take_view deviceId{deviceIdResult.getValue(), 3};
        if (std::ranges::equal(deviceId, LOADER_ID))
        {
            isLoader = true;
        }
        else if (std::ranges::equal(deviceId, MAIN_ID))
        {
            isLoader = false;
        }
        else
        {
            return ResultType::createError("Unknown device type!", utils::format("id: [{}]", getDeviceIdAsString()));
        }

        if (deviceIdResult.getValue().back() != CURRENT_VERSION)
        {
            return ResultType::createError("Unsupported firmware version!", utils::format("id: [{}]", getDeviceIdAsString()));
        }
    }

    const auto statusResult = transaction.readData<uint32_t>(MemorySpaceWtc640::STATUS.getFirstAddress(), 1);
    if (!statusResult.isOk())
    {
        return ResultType::createFromError(statusResult);
    }
    assert(statusResult.getValue().size() == 1);

    const StatusWtc640 status(statusResult.getValue().front());
    if (status.isCameraNotReady())
    {
        return ResultType::createError("Device not ready!", "");
    }

    const auto deviceType = status.getDeviceType();
    if (!deviceType.has_value())
    {
        return ResultType::createError("Invalid device type!", "");
    }

    return deviceType.value();
}

void PropertiesWtc640::addCoreSerialNumberAdapters()
{
    auto addCoreSerialNumberAdapter = [this](PropertyId propertyId, const AddressRange& addressRange,
            DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags)
    {
        auto serialNumberValidation = [=](const std::string& serialNumber) -> VoidResult
        {
            if (serialNumber.length() > static_cast<int>(addressRange.getSize()))
            {
                return VoidResult::createError("Serial number too long", utils::format("{}\nMax length: {}", serialNumber, addressRange.getSize()));
            }

            boost::basic_regex regex(getSerialNumberRegexPattern());
            if (!boost::regex_search(serialNumber, regex))
            {
                return VoidResult::createError("Invalid format", utils::format("{}\nExpected pattern: {}", serialNumber, regex.expression()));
            }

            if (const auto dateResult = getDateFromSerialNumber(serialNumber); !dateResult.isOk())
            {
                return dateResult.toVoidResult();
            }

            return VoidResult::createOk();
        };

        const auto serialNumberTransformFunction = [](const std::string& serialNumber, const PropertyValues::Transaction& )
        {
            return utils::stringToUpperTrimmed(serialNumber);
        };

        addStringDeviceValueSimpleAdapter(propertyId, addressRange,
                                          readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags,
                                          serialNumberValidation,
                                          serialNumberTransformFunction);
    };

    addCoreSerialNumberAdapter(PropertyIdWtc640::SERIAL_NUMBER_IN_FLASH, MemorySpaceWtc640::SERIAL_NUMBER_IN_FLASH,
                               DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE);

    addCoreSerialNumberAdapter(PropertyIdWtc640::SERIAL_NUMBER_CURRENT, MemorySpaceWtc640::SERIAL_NUMBER_CURRENT,
                               DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE);
    {
        const auto propertyId = PropertyIdWtc640::PRODUCTION_DATE;

        const auto serialNumberCurrentAdapter = std::dynamic_pointer_cast<PropertyAdapterValue<std::string>>(getPropertyAdapters().at(PropertyIdWtc640::SERIAL_NUMBER_CURRENT));

        const auto productionDateAdapter = std::make_shared<PropertyAdapterValueDerivedFrom1<std::string, std::string>>(
                    propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                    getPropertyValues(), serialNumberCurrentAdapter,
                    [=](const OptionalResult<std::string>& serialNumber, const PropertyValues::Transaction& ) -> OptionalResult<std::string>
                    {
                        using ResultType = OptionalResult<std::string>;

                        if (serialNumber.containsError())
                        {
                            return ResultType::createError("Serial number error!", "");
                        }

                        if (serialNumber.containsValue())
                        {
                            static constexpr int MIN_DATE_LENGTH = 5;
                            if (serialNumber.getValue().length() < MIN_DATE_LENGTH)
                            {
                                return ResultType::createError("Invalid serial number!", utils::format("date minimum length: {}", MIN_DATE_LENGTH));
                            }

                            const auto dateResult = getDateFromSerialNumber(serialNumber.getValue());
                            if (!dateResult.isOk())
                            {
                                return ResultType::createFromError(dateResult);
                            }

                            return utils::format("{} {}", dateResult.getValue().date().month().as_long_string(), dateResult.getValue().date().year());
                        }

                        return std::nullopt;
                    });
        addValueAdapter(std::make_shared<PropertyValue<std::string>>(propertyId), productionDateAdapter);
    }
}

void PropertiesWtc640::addArticleNumberAdapters()
{
    assert(Sensor::ALL_ITEMS.size() == ARTICLE_NUMBER_SENSORS.size());
    assert(Core::ALL_ITEMS.size() == ARTICLE_NUMBER_CORE_TYPES.size());
    assert(DetectorSensitivity::ALL_ITEMS.size() == ARTICLE_NUMBER_DETECTOR_SENSITIVITIES.size());
    assert(Focus::ALL_ITEMS.size() == ARTICLE_NUMBER_FOCUSES.size());
    assert(Framerate::ALL_ITEMS.size() == ARTICLE_NUMBER_FRAMERATES.size());

    auto addCoreArticlelNumberAdapter = [&](PropertyId articleNumberProperty, AddressRange addressRange, const PropertyAdapterBase::GetStatusForDeviceFunction& statusFunction,
            PropertyId sensorTypeProperty, PropertyId coreTypeProperty, PropertyId detectorSensitivityProperty, PropertyId focusTypeProperty, PropertyId maxFramerateProperty)
    {
        const auto compositeReader = [this, addressRange](connection::IDeviceInterface* device) -> ValueResult<ArticleNumber>
        {
            using ResultType = ValueResult<ArticleNumber>;

            const auto readResult = device->readAddressRange(addressRange, ProgressTask());
            if (!readResult.isOk())
            {
                return ResultType::createFromError(readResult);
            }

            const auto result = ArticleNumber::createFromString(dataToString(readResult.getValue()));
            if (result.isOk())
            {
                if (!result.getValue().sensor.isOk())
                {
                    return ResultType::createFromError(result.getValue().sensor);
                }
                else if (result.getValue().sensor.getValue() != Sensor::Item::PICO_640)
                {
                    return ResultType::createError("Invalid article number!", utils::format("sensor type: {} expected: {}", static_cast<int>(result.getValue().sensor.getValue()), static_cast<int>(Sensor::Item::PICO_640)));
                }
            }

            return result;
        };

        const auto compositeWriter = [addressRange](connection::IDeviceInterface* device, const ArticleNumber& articleNumber) -> VoidResult
        {
            const auto articleNumberString = articleNumber.toString();
            if (!articleNumberString.isOk())
            {
                return articleNumberString.toVoidResult();
            }

            auto dataContainer = stringToData(articleNumberString.getValue());
            if (dataContainer.size() > addressRange.getSize())
            {
                return VoidResult::createError("Article number too long", utils::format("max length: {} value: {}", addressRange.getSize(), articleNumberString.getValue()));
            }
            dataContainer.resize(addressRange.getSize(), 0);
            return device->writeTypedData<uint8_t>(dataContainer, addressRange.getFirstAddress(), ProgressTask());
        };

        using CompositeAdapterType = PropertyAdapterValueDeviceComposite<ArticleNumber, PropertyAdapterValueDeviceSimple>;
        const auto compositeAdapter = std::make_shared<CompositeAdapterType>(
                                          articleNumberProperty, statusFunction,
                                          AdapterTaskCreator(m_weakThis), addressRange,
                                          compositeReader,
                                          compositeWriter);

        const auto compositeValue = std::make_shared<PropertyValue<ArticleNumber>>(articleNumberProperty);
        compositeValue->setCustomConvertToStringFunction([](const ArticleNumber& value)
        {
            const auto toStringResult = value.toString();
            return toStringResult.isOk() ? toStringResult.getValue() : std::string();
        });

        addValueAdapter(compositeValue, compositeAdapter);

        addValueAdapter(createPropertyValueEnum<Sensor>(sensorTypeProperty),
                        std::make_shared<PropertyAdapterValueComponent<Sensor::Item, CompositeAdapterType>>(
                            sensorTypeProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &ArticleNumber::sensor,
                            Sensor::Item::PICO_640));

        addValueAdapter(createPropertyValueEnum<Core>(coreTypeProperty),
                        std::make_shared<PropertyAdapterValueComponent<Core::Item, CompositeAdapterType>>(
                            coreTypeProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &ArticleNumber::coreType,
                            Core::Item::NON_RADIOMETRIC));

        addValueAdapter(createPropertyValueEnum<DetectorSensitivity>(detectorSensitivityProperty),
                        std::make_shared<PropertyAdapterValueComponent<DetectorSensitivity::Item, CompositeAdapterType>>(
                            detectorSensitivityProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &ArticleNumber::detectorSensitivity,
                            DetectorSensitivity::Item::PERFORMANCE_NETD_50MK));

        addValueAdapter(createPropertyValueEnum<Focus>(focusTypeProperty),
                        std::make_shared<PropertyAdapterValueComponent<Focus::Item, CompositeAdapterType>>(
                            focusTypeProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &ArticleNumber::focus,
                            Focus::Item::MANUAL_H25));

        addValueAdapter(createPropertyValueEnum<Framerate>(maxFramerateProperty),
                        std::make_shared<PropertyAdapterValueComponent<Framerate::Item, CompositeAdapterType>>(
                            maxFramerateProperty, statusFunction,
                            getPropertyValues(),
                            compositeAdapter,
                            &ArticleNumber::maxFramerate,
                            Framerate::Item::FPS_8_57));
    };

    addCoreArticlelNumberAdapter(PropertyIdWtc640::ARTICLE_NUMBER_CURRENT, MemorySpaceWtc640::ARTICLE_NUMBER_CURRENT, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                 PropertyIdWtc640::SENSOR_TYPE_CURRENT, PropertyIdWtc640::CORE_TYPE_CURRENT, PropertyIdWtc640::DETECTOR_SENSITIVITY_CURRENT, PropertyIdWtc640::FOCUS_TYPE_CURRENT, PropertyIdWtc640::MAX_FRAMERATE_CURRENT);
    addCoreArticlelNumberAdapter(PropertyIdWtc640::ARTICLE_NUMBER_IN_FLASH, MemorySpaceWtc640::ARTICLE_NUMBER_IN_FLASH, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                 PropertyIdWtc640::SENSOR_TYPE_IN_FLASH, PropertyIdWtc640::CORE_TYPE_IN_FLASH, PropertyIdWtc640::DETECTOR_SENSITIVITY_IN_FLASH, PropertyIdWtc640::FOCUS_TYPE_IN_FLASH, PropertyIdWtc640::MAX_FRAMERATE_IN_FLASH);
}

void PropertiesWtc640::addPalettesAdapters()
{
    auto addIndexAdapter = [this](PropertyId property, const AddressRange& addressRange)
    {
        static constexpr unsigned MAX_PALETTES_COUNT = MemorySpaceWtc640::PALETTES_FACTORY_MAX_COUNT + MemorySpaceWtc640::PALETTES_USER_MAX_COUNT;
        addUnsignedDeviceValueSimpleAdapterArithmetic(property, addressRange, 0b1111,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER,
                                                      0, MAX_PALETTES_COUNT - 1);
    };

    addIndexAdapter(PropertyIdWtc640::PALETTE_INDEX_CURRENT, MemorySpaceWtc640::PALETTE_INDEX_CURRENT);
    addIndexAdapter(PropertyIdWtc640::PALETTE_INDEX_IN_FLASH, MemorySpaceWtc640::PALETTE_INDEX_IN_FLASH);

    auto addPaletteAdapter = [&](PropertyId palettePropertyId, unsigned paletteIndex,
                                 const AddressRange& addressRangeName, const AddressRange& addressRangeData,
                                 const std::string& name,
                                 DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags)
    {
        using AdapterType = PropertyAdapterValueDeviceProgress<core::Palette>;
        const auto adapter = std::make_shared<AdapterType>(palettePropertyId, createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                                                           AdapterTaskCreator(m_weakThis), connection::AddressRanges(std::vector<connection::AddressRange>{addressRangeName, addressRangeData}),
                                                           createPaletteReader(addressRangeName, addressRangeData, name),
                                                           createPaletteWriter(addressRangeName, addressRangeData, name));

        auto valueAdapter = std::make_shared<PropertyValue<core::Palette>>(palettePropertyId,
                                                                             [](const core::Palette& palette)
        {
            return palette.getName().empty() ? VoidResult::createError("Invalid name!", "name is empty") :
                                                 VoidResult::createOk();
        });
        valueAdapter->setCustomConvertToStringFunction([](const core::Palette& value) -> std::string
        {
            return utils::format("Palette: {}", value.getName());
        });

        addValueAdapter(valueAdapter, adapter);
    };

    assert(PropertyIdWtc640::PALETTES_FACTORY_CURRENT.size() == MemorySpaceWtc640::PALETTES_FACTORY_MAX_COUNT && PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.size() == MemorySpaceWtc640::PALETTES_FACTORY_MAX_COUNT);
    assert(PropertyIdWtc640::PALETTES_USER_CURRENT.size() == MemorySpaceWtc640::PALETTES_USER_MAX_COUNT && PropertyIdWtc640::PALETTES_USER_IN_FLASH.size() == MemorySpaceWtc640::PALETTES_USER_MAX_COUNT);
    for (unsigned paletteIndex = 0; paletteIndex < MemorySpaceWtc640::PALETTES_FACTORY_MAX_COUNT + MemorySpaceWtc640::PALETTES_USER_MAX_COUNT; ++paletteIndex)
    {
        const bool isAdminPalette = false;

        addPaletteAdapter(PropertyIdWtc640::getPaletteCurrentId(paletteIndex), paletteIndex,
                          MemorySpaceWtc640::getPaletteNameCurrent(paletteIndex), MemorySpaceWtc640::getPaletteDataCurrent(paletteIndex),
                          utils::format("palette {}", paletteIndex + 1),
                          DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

        addPaletteAdapter(PropertyIdWtc640::getPaletteInFlashId(paletteIndex), paletteIndex,
                          MemorySpaceWtc640::getPaletteNameInFlash(paletteIndex), MemorySpaceWtc640::getPaletteDataInFlash(paletteIndex),
                          utils::format("palette {} in flash", paletteIndex + 1),
                          DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);
    }
}

void PropertiesWtc640::addImageFlipAdapters(PropertyId compositeProperty, PropertyId flipImageHorizontallyProperty, PropertyId flipImageVerticallyProperty, const AddressRange& addressRange)
{
    const auto statusFunction = createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER);

    auto compositeReader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<ImageFlip>
    {
        const auto data = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
        if (!data.isOk())
        {
            return ValueResult<ImageFlip>::createFromError(data);
        }
        assert(data.getValue().size() == 1);

        ImageFlip imageFlip;

        imageFlip.flipImageVertically   = (data.getValue().front() & 0b01);
        imageFlip.flipImageHorizontally = (data.getValue().front() & 0b10);

        return imageFlip;
    };

    auto compositeWriter = [addressRange](connection::IDeviceInterface* device, const ImageFlip& imageFlip) -> VoidResult
    {
        if (!imageFlip.flipImageVertically.isOk())
        {
            return imageFlip.flipImageVertically.toVoidResult();
        }

        if (!imageFlip.flipImageHorizontally.isOk())
        {
            return imageFlip.flipImageHorizontally.toVoidResult();
        }

        std::vector<uint32_t> data(1, 0);
        if (imageFlip.flipImageVertically.getValue())
        {
            data.front() |= 0b01;
        }

        if (imageFlip.flipImageHorizontally.getValue())
        {
            data.front() |= 0b10;
        }

        return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
    };

    using CompositeAdapterType = PropertyAdapterValueDeviceComposite<ImageFlip, PropertyAdapterValueDeviceSimple>;
    const auto compositeAdapter = std::make_shared<CompositeAdapterType>(
                                      compositeProperty, statusFunction,
                                      AdapterTaskCreator(m_weakThis), addressRange,
                                      compositeReader,
                                      compositeWriter);

    auto valueAdapter = std::make_shared<PropertyValue<ImageFlip>>(compositeProperty);
    valueAdapter->setCustomConvertToStringFunction([](const ImageFlip& value)
    {
        return createCompositeValueString(
                    {
                        {"flipImageVertically", value.flipImageVertically.toVoidResult()},
                        {"flipImageHorizontally", value.flipImageHorizontally.toVoidResult()},
                    });
    });

    addValueAdapter(valueAdapter, compositeAdapter);

    addValueAdapter(std::make_shared<PropertyValue<bool>>(flipImageVerticallyProperty),
                    std::make_shared<PropertyAdapterValueComponent<bool, CompositeAdapterType>>(
                        flipImageVerticallyProperty, statusFunction,
                        getPropertyValues(),
                        compositeAdapter,
                        &ImageFlip::flipImageVertically,
                        false));

    addValueAdapter(std::make_shared<PropertyValue<bool>>(flipImageHorizontallyProperty),
                    std::make_shared<PropertyAdapterValueComponent<bool, CompositeAdapterType>>(
                        flipImageHorizontallyProperty, statusFunction,
                        getPropertyValues(),
                        compositeAdapter,
                        &ImageFlip::flipImageHorizontally,
                        false));
}

void PropertiesWtc640::addDynamicPresetAdapters(connection::DeviceInterfaceWtc640* deviceInterface)
{
    auto selectPresetAttribute = [&](uint8_t presetIndex, PresetAttribute attribute) -> ValueResult<uint32_t>
    {
        std::vector<uint8_t> data(4, 0);
        data.at(0) = attribute;
        data.at(2) = presetIndex;

        if (const auto result = deviceInterface->writeTypedData<uint8_t>(data, MemorySpaceWtc640::SELECTED_ATTRIBUTE_AND_PRESET_INDEX.getFirstAddress(), ProgressTask()); !result.isOk())
        {
            return ValueResult<uint32_t>::createFromError(result);
        }

        std::vector<uint32_t> addressData(1, 0);
        if (const auto result = deviceInterface->readTypedData<uint32_t>(addressData, MemorySpaceWtc640::ATTRIBUTE_ADDRESS.getFirstAddress(), ProgressTask()); !result.isOk())
        {
            return ValueResult<uint32_t>::createFromError(result);
        }
        return addressData.front();
    };

    auto addTimestampAdapter = [this](PropertyId propertyId, uint32_t address)
    {
        const auto addressRange = connection::AddressRange::firstAndSize(address, 4);
        if (const auto result = checkPresetAdapterAddressRange(addressRange, propertyId); !result.isOk())
        {
            return result;
        }

        const auto reader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<boost::posix_time::ptime>
        {
            const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
            if (!result.isOk())
            {
                return ValueResult<boost::posix_time::ptime>::createFromError(result);
            }

            assert(result.getValue().size() == 1);
            const boost::posix_time::ptime date(boost::gregorian::date(1970, 1, 1), boost::posix_time::seconds(result.getValue().front()));
            return date;
        };

        const auto writer = [addressRange](connection::IDeviceInterface* device, boost::posix_time::ptime timestamp) -> VoidResult
        {
            std::vector<uint32_t> data(1, 0);
            data.at(0) = static_cast<uint32_t>((timestamp - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_seconds());
            return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };

        addValueAdapter(std::make_shared<PropertyValue<boost::posix_time::ptime>>(propertyId),
                        std::make_shared<PropertyAdapterValueDeviceSimple<boost::posix_time::ptime>>(
                            propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER),
                            AdapterTaskCreator(m_weakThis), addressRange,
                            reader,
                            writer));

        return VoidResult::createOk();
    };

    auto addPresetIdAdapter = [this](PropertyId propertyId, uint32_t address)
    {
        const auto addressRange = connection::AddressRange::firstAndSize(address, 4);
        if (const auto result = checkPresetAdapterAddressRange(addressRange, propertyId); !result.isOk())
        {
            return result;
        }

        const auto reader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<PresetId>
        {
            using ResultType = ValueResult<PresetId>;

            TRY_GET_RESULT(const auto result, device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask()));

            assert(result.size() == 1);
            PresetId presetId;

            TRY_GET_RESULT(presetId.range, Range::getFromDeviceValue(result.at(0)));
            TRY_GET_RESULT(presetId.lens, Lens::getFromDeviceValue(result.at(0)));
            TRY_GET_RESULT(presetId.lensVariant, LensVariant::getFromDeviceValue(result.at(0)));
            TRY_GET_RESULT(presetId.version, PresetVersion::getFromDeviceValue(result.at(0)));
            return presetId;
        };

        const auto writer = [addressRange](connection::IDeviceInterface* device, PresetId presetId) -> VoidResult
        {
            std::vector<uint32_t> data(1, 0);
            data.at(0) = Lens::getDeviceValue(presetId.lens) | LensVariant::getDeviceValue(presetId.lensVariant) | Range::getDeviceValue(presetId.range);

            return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };

        addValueAdapter(createPresetIdValueAdapter(propertyId),
                        std::make_shared<PropertyAdapterValueDeviceSimple<PresetId>>(
                            propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER),
                            AdapterTaskCreator(m_weakThis), addressRange,
                            reader,
                            writer));

        return VoidResult::createOk();
    };

    auto addPresetValueAdapter = [this](PropertyId propertyId, uint32_t address, unsigned minValue, unsigned maxValue)
    {
        const auto addressRange = connection::AddressRange::firstAndSize(address, 4);
        if (const auto result = checkPresetAdapterAddressRange(addressRange, propertyId); !result.isOk())
        {
            return result;
        }

        constexpr uint32_t MASK = 0xFFFF;
        assert((minValue & MASK) == minValue && (maxValue & MASK) == maxValue);
        addUnsignedDeviceValueSimpleAdapterArithmetic(propertyId, addressRange, MASK,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::NONE,
                                                      minValue, maxValue);

        return VoidResult::createOk();
    };

    auto addPresetAdapter = [=, this](uint8_t presetIndex, PresetAttribute attribute, uint8_t presetsCount)
    {
        const auto addressDataResult = selectPresetAttribute(presetIndex, attribute);
        if(!addressDataResult.isOk())
        {
            return addressDataResult.toVoidResult();
        }
        const auto addressData = addressDataResult.getValue();

        const auto it = m_presetAttributeIds.at(presetIndex).find(attribute);
        assert(it != m_presetAttributeIds.at(presetIndex).end());
        if (it != m_presetAttributeIds.at(presetIndex).end())
        {
            const auto propertyId = it->second;
            switch (attribute)
            {
                case PRESETATTRIBUTE_TIMESTAMP:
                    return addTimestampAdapter(propertyId, addressData);

                case PRESETATTRIBUTE_PRESET_ID:
                    return addPresetIdAdapter(propertyId, addressData);

                case PRESETATTRIBUTE_GAIN_MATRIX:
                {
                    static constexpr uint16_t FACTOR = 1 << 14;

                    return addNucMatrixAdapter<uint16_t>(propertyId, addressData,
                                               utils::format("preset {} nuc matrix", presetIndex + 1),
                                               [](uint16_t value) -> ValueResult<float>
                                               {
                                                   float resultValue = value;
                                                   resultValue /= FACTOR;
                                                   if (!std::isfinite(resultValue))
                                                   {
                                                       return ValueResult<float>::createError("Invalid value!", utils::format("value: {}", value));
                                                   }

                                                   return resultValue;
                                               },
                                               [](float value) -> ValueResult<uint16_t>
                                               {
                                                   if (value < NUC_MIN_VALUE || value > NUC_MAX_VALUE)
                                                   {
                                                       return ValueResult<uint16_t>::createError("Invalid value!", utils::format("value: {} min: {} max: {}", value, NUC_MIN_VALUE, NUC_MAX_VALUE));
                                                   }

                                                   const auto resultValue = std::clamp(std::round(value * FACTOR), 0.0f, static_cast<float>(std::numeric_limits<uint16_t>::max()));
                                                   return static_cast<uint16_t>(resultValue);
                                               });
                }
                case PRESETATTRIBUTE_ONUC_MATRIX:
                    return addNucMatrixAdapter<int16_t>(propertyId, addressData,
                                               utils::format("preset {} onuc matrix", presetIndex + 1),
                                               [](int16_t value) -> ValueResult<float>
                                               {
                                                   return static_cast<float>(value);
                                               },
                                               [](float value) -> ValueResult<int16_t>
                                               {
                                                   const auto resultValue = std::round(value);

                                                   if (resultValue < ONUC_MIN_VALUE || resultValue > ONUC_MAX_VALUE)
                                                   {
                                                       return ValueResult<int16_t>::createError("Invalid value!", utils::format("value: {} min: {} max: {}", value, ONUC_MIN_VALUE, ONUC_MAX_VALUE));
                                                   }

                                                   return static_cast<int16_t>(resultValue);
                                               });
                default:
                    assert(false);
            }
        }

        return VoidResult::createOk();
    };

    std::vector<uint8_t> countData(4, 0);
    if (const auto result = deviceInterface->readTypedData<uint8_t>(countData, MemorySpaceWtc640::NUMBER_OF_PRESETS_AND_ATTRIBUTES.getFirstAddress(), ProgressTask()); !result.isOk())
    {
        WW_LOG_PROPERTIES_CRITICAL << "Preset table error!" << result.getGeneralErrorMessage() << result.getDetailErrorMessage();

        return;
    }
    const uint8_t attributesCount = countData.at(0);
    const uint8_t presetsCount = countData.at(2);

    auto addPresetPropertiesAndAdapters = [&](uint8_t presetIndex)
    {
        if (m_presetAttributeIds.size() == presetIndex)
        {
            std::map<PresetAttribute, PropertyId> propertyIds;
            for (PresetAttribute attribute = PRESETATTRIBUTE__BEGIN; attribute < PRESETATTRIBUTE__END && (attribute - PRESETATTRIBUTE__BEGIN) < attributesCount; attribute = static_cast<PresetAttribute>(attribute + 1))
            {
                const auto idString = PropertiesWtc640::getAttributePropertyIdString(presetIndex, attribute);
                propertyIds.emplace(attribute, PropertyId::createPropertyId(idString, ""));
            }

            m_presetAttributeIds.push_back(propertyIds);
        }

        for (PresetAttribute attribute = PRESETATTRIBUTE__BEGIN; attribute < PRESETATTRIBUTE__END && (attribute - PRESETATTRIBUTE__BEGIN) < attributesCount; attribute = static_cast<PresetAttribute>(attribute + 1))
        {
            const auto result = addPresetAdapter(presetIndex, attribute, presetsCount);
            if (!result.isOk())
            {
                WW_LOG_PROPERTIES_CRITICAL << "Preset table error!" << result.getGeneralErrorMessage() << result.getDetailErrorMessage();

                return VoidResult::createError("Preset table error!");
            }
        }

        return VoidResult::createOk();
    };

    auto addPresetAttributesStatusConstraints = [&](uint8_t presetIndex)
    {
        const auto propertyPresetId = m_presetAttributeIds.at(presetIndex).at(PRESETATTRIBUTE_PRESET_ID);
        const auto presetIdAdapter = getPropertyAdapters().at(propertyPresetId);
        auto enablePropertyDefinedPresetIdConstraintFuncion = [propertyPresetId](const PropertyValues::Transaction& transaction)
        {
            const auto presetId = transaction.getValue<PresetId>(propertyPresetId);
            if (!presetId.containsValue() || !presetId.getValue().isDefined())
            {
                return PropertyAdapterBase::Status::DISABLED;
            }

            return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
        };

        auto enableSnucTablePresetVersionConstraintFunction = [propertyPresetId](const PropertyValues::Transaction& transaction)
        {
            const auto presetId = transaction.getValue<PresetId>(propertyPresetId);
            if (presetId.containsValue() && presetId.getValue().isDefined() && presetId.getValue().version == PresetVersion::Item::PRESET_VERSION_WITH_SNUC)
            {
                return PropertyAdapterBase::Status::ENABLED_READ_WRITE;
            }

            return PropertyAdapterBase::Status::DISABLED;
        };



        for (PresetAttribute attribute = PRESETATTRIBUTE__BEGIN; attribute < PRESETATTRIBUTE__END && (attribute - PRESETATTRIBUTE__BEGIN) < attributesCount; attribute = static_cast<PresetAttribute>(attribute + 1))
        {
            if (attribute != PRESETATTRIBUTE_PRESET_ID)
            {
                const auto propertyId = m_presetAttributeIds.at(presetIndex).at(attribute);
                if (getPropertyAdapters().count(propertyId))
                {
                    const auto adapter = getPropertyAdapters().at(propertyId);

                    if (attribute == PRESETATTRIBUTE_SNUC_TABLE)
                    {
                        adapter->setStatusConstraintByValuesFunction(enableSnucTablePresetVersionConstraintFunction, { presetIdAdapter }, getPropertyValues().get());
                    }
                    else
                    {
                        adapter->setStatusConstraintByValuesFunction(enablePropertyDefinedPresetIdConstraintFuncion, { presetIdAdapter }, getPropertyValues().get());
                    }
                }
            }
        }
    };

    for (uint8_t presetIndex = 0; presetIndex < presetsCount; ++presetIndex)
    {
        if (const auto result = addPresetPropertiesAndAdapters(presetIndex); !result.isOk())
        {
            removeDynamicPresetAdapters();
            return;
        }

        addPresetAttributesStatusConstraints(presetIndex);
    }

    m_currentPresetsCount = presetsCount;

    onPresetAdaptersChanged();
}

VoidResult PropertiesWtc640::checkPresetAdapterAddressRange(const connection::AddressRange& addressRange, PropertyId propertyId) const
{
    const auto memoryDescriptor = getDeviceInterfaceWtc640()->getMemorySpace().getMemoryDescriptor(addressRange);
    if (!memoryDescriptor.isOk() || memoryDescriptor.getValue().type != connection::MemoryTypeWtc640::FLASH)
    {
        return VoidResult::createError("Invalid memory range - flash expected!", utils::format("address range: {}", addressRange.toHexString()));
    }

    for (const auto& deviceType : {DevicesWtc640::MAIN_USER})
    {
        const auto propertiesInConflict = getMappedPropertiesInConflict(addressRange, deviceType);
        if (!propertiesInConflict.empty())
        {
            std::vector<std::string> list;
            for (const auto& propertyInConflict : propertiesInConflict)
            {
                list.push_back(propertyInConflict.getIdString());
            }

            return VoidResult::createError("Invalid memory range - conflict!", utils::format("property: {} address range: {} properties in conflict: {}", propertyId.getIdString(), addressRange.toHexString(), utils::joinStringVector(list, ", ")));
        }
    }

    return VoidResult::createOk();
}

template<class T>
VoidResult PropertiesWtc640::addNucMatrixAdapter(PropertyId propertyId, uint32_t address,
                                                 const std::string& matrixName,
                                                 const std::function<ValueResult<float> (T)>& toFloatFunction,
                                                 const std::function<ValueResult<T> (float)>& fromFloatFunction)
{
    const auto addressRange = connection::AddressRange::firstAndSize(address, MemorySpaceWtc640::PRESET_MATRIX_SIZE);
    if (const auto result = checkPresetAdapterAddressRange(addressRange, propertyId); !result.isOk())
    {
        return result;
    }

    const auto reader = [addressRange, matrixName, toFloatFunction](connection::IDeviceInterface* device, ProgressController progressController) -> ValueResult<NucMatrix>
    {
        using ResultType = ValueResult<NucMatrix>;

        const auto progress = progressController.createTaskBound(utils::format("reading {}", matrixName), addressRange.getSize(), true);

        const auto result = device->readTypedDataFromRange<T>(addressRange, progress);
        if (!result.isOk())
        {
            return ResultType::createFromError(result);
        }

        NucMatrix matrix;
        matrix.data.resize(result.getValue().size());

        for (size_t i = 0; i < matrix.data.size(); ++i)
        {
            const auto conversionResult = toFloatFunction(result.getValue().at(i));
            if (!conversionResult.isOk())
            {
                return ResultType::createFromError(conversionResult);
            }

            matrix.data.at(i) = conversionResult.getValue();
        }

        return matrix;
    };

    const auto writer = [addressRange, matrixName, fromFloatFunction](connection::IDeviceInterface* device, const NucMatrix& matrix, ProgressController progressController) -> VoidResult
    {
        std::vector<T> data;
        data.resize(matrix.data.size());

        for (size_t i = 0; i < matrix.data.size(); ++i)
        {
            const auto conversionResult = fromFloatFunction(matrix.data.at(i));
            if (!conversionResult.isOk())
            {
                return conversionResult.toVoidResult();
            }

            data.at(i) = conversionResult.getValue();
        }

        const auto progress = progressController.createTaskBound(utils::format("writing {}", matrixName), data.size() * sizeof(T), false);

        return device->writeTypedData<T>(data, addressRange.getFirstAddress(), progress);
    };

    auto validationFunction = [](const NucMatrix& matrix)
    {
        static constexpr size_t MATRIX_SIZE = DevicesWtc640::WIDTH * DevicesWtc640::HEIGHT;
        if (matrix.data.size() != MATRIX_SIZE)
        {
            return VoidResult::createError("Invalid matrix!", utils::format("size: {} expected: {}", matrix.data.size(), MATRIX_SIZE));
        }

        return VoidResult::createOk();
    };

    const auto valueAdapter = std::make_shared<PropertyValue<NucMatrix>>(propertyId, validationFunction);
    valueAdapter->setCustomConvertToStringFunction([](const NucMatrix& value)
                                                   {
                                                       return utils::format("Matrix size: {}", value.data.size());
                                                   });

    addValueAdapter(valueAdapter,
                    std::make_shared<PropertyAdapterValueDeviceProgress<NucMatrix>>(
                        propertyId, createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::MAIN_640, ModeFlags::USER),
                        AdapterTaskCreator(m_weakThis), addressRange,
                        reader,
                        writer));

    return VoidResult::createOk();
}

void PropertiesWtc640::removeDynamicPresetAdapters()
{
    m_currentPresetsCount = 0;

    onPresetAdaptersChanged();

    for (const auto& presetAttributeIds : m_presetAttributeIds)
    {
        for (const auto& [presetAttribute, propertyId] : presetAttributeIds)
        {
            removeValueAdapter(propertyId);
        }
    }
}

void PropertiesWtc640::addDummyDynamicUsbAdapters()
{
    {
        auto propertyId = PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION;
        auto value = std::make_shared<PropertyValue<Version>>(propertyId);
        auto adapter = std::make_shared<PropertyAdapterValueDeviceSimple<Version>>(
            propertyId,
            [](const std::optional<DeviceType>&) { return PropertyAdapterBase::Status::DISABLED; },
            AdapterTaskCreator(m_weakThis),
            connection::AddressRanges(),
            nullptr,
            nullptr);
        addValueAdapter(value, adapter);
    }
    {
        auto propertyId = PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER;
        auto value = std::make_shared<PropertyValue<std::string>>(propertyId);
        auto adapter = std::make_shared<PropertyAdapterValueDeviceSimple<std::string>>(
            propertyId,
            [](const std::optional<DeviceType>&) { return PropertyAdapterBase::Status::DISABLED; },
            AdapterTaskCreator(m_weakThis),
            connection::AddressRanges(),
            nullptr,
            nullptr);
        addValueAdapter(value, adapter);
    }
}

void PropertiesWtc640::addDynamicUsbAdapters()
{
    auto addFX3StringDescriptor = [this](PropertyId propertyId, uint8_t stringDescriptorNumber)
    {
        auto valueAdapter = std::make_shared<PropertyValue<std::string>>(propertyId);
        addValueAdapter(valueAdapter, std::make_shared<ValueAdapterUsbStringDescriptor>(
                                          propertyId,
                                          createStatusFunction(DeviceFlags::MAIN_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                          AdapterTaskCreator(m_weakThis),
                                          WTC_640_VENDOR_IDS[0],
                                          WTC_640_PRODUCT_IDS[0],
                                          stringDescriptorNumber,
                                          m_dataLinkInterface));
    };

    addFX3StringDescriptor(core::PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION, 5);
    addFX3StringDescriptor(core::PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER, 3);

    const auto pluginTypeId = core::PropertyIdWtc640::PLUGIN_TYPE;
    auto constraintFuncionUsbPlugin = [pluginTypeId, this](const PropertyValues::Transaction& transaction)
    {
        const auto pluginTypeResult = transaction.getValue<Plugin::Item>(pluginTypeId);
        if (!pluginTypeResult.containsValue() || pluginTypeResult.getValue() != Plugin::Item::USB)
        {
            return PropertyAdapterBase::Status::DISABLED;
        }
        return PropertyAdapterBase::Status::ENABLED_READ_ONLY;
    };

    addPropertyConstraints(pluginTypeId, std::move(constraintFuncionUsbPlugin), {core::PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION, core::PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER});
}

void PropertiesWtc640::removeDynamicUsbAdapters()
{
    for(const auto& propertyId : getDynamicPropertyAdapters())
    {
        if (getPropertyAdapters().count(propertyId))
        {
            removeValueAdapter(propertyId);
        }
    }
}

void PropertiesWtc640::onPresetAdaptersChanged()
{
    using PresetIdAdapterType = PropertyAdapterValue<PresetId>;

    std::vector<std::shared_ptr<PresetIdAdapterType>> presetIdAdapters;
    for (uint8_t presetIndex = 0; presetIndex < m_currentPresetsCount; ++presetIndex)
    {
        const auto propertyId = m_presetAttributeIds.at(presetIndex).at(PRESETATTRIBUTE_PRESET_ID);
        presetIdAdapters.push_back(std::dynamic_pointer_cast<PresetIdAdapterType>(getPropertyAdapters().at(propertyId)));
        assert(presetIdAdapters.back() != nullptr);
    }

    for (const auto propertyId : {PropertyIdWtc640::SELECTED_LENS_RANGE_CURRENT, PropertyIdWtc640::SELECTED_LENS_RANGE_IN_FLASH, PropertyIdWtc640::ACTIVE_LENS_RANGE})
    {
        auto* lensRangeAdapter = boost::polymorphic_downcast<PropertyAdapterCurrentLensRange*>(getPropertyAdapters().at(propertyId).get());
        lensRangeAdapter->setPresetIdAdapters(presetIdAdapters);
    }

    auto* allValidLensRangesAdapter = boost::polymorphic_downcast<PropertyAdapterAllValidLensRanges*>(getPropertyAdapters().at(PropertyIdWtc640::ALL_VALID_LENS_RANGES).get());
    allValidLensRangesAdapter->setPresetIdAdapters(presetIdAdapters);
}

void PropertiesWtc640::addVersionAdapter(PropertyId propertyId, const AddressRange& addressRange)
{
    assert(addressRange.getSize() == 4);

    auto addressReader = [addressRange](connection::IDeviceInterface* device) -> ValueResult<Version>
    {
        const auto result = device->readAddressRange(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<Version>::createFromError(result);
        }

        Version version(0, 0, 0);

        version.minor2 = result.getValue().at(1);
        version.minor2 <<= 8;
        version.minor2 += result.getValue().at(0);

        version.minor = result.getValue().at(2);

        version.major = result.getValue().at(3);

        return version;
    };

    addValueAdapter(std::make_shared<PropertyValue<Version>>(propertyId),
                    std::make_shared<PropertyAdapterValueDeviceSimple<Version>>(propertyId, createStatusFunction(DeviceFlags::ALL_640, ModeFlags::USER, DeviceFlags::NONE, ModeFlags::NONE),
                                                                                AdapterTaskCreator(m_weakThis), addressRange,
                                                                                addressReader,
                                                                                nullptr));
}

void PropertiesWtc640::addCintAdapter(PropertyId propertyId, const AddressRange& addressRange, bool isEditable)
{
    auto transformFunction = [](SensorCint::Item value, const PropertyValues::Transaction& )
    {
        if (SensorCint::ALL_ITEMS.find(value) == SensorCint::ALL_ITEMS.end())
        {
            return SensorCint::Item::CINT_6_5_GAIN_1_00;
        }

        return value;
    };

    addEnumDeviceValueSimpleAdapter<SensorCint>(propertyId, addressRange,
                                                      DeviceFlags::MAIN_640, ModeFlags::USER, (isEditable ? DeviceFlags::MAIN_640: DeviceFlags::NONE), (isEditable ? ModeFlags::NONE : ModeFlags::NONE),
                                                      nullptr,
                                                      transformFunction);
}

PropertyAdapterBase::GetStatusForDeviceFunction PropertiesWtc640::createStatusFunction(DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags)
{
    assert((readModeFlags != ModeFlags::NONE) == (readDeviceFlags != DeviceFlags::NONE) && "Reading partially set but unable to enable!");
    assert((writeModeFlags != ModeFlags::NONE) == (writeDeviceFlags != DeviceFlags::NONE) && "Writing partially set but unable to enable!");

    return [=](const std::optional<DeviceType>& deviceType)
    {
        int currentDeviceFlags = 0;
        auto currentModeFlags = static_cast<int>(ModeFlags::USER);
        if (deviceType.has_value())
        {
            if (deviceType.value() == DevicesWtc640::MAIN_USER)
            {
                currentDeviceFlags |= static_cast<int>(DeviceFlags::MAIN_640);
            }
            else
            {
                assert(deviceType.value() == DevicesWtc640::LOADER);

                currentDeviceFlags |= static_cast<int>(DeviceFlags::LOADER_640);
            }
        }

        const bool isReadEnabled = (currentDeviceFlags & static_cast<int>(readDeviceFlags)) && (currentModeFlags & static_cast<int>(readModeFlags));
        const bool isWriteEnabled = (currentDeviceFlags & static_cast<int>(writeDeviceFlags)) && (currentModeFlags & static_cast<int>(writeModeFlags));
        if (isReadEnabled)
        {
            return isWriteEnabled ? PropertyAdapterBase::Status::ENABLED_READ_WRITE : PropertyAdapterBase::Status::ENABLED_READ_ONLY;
        }
        else if (isWriteEnabled)
        {
            return PropertyAdapterBase::Status::ENABLED_WRITE_ONLY;
        }

        return PropertyAdapterBase::Status::DISABLED;
    };
}

void PropertiesWtc640::addUnsignedDeviceValueSimpleAdapterArithmetic(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                                                     DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                                     unsigned minValue, unsigned maxValue)
{
    addValueAdapter(std::make_shared<PropertyValueArithmetic<unsigned>>(property, minValue, maxValue),
                    std::make_shared<PropertyAdapterValueDeviceSimple<unsigned>>(property, createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                                                                                 AdapterTaskCreator(m_weakThis), addressRange,
                                                                                 createUnsignedReader(addressRange, mask),
                                                                                 (writeModeFlags != ModeFlags::NONE) ? createUnsignedWriter(addressRange, mask) : nullptr));
}

void PropertiesWtc640::addSignedDeviceValueSimpleAdapterArithmetic(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                                                   DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                                   signed minValue, signed maxValue)
{
    addValueAdapter(std::make_shared<PropertyValueArithmetic<signed>>(property, minValue, maxValue),
                    std::make_shared<PropertyAdapterValueDeviceSimple<signed>>(property, createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                                                                               AdapterTaskCreator(m_weakThis), addressRange,
                                                                               createSignedReader(addressRange, mask),
                                                                               (writeModeFlags != ModeFlags::NONE) ? createSignedWriter(addressRange, mask) : nullptr));
}


void PropertiesWtc640::addStringDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                                         DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                         const std::function<VoidResult (const std::string&)>& validationFunction,
                                                         const std::function<std::string (const std::string&, const PropertyValues::Transaction&)>& transformFunction)
{
    addValueAdapter(std::make_shared<PropertyValue<std::string>>(property, validationFunction),
                    std::make_shared<PropertyAdapterValueDeviceSimple<std::string>>(property, createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                                                                                 AdapterTaskCreator(m_weakThis), addressRange,
                                                                                 createStringReader(addressRange),
                                                                                 (writeModeFlags != ModeFlags::NONE) ? createStringWriter(addressRange) : nullptr, transformFunction));
}

void PropertiesWtc640::addBoolDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange, uint32_t mask,
                                                       DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags)
{
    assert(mask != 0);

    const auto reader = [addressRange, mask](connection::IDeviceInterface* device) -> ValueResult<bool>
    {
        const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<bool>::createFromError(result);
        }

        assert(result.getValue().size() == 1);
        return (result.getValue().front() & mask) != 0;
    };

    std::function<VoidResult (connection::IDeviceInterface* , bool )> writer;
    if (writeModeFlags != ModeFlags::NONE)
    {
        writer = [addressRange, mask](connection::IDeviceInterface* device, bool value) -> VoidResult
        {
            std::vector<uint32_t> data(1, 0);
            data.at(0) = value ? mask : 0;
            return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };
    }

    addValueAdapter(std::make_shared<PropertyValue<bool>>(property),
                    std::make_shared<PropertyAdapterValueDeviceSimple<bool>>(
                        property,
                        createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                        AdapterTaskCreator(m_weakThis), addressRange,
                        reader,
                        writer));
}

template<class EnumDefinitionClass>
void PropertiesWtc640::addEnumDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                                       DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                       const std::function<VoidResult (typename EnumDefinitionClass::Item)>& validationFunction,
                                                       const std::function<typename EnumDefinitionClass::Item (typename EnumDefinitionClass::Item, const PropertyValues::Transaction&)>& transformFunction)
{
    using EnumType = typename EnumDefinitionClass::Item;
    static_assert(sizeof(EnumDefinitionClass::MASK) <= sizeof(uint32_t));
    static_assert(sizeof(decltype(decltype(EnumDefinitionClass::ALL_ITEMS)::mapped_type::deviceValue)) <= sizeof(EnumDefinitionClass::MASK));

    std::map<EnumType, std::string> valueToUserNameMap;

    std::map<uint32_t, EnumType> toEnum;
    std::map<EnumType, uint32_t> fromEnum;
    for (const auto [value, description] : EnumDefinitionClass::ALL_ITEMS)
    {
        valueToUserNameMap.emplace(value, description.userName);

        assert((description.deviceValue & EnumDefinitionClass::MASK) == description.deviceValue && "Invalid enum mask!");
        toEnum.emplace(description.deviceValue, value);
        VERIFY(fromEnum.emplace(value, description.deviceValue).second);
    }

    using ReadResultType = ValueResult<EnumType>;
    const auto reader = [addressRange, toEnum](connection::IDeviceInterface* device) -> ReadResultType
    {
        const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ReadResultType::createFromError(result);
        }
        assert(result.getValue().size() == 1);

        const auto deviceValue =  result.getValue().front() & EnumDefinitionClass::MASK;
        const auto it = toEnum.find(deviceValue);
        if (it == toEnum.end())
        {
            return ReadResultType::createError("Value out of range!", utils::format("value: {}", deviceValue));
        }

        return it->second;
    };

    std::function<VoidResult (connection::IDeviceInterface* , EnumType )> writer;
    if (writeModeFlags != ModeFlags::NONE)
    {
        writer = [addressRange, fromEnum](connection::IDeviceInterface* device, EnumType value) -> VoidResult
        {
            const auto it = fromEnum.find(value);
            if (it == fromEnum.end())
            {
                return VoidResult::createError("Value out of range!", utils::format("value: {}", static_cast<int>(value)));
            }

            std::vector<uint32_t> data(1, it->second);
            return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };
    }

    addValueAdapterImpl(std::make_shared<PropertyValueEnum<EnumType>>(property, valueToUserNameMap, validationFunction),
                        property,
                        createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                        AdapterTaskCreator(m_weakThis), addressRange,
                        reader,
                        writer,
                        transformFunction);
}

template<class EnumDefinitionClass>
std::shared_ptr<PropertyValueEnum<typename EnumDefinitionClass::Item>> PropertiesWtc640::createPropertyValueEnum(PropertyId propertyId,
                                                                                       const std::function<VoidResult (const typename EnumDefinitionClass::Item&)>& validationFunction)
{
    using EnumType = typename EnumDefinitionClass::Item;

    std::map<EnumType, std::string> valueToUserNameMap;

    for (const auto [value, description] : EnumDefinitionClass::ALL_ITEMS)
    {
        valueToUserNameMap.emplace(value, description.userName);
    }

    return std::make_shared<PropertyValueEnum<EnumType>>(propertyId, valueToUserNameMap,
                                                         validationFunction);
}

void PropertiesWtc640::addFixedPointMCP9804DeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                                                    DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                                    bool signedFormat, std::optional<double> minimum, std::optional<double> maximum)
{
    static constexpr uint16_t FIXED_POINT_BITS = 12;
    static constexpr uint16_t FIXED_POINT_FRACTIONAL_BITS = 4;
    static constexpr uint16_t FIXED_POINT_MASK = (1 << FIXED_POINT_BITS) - 1;
    static constexpr uint16_t FIXED_POINT_SIGN_MASK = 1 << FIXED_POINT_BITS;

    const auto doubleFromFixedPoint = [signedFormat](uint16_t valueFixed) -> double
    {
        int16_t extendedValue = valueFixed & FIXED_POINT_MASK;
        const bool isValueFixedNegative = valueFixed & FIXED_POINT_SIGN_MASK;

        if (extendedValue == 0 && isValueFixedNegative)
        {
            return -0.0;
        }

        if (signedFormat && isValueFixedNegative)
        {
            extendedValue |= ~FIXED_POINT_MASK;
        }
        return (static_cast<double>(extendedValue) / static_cast<double>(1 << FIXED_POINT_FRACTIONAL_BITS));
    };

    const auto fixedPointFromDouble = [signedFormat](double valueDouble) -> uint16_t
    {
        assert(std::isfinite(valueDouble));
        assert(signedFormat || valueDouble >= 0);

        uint16_t valueFixed = std::round(valueDouble * static_cast<double>(1 << FIXED_POINT_FRACTIONAL_BITS));
        valueFixed &= FIXED_POINT_MASK;

        if (std::signbit(valueDouble))
        {
            valueFixed |= FIXED_POINT_SIGN_MASK;
        }
        return valueFixed;
    };

    const auto transformFunction = [doubleFromFixedPoint, fixedPointFromDouble](double value, const PropertyValues::Transaction& transaction)
    {
        if (!std::isfinite(value))
        {
            return value;
        }
        return doubleFromFixedPoint(fixedPointFromDouble(value));
    };

    const auto reader = [addressRange, doubleFromFixedPoint](connection::IDeviceInterface* device) -> ValueResult<double>
    {
        const auto result = device->readTypedDataFromRange<uint16_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<double>::createFromError(result);
        }
        assert(result.getValue().size() >= 1);
        return doubleFromFixedPoint(result.getValue().at(0));
    };

    std::function<VoidResult (connection::IDeviceInterface* , double )> writer;
    if (writeModeFlags != ModeFlags::NONE)
    {
        writer = [addressRange, doubleFromFixedPoint, fixedPointFromDouble](connection::IDeviceInterface* device, double value) -> VoidResult
        {
            std::vector<uint16_t> data(2, 0);
            data.at(0) = fixedPointFromDouble(value);
            assert(doubleFromFixedPoint(data.at(0)) == value);
            return device->writeTypedData<uint16_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };
    }

//  | sign |bits11-0|   value  |
//  |:----:|:------:|:--------:|
//  |   1  |  10..0 |   -128   | min value
//  |   1  |  0...0 |    -0    |
//  |   0  |  0...0 |     0    |
//  |   0  |  1...1 | 255.9375 | max value
    double fixedPointMinimum = signedFormat ? doubleFromFixedPoint((1 << (FIXED_POINT_BITS-1)) | FIXED_POINT_SIGN_MASK) : doubleFromFixedPoint(0);
    double fixedPointMaximum = doubleFromFixedPoint(FIXED_POINT_MASK);

    if (minimum.has_value())
    {
        assert(minimum.value() >= fixedPointMinimum);
        fixedPointMinimum = minimum.value();
    }

    if (maximum.has_value())
    {
        assert(maximum.value() <= fixedPointMaximum);
        fixedPointMaximum = maximum.value();
    }
    assert(fixedPointMinimum < fixedPointMaximum);

    addValueAdapter(std::make_shared<PropertyValueArithmetic<double>>(property, fixedPointMinimum, fixedPointMaximum),
                    std::make_shared<PropertyAdapterValueDeviceSimple<double>>(
                        property,
                        createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                        AdapterTaskCreator(m_weakThis), addressRange,
                        reader,
                        writer,
                        transformFunction));
}

void PropertiesWtc640::addUnsignedFixedPointDeviceValueSimpleAdapter(PropertyId property, const AddressRange& addressRange,
                                                                     DeviceFlags readDeviceFlags, ModeFlags readModeFlags, DeviceFlags writeDeviceFlags, ModeFlags writeModeFlags,
                                                                     double step, unsigned fractionalBits, unsigned totalBits, unsigned valueShift,
                                                                     double minimum, double maximum)
{
    const unsigned fixedPointFractionalBits = fractionalBits;
    const unsigned fixedPointTotalBits = totalBits;

    const uint32_t fixedPointMask = ((1 << fixedPointTotalBits) - 1) << valueShift;

    const auto doubleFromFixedPoint = [fixedPointFractionalBits, valueShift, fixedPointMask](uint32_t valueFixed) -> double
    {
        const uint32_t maskedValue = (valueFixed & fixedPointMask) >> valueShift;
        return (static_cast<double>(maskedValue) / static_cast<double>(1 << fixedPointFractionalBits));
    };

    const auto fixedPointFromDouble = [fixedPointFractionalBits, valueShift, fixedPointMask](double valueDouble) -> uint32_t
    {
        assert(std::isfinite(valueDouble));
        assert(valueDouble >= 0);

        uint32_t valueFixed = std::round(valueDouble * static_cast<double>(1 << fixedPointFractionalBits));
        valueFixed = (valueFixed << valueShift) & fixedPointMask;
        return valueFixed;
    };

    const auto transformFunction = [doubleFromFixedPoint, fixedPointFromDouble](double value, const PropertyValues::Transaction& transaction)
    {
        if (!std::isfinite(value))
        {
            return value;
        }
        return doubleFromFixedPoint(fixedPointFromDouble(value));
    };

    const auto reader = [addressRange, doubleFromFixedPoint](connection::IDeviceInterface* device) -> ValueResult<double>
    {
        const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<double>::createFromError(result);
        }
        assert(result.getValue().size() >= 1);
        return doubleFromFixedPoint(result.getValue().at(0));
    };

    std::function<VoidResult (connection::IDeviceInterface* , double )> writer;
    if (writeModeFlags != ModeFlags::NONE)
    {
        writer = [addressRange, fixedPointFromDouble, fixedPointMask](connection::IDeviceInterface* device, double value) -> VoidResult
        {
            std::vector<uint32_t> data(1, 0); // Read current value to preserve other bits
            const auto readResult = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
            if (readResult.isOk() && readResult.getValue().size() >= 1)
            {
                data.at(0) = readResult.getValue().at(0);
            }

            data.at(0) = (data.at(0) & ~fixedPointMask) | fixedPointFromDouble(value);
            return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
        };
    }

    addValueAdapter(std::make_shared<PropertyValueArithmetic<double>>(property, minimum, maximum),
                    std::make_shared<PropertyAdapterValueDeviceSimple<double>>(
                        property,
                        createStatusFunction(readDeviceFlags, readModeFlags, writeDeviceFlags, writeModeFlags),
                        AdapterTaskCreator(m_weakThis), addressRange,
                        reader,
                        writer,
                        transformFunction));
}

PropertyAdapterValueDeviceSimple<unsigned>::ValueReader PropertiesWtc640::createUnsignedReader(const AddressRange& addressRange, uint32_t mask)
{
    assert(addressRange.getSize() == sizeof(uint32_t));

    return [addressRange, mask](connection::IDeviceInterface* device) -> ValueResult<unsigned>
    {
        const auto result = device->readTypedDataFromRange<uint32_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<unsigned>::createFromError(result);
        }

        assert(result.getValue().size() == 1);
        return result.getValue().front() & mask;
    };
}

PropertyAdapterValueDeviceSimple<unsigned>::ValueWriter PropertiesWtc640::createUnsignedWriter(const AddressRange& addressRange, uint32_t mask)
{
    assert(addressRange.getSize() == sizeof(uint32_t));

    return [addressRange, mask](connection::IDeviceInterface* device, unsigned value) -> VoidResult
    {
        assert((value & mask) == value);

        std::vector<uint32_t> data(1, 0);
        data.at(0) = value;
        return device->writeTypedData<uint32_t>(data, addressRange.getFirstAddress(), ProgressTask());
    };
}

PropertyAdapterValueDeviceSimple<signed>::ValueReader PropertiesWtc640::createSignedReader(const AddressRange& addressRange, uint32_t mask)
{
    assert(addressRange.getSize() == sizeof(uint32_t));

    return [addressRange](connection::IDeviceInterface* device) -> ValueResult<signed>
    {
        const auto result = device->readTypedDataFromRange<int32_t>(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<signed>::createFromError(result);
        }

        assert(result.getValue().size() == 1);
        return result.getValue().front();
    };
}

PropertyAdapterValueDeviceSimple<signed>::ValueWriter PropertiesWtc640::createSignedWriter(const AddressRange& addressRange, uint32_t mask)
{
    assert(addressRange.getSize() == sizeof(uint32_t));

    return [addressRange](connection::IDeviceInterface* device, signed value) -> VoidResult
    {
        std::vector<int32_t> data(1, 0);
        data.at(0) = value;
        return device->writeTypedData<int32_t>(data, addressRange.getFirstAddress(), ProgressTask());
    };
}

PropertyAdapterValueDeviceSimple<std::string>::ValueReader PropertiesWtc640::createStringReader(const AddressRange& addressRange)
{
    return [addressRange](connection::IDeviceInterface* device) -> ValueResult<std::string>
    {
        const auto result = device->readAddressRange(addressRange, ProgressTask());
        if (!result.isOk())
        {
            return ValueResult<std::string>::createFromError(result);
        }

        return dataToString(result.getValue());
    };
}

PropertyAdapterValueDeviceSimple<std::string>::ValueWriter PropertiesWtc640::createStringWriter(const AddressRange& addressRange)
{
    return [addressRange](connection::IDeviceInterface* device, const std::string& string)
    {
        auto data = stringToData(string);
        data.resize(addressRange.getSize(), 0);
        return device->writeData(data, addressRange.getFirstAddress(), ProgressTask());
    };
}

PropertyAdapterValueDeviceProgress<core::Palette>::ValueReader PropertiesWtc640::createPaletteReader(const AddressRange& rangeName, const AddressRange& rangeData, const std::string& paletteName)
{
    return [rangeName, rangeData, paletteName](connection::IDeviceInterface* device, ProgressController progressController) -> ValueResult<core::Palette>
    {
        using ResultType = ValueResult<core::Palette>;

        const auto progress = progressController.createTaskBound(utils::format("reading {}", paletteName), rangeName.getSize() + rangeData.getSize(), true);

        auto deserializePalette = [&](const std::vector<uint8_t>& namesData, const std::vector<uint8_t>& coloursData) -> ResultType
        {
            core::Palette palette;

            palette.setName(dataToString(namesData).data());

            core::Palette::ColorData yCbCr;
            assert(yCbCr.size() * 4 == MemorySpaceWtc640::PALETTE_DATA_SIZE);
            for (size_t colorIndex = 0, dataIndex = 0; colorIndex < yCbCr.size(); ++colorIndex)
            {
                dataIndex++;

                yCbCr[colorIndex][core::Palette::INDEX_Y]  = coloursData[dataIndex++];
                yCbCr[colorIndex][core::Palette::INDEX_CB] = coloursData[dataIndex++];
                yCbCr[colorIndex][core::Palette::INDEX_CR] = coloursData[dataIndex++];
            }
            palette.setYCbCr(yCbCr);

            return palette;
        };

        const auto nameResult = device->readAddressRange(rangeName, progress);
        if (!nameResult.isOk())
        {
            return ResultType::createFromError(nameResult);
        }

        const auto dataResult = device->readAddressRange(rangeData, progress);
        if (!dataResult.isOk())
        {
            return ResultType::createFromError(dataResult);
        }

        return deserializePalette(nameResult.getValue(), dataResult.getValue());
    };
}

PropertyAdapterValueDeviceProgress<core::Palette>::ValueWriter PropertiesWtc640::createPaletteWriter(const AddressRange& rangeName, const AddressRange& rangeData, const std::string& paletteName)
{
    return [rangeName, rangeData, paletteName](connection::IDeviceInterface* device, const core::Palette& palette, ProgressController progressController) -> VoidResult
    {
        std::vector<uint8_t> nameData(rangeName.getSize(), 0);
        nameData = stringToData(palette.getName());
        nameData.resize(rangeName.getSize(), 0);

        std::vector<uint8_t> coloursData(rangeData.getSize(), 0xFF);
        const auto& yCbCr = palette.getYCbCr();
        assert(yCbCr.size() * 4 == coloursData.size());
        for (size_t colorIndex = 0, offset = 0; colorIndex < yCbCr.size(); ++colorIndex)
        {
            coloursData.at(offset++) = 0;
            coloursData.at(offset++) = yCbCr[colorIndex][core::Palette::INDEX_Y];
            coloursData.at(offset++) = yCbCr[colorIndex][core::Palette::INDEX_CB];
            coloursData.at(offset++) = yCbCr[colorIndex][core::Palette::INDEX_CR];
        }

        const auto progress = progressController.createTaskBound(utils::format("writing {}", paletteName), nameData.size() + coloursData.size(), false);

        if (const auto result = device->writeData(nameData, rangeName.getFirstAddress(), progress); !result.isOk())
        {
            return result;
        }

        if (const auto result = device->writeData(coloursData, rangeData.getFirstAddress(), progress); !result.isOk())
        {
            return result;
        }

        return VoidResult::createOk();
    };
}

std::shared_ptr<PropertyValue<PropertiesWtc640::PresetId>> PropertiesWtc640::createPresetIdValueAdapter(PropertyId propertyId)
{
    auto valueAdapter = std::make_shared<PropertyValue<PresetId>>(propertyId);

    valueAdapter->setCustomConvertToStringFunction([](const PresetId& value)
    {
        std::vector<std::string> list;

        list.emplace_back(utils::format("Lens {}", Lens::ALL_ITEMS.at(value.lens).userName));
        list.emplace_back(utils::format("Range {}", Range::ALL_ITEMS.at(value.range).userName));
        list.emplace_back(utils::format("LensVariant {}", LensVariant::ALL_ITEMS.at(value.lensVariant).userName));
        list.emplace_back(utils::format("version {}", PresetVersion::ALL_ITEMS.at(value.version).userName));

        return utils::joinStringVector(list, ", ");
    });

    return valueAdapter;
}

std::string PropertiesWtc640::getArticleNumberSection(const std::string& articleNumber, ArticleSection articleSection)
{
    std::vector<std::string> articleTokens;
    boost::split(articleTokens, articleNumber, boost::is_any_of("-"));
    if (articleTokens.size() != ARTICLESECTION__END)
    {
        return std::string();
    }

    return articleTokens[static_cast<uint8_t>(articleSection)];
}

std::string PropertiesWtc640::getSerialNumberRegexPattern()
{
    return "^[0-9]{5}-[0-9]{3}-[0-9]{4}$";
}

std::string PropertiesWtc640::getArticleNumberRegexPattern()
{
    std::vector<std::string> pattern;

    {
        std::vector<std::string> allSensorTypes;
        for (const auto& [sensor, name] : ARTICLE_NUMBER_SENSORS)
        {
            allSensorTypes.emplace_back(name);
        }
        pattern.emplace_back(utils::format("({})", utils::joinStringVector(allSensorTypes, "|")));
    }

    {
        std::vector<std::string> allCoreTypes;
        for (const auto& [core, name] : ARTICLE_NUMBER_CORE_TYPES)
        {
            allCoreTypes.emplace_back(name);
        }
        pattern.emplace_back(utils::format("({})", utils::joinStringVector(allCoreTypes, "|")));
    }

    {
        std::vector<std::string> allDetectorSensitivities;
        for (const auto& [sensitivity, name] : ARTICLE_NUMBER_DETECTOR_SENSITIVITIES)
        {
            allDetectorSensitivities.emplace_back(name);
        }
        pattern.emplace_back(utils::format("({})", utils::joinStringVector(allDetectorSensitivities, "|")));
    }

    {
        std::vector<std::string> allFocusTypes;
        for (const auto& [focus, name] : ARTICLE_NUMBER_FOCUSES)
        {
            allFocusTypes.emplace_back(name);
        }
        pattern.emplace_back(utils::format("({})", utils::joinStringVector(allFocusTypes, "|")));
    }

    {
        std::vector<std::string> allFramerates;
        for (const auto& [framerate, name] : ARTICLE_NUMBER_FRAMERATES)
        {
            allFramerates.emplace_back(name);
        }
        pattern.emplace_back(utils::format("({})", utils::joinStringVector(allFramerates, "|")));
    }

    return utils::format("^{}$", utils::joinStringVector(pattern, "-"));
}

ValueResult<Sensor::Item> PropertiesWtc640::getSensorFromArticleNumber(const std::string& articleNumber)
{
    using ResultType = ValueResult<Sensor::Item>;

    const std::string value = getArticleNumberSection(articleNumber, ARTICLESECTION_SENSOR_TYPE);
    for (const auto& [sensor, name] : ARTICLE_NUMBER_SENSORS)
    {
        if (name == utils::stringToUpperTrimmed(value))
        {
            return sensor;
        }
    }

    return ResultType::createError("Article number error", utils::format("invalid sensor: {}", value));
}

ValueResult<Core::Item> PropertiesWtc640::getCoreTypeFromArticleNumber(const std::string& articleNumber)
{
    using ResultType = ValueResult<Core::Item>;

    const std::string value = getArticleNumberSection(articleNumber, ARTICLESECTION_CORE_TYPE);
    for (const auto& [core, name] : ARTICLE_NUMBER_CORE_TYPES)
    {
        if (name == utils::stringToUpperTrimmed(value))
        {
            return core;
        }
    }

    return ResultType::createError("Article number error", utils::format("invalid core type: {}", value));
}

ValueResult<DetectorSensitivity::Item> PropertiesWtc640::getDetectorSensitivityFromArticleNumber(const std::string& articleNumber)
{
    using ResultType = ValueResult<DetectorSensitivity::Item>;

    const std::string value = getArticleNumberSection(articleNumber, ARTICLESECTION_DETECTOR_TYPE);
    for (const auto& [sensitivity, name] : ARTICLE_NUMBER_DETECTOR_SENSITIVITIES)
    {
        if (name == utils::stringToUpperTrimmed(value))
        {
            return sensitivity;
        }
    }

    return ResultType::createError("Article number error", utils::format("invalid detector sensitivity: {}", value));
}

ValueResult<Focus::Item> PropertiesWtc640::getFocusFromArticleNumber(const std::string& articleNumber)
{
    using ResultType = ValueResult<Focus::Item>;

    const std::string value = getArticleNumberSection(articleNumber, ARTICLESECTION_FOCUS_TYPE);
    for (const auto& [focus, name] : ARTICLE_NUMBER_FOCUSES)
    {
        if (name == utils::stringToUpperTrimmed(value))
        {
            return focus;
        }
    }

    return ResultType::createError("Article number error", utils::format("invalid focus: {}", value));
}

ValueResult<Framerate::Item> PropertiesWtc640::getMaxFramerateFromArticleNumber(const std::string& articleNumber)
{
    using ResultType = ValueResult<Framerate::Item>;

    const std::string value = getArticleNumberSection(articleNumber, ARTICLESECTION_MAX_FRAMERATE);
    for (const auto& [framerate, name] : ARTICLE_NUMBER_FRAMERATES)
    {
        if (value == name)
        {
            return framerate;
        }
    }

    return ResultType::createError("Article number error", utils::format("invalid framerate: {}", value));
}

ValueResult<boost::posix_time::ptime> PropertiesWtc640::getDateFromSerialNumber(const std::string& serialNumber)
{
    if (serialNumber.length() < 4)
    {
        return ValueResult<boost::posix_time::ptime>::createError("Invalid serial number!", "too short");
    }

    std::string dateMonth = serialNumber.substr(serialNumber.length() - 4, 4);
    std::string year;
    std::string month;

    year.append(1, dateMonth[0]);
    year.append(1, dateMonth[1]);
    month.append(1, dateMonth[2]);
    month.append(1, dateMonth[3]);

    if (!std::all_of(year.begin(), year.end(), ::isdigit))
    {
        return ValueResult<boost::posix_time::ptime>::createError("Invalid date!", "year is not a number");
    }

    if (!std::all_of(month.begin(), month.end(), ::isdigit))
    {
        return ValueResult<boost::posix_time::ptime>::createError("Invalid date!", "month is not a number");
    }

    const int monthInt = std::stoi(month);
    if (monthInt < 1 || monthInt > 12)
    {
        return ValueResult<boost::posix_time::ptime>::createError("Invalid date!", "month is out of range");
    }

    const auto dateString = utils::format("20{}01{}", year, month);
    auto date = boost::gregorian::from_undelimited_string(dateString);
    if (date.is_not_a_date())
    {
        return ValueResult<boost::posix_time::ptime>::createError("Invalid date!", dateString);
    }

    boost::posix_time::ptime date2(date);
    return date2;
}

std::string PropertiesWtc640::dataToString(const std::vector<uint8_t>& data)
{
    std::string string;
    string.reserve(data.size());
    string.append(reinterpret_cast<const char*>(data.data()), data.size());
    string.erase(std::remove_if(string.begin(), string.end(), [](unsigned char x)
    {
        return std::iscntrl(x);
    }), string.end());
    return string;
}

std::vector<uint8_t> PropertiesWtc640::stringToData(const std::string& string)
{
    std::vector<uint8_t> data(string.length(), 0);

    for (int i = 0; i < string.length(); ++i)
    {
        data.at(i) = string[i];
    }

    return data;
}

std::string PropertiesWtc640::getAttributePropertyIdString(unsigned int presetIndex, PresetAttribute attribute)
{
    switch (attribute)
    {
        case PRESETATTRIBUTE_TIMESTAMP:
            return utils::format("PRESET_{}_TIMESTAMP", presetIndex + 1);

        case PRESETATTRIBUTE_PRESET_ID:
            return utils::format("PRESET_{}_PRESET_ID", presetIndex + 1);

        case PRESETATTRIBUTE_GAIN_MATRIX:
            return utils::format("PRESET_{}_GAIN_MATRIX", presetIndex + 1);

        case PRESETATTRIBUTE_ONUC_MATRIX:
            return utils::format("PRESET_{}_ONUC_MATRIX", presetIndex + 1);

        case PRESETATTRIBUTE_SNUC_TABLE:
            return utils::format("PRESET_{}_SNUC_TABLE", presetIndex + 1);

        default:
            assert(false);
            return std::string();
    }
}

std::string PropertiesWtc640::createCompositeValueString(const std::vector<std::pair<std::string, VoidResult> >& components)
{
    std::vector<std::string> list;

    for (const auto& [name, result] : components)
    {
        list.emplace_back(utils::format("{} {}", name, result.isOk() ? "OK" : "Err"));
    }

    return utils::joinStringVector(list, "\n");
}

template<class ValueType, typename... Args>
void PropertiesWtc640::addValueAdapterImpl(const std::shared_ptr<PropertyValueEnum<ValueType>>& value, Args&&... args)
{
    addValueAdapter(value, std::make_shared<PropertyAdapterValueDeviceSimple<ValueType>>(std::forward<Args>(args)...));
}

template<typename... Args>
void PropertiesWtc640::addValueAdapterImpl(const std::shared_ptr<PropertyValueEnum<VideoFormat::Item>>& value, Args&&... args)
{
    addValueAdapter(value, std::make_shared<VideoFormatAdapter>(this, std::forward<Args>(args)...));
}

PropertiesWtc640::ConnectionInfoTransaction::ConnectionInfoTransaction(const PropertiesTransaction& propertiesTransaction) :
    m_propertiesTransaction(propertiesTransaction)
{
}

const connection::Stats& PropertiesWtc640::ConnectionInfoTransaction::getConnectionStats() const
{
    if (!m_connectionStats.has_value())
    {
        m_connectionStats = getProperties()->getDeviceInterfaceWtc640()->getStatus()->getStatsCopy();
    }

    return m_connectionStats.value();
}

PropertiesWtc640* PropertiesWtc640::ConnectionInfoTransaction::getProperties() const
{
    return boost::polymorphic_downcast<PropertiesWtc640*>(m_propertiesTransaction.getProperties().get());
}

PropertiesWtc640::ConnectionStateTransaction::ConnectionStateTransaction(const std::shared_ptr<ConnectionStateTransactionData>& connectionTransactionData) :
    m_connectionStateTransactionData(connectionTransactionData)
{
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::connectUart(const core::connection::SerialPortInfo& portInfo, Baudrate::Item baudrate) const
{
    const auto connectionResult = connection::DataLinkUart::createConnection(portInfo, baudrate);
    if (!connectionResult.isOk())
    {
        return connectionResult.toVoidResult();
    }

    if (const auto result = setDataLinkInterface(connectionResult.getValue()); !result.isOk())
    {
        return result;
    }

    getProperties()->m_lastConnectedUartPort = portInfo;
    getProperties()->m_lastConnectedEbusDevice = std::nullopt;
    return VoidResult::createOk();
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::connectUartAuto(const std::vector<core::connection::SerialPortInfo>& ports, ProgressController progressController) const
{
    static constexpr std::string_view ERROR_CONNECT_FAILED = "Connect failed.";
    static constexpr std::string_view ERROR_NO_PORTS = "Error, no ports available.";
    static constexpr std::string_view ERROR_ANY_PORT = "Error, unable to connect to any port.";

    auto task = progressController.createTaskUnbound("Connecting to UART port(s).", true);

    std::vector<std::string> resultMessages;
    for (const auto& [baudrate, description] : core::BaudrateWtc::ALL_ITEMS | std::views::reverse)
    {
        for (const auto& port: ports)
        {
            task.sendProgressMessage(utils::format("Trying port {}, {} bps", port.systemLocation, core::Baudrate::getBaudrateSpeed(baudrate)));

            auto result = connectUart(port, baudrate);
            if (task.isCancelled() || result.isOk())
            {
                return VoidResult::createOk();
            }

            resultMessages.emplace_back(utils::format("baudrate {}: {}", core::Baudrate::getBaudrateSpeed(baudrate), result.getDetailErrorMessage()));
        }
    }

    auto result = VoidResult::createError(ERROR_ANY_PORT.data());
    if (ports.size() == 0)
    {
        result = VoidResult::createError(ERROR_NO_PORTS.data());
    }
    else if (ports.size() == 1)
    {
        result = VoidResult::createError(ERROR_CONNECT_FAILED.data(), utils::joinStringVector(resultMessages, "\n"));
    }

    task.sendErrorMessage(result.toString());
    return result;
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::connectEbus(const connection::EbusDevice& device) const
{
    static constexpr connection::EbusSerialPort EBUS_SERIAL_PORT = connection::EBUS_SERIAL_PORT_BULK_0;
    static constexpr Baudrate::Item EBUS_SERIAL_BAUDRATE = Baudrate::Item::B_921600;

    if (getProperties()->m_ebusPlugin == nullptr)
    {
        return VoidResult::createError("Gige not accessible!", "ebusPlugin not loaded");
    }

    const auto connectionResult = getProperties()->m_ebusPlugin->createConnection(device, EBUS_SERIAL_BAUDRATE, EBUS_SERIAL_PORT);
    if (!connectionResult.isOk())
    {
        return connectionResult.toVoidResult();
    }

    if (const auto result = setDataLinkInterface(connectionResult.getValue()); !result.isOk())
    {
        return result;
    }

    getProperties()->m_lastConnectedUartPort = std::nullopt;
    getProperties()->m_lastConnectedEbusDevice = device;
    return VoidResult::createOk();
}

void PropertiesWtc640::ConnectionStateTransaction::disconnectCore() const
{
    const auto result = setDataLinkInterface(nullptr);
    assert(result.isOk());
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::reconnectCore() const
{
    disconnectCore();

    const uint16_t RECONNECT_DELAY_FOR_TCP = 5000;

    if (getProperties()->m_lastConnectedUartPort.has_value())
    {
        for (const auto& [baudrate, description] : BaudrateWtc::ALL_ITEMS | std::views::reverse)
        {
            if (connectUart(getProperties()->m_lastConnectedUartPort.value(), baudrate).isOk())
            {
                return VoidResult::createOk();
            }
        }
    }

    else if (getProperties()->m_lastConnectedEbusDevice.has_value())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_FOR_TCP));
        if (connectEbus(getProperties()->m_lastConnectedEbusDevice.value()).isOk())
        {
            return VoidResult::createOk();
        }
    }

    return VoidResult::createError("Unable to reconnect!", "");
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::reconnectCoreAfterReset(const std::optional<Baudrate::Item>& oldBaudrate) const
{
#if defined(__APPLE__)
    static constexpr int attemptsCount = 6;
#else
    static constexpr int attemptsCount = 3;
#endif
    for (int i = 0; i < attemptsCount; ++i)
    {
        if (i > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2'000));
        }

        if (const auto reconnectResult = reconnectCore(); reconnectResult.isOk())
        {
            if (oldBaudrate.has_value())
            {
                const auto setBaudrateResult = openConnectionExclusiveTransactionWtc640().setCoreBaudrate(oldBaudrate.value());
                if (!setBaudrateResult.isOk())
                {
                    disconnectCore();
                    return setBaudrateResult;
                }
            }

            return VoidResult::createOk();
        }
    }

    return VoidResult::createError("Unable to reconnect!", utils::format("all attempts failed - attempts count: {}", attemptsCount));
}

std::optional<DeviceType> PropertiesWtc640::ConnectionStateTransaction::getCurrentDeviceType() const
{
    return m_connectionStateTransactionData->getCurrentDeviceType();
}

std::optional<Baudrate::Item> PropertiesWtc640::ConnectionStateTransaction::getCurrentBaudrate() const
{
    return getProperties()->getCurrentBaudrateImpl();
}

PropertiesWtc640::ConnectionExclusiveTransactionWtc640 PropertiesWtc640::ConnectionStateTransaction::openConnectionExclusiveTransactionWtc640() const
{
    return ConnectionExclusiveTransactionWtc640(m_connectionStateTransactionData->createConnectionExclusiveTransaction());
}

PropertiesWtc640* PropertiesWtc640::ConnectionStateTransaction::getProperties() const
{
    return boost::polymorphic_downcast<PropertiesWtc640*>(m_connectionStateTransactionData->getProperties().get());
}

VoidResult PropertiesWtc640::ConnectionStateTransaction::setDataLinkInterface(const std::shared_ptr<connection::IDataLinkInterface>& dataLinkInterface) const
{
    auto* deviceInterface = boost::polymorphic_downcast<connection::DeviceInterfaceWtc640*>(m_connectionStateTransactionData->getDeviceInterface());
    auto* protocolInterface = boost::polymorphic_downcast<connection::ProtocolInterfaceTCSI*>(deviceInterface->getProtocolInterface().get());
    assert(protocolInterface != nullptr);

    m_connectionStateTransactionData->setCurrentDeviceType(std::nullopt);
    deviceInterface->setMemorySpace(MemorySpaceWtc640::getDeviceSpace(m_connectionStateTransactionData->getCurrentDeviceType()));
    getProperties()->m_connectionLostSent = false;
    getProperties()->m_dataLinkInterface = dataLinkInterface;
    protocolInterface->setDataLinkInterface(getProperties()->m_dataLinkInterface);

    getProperties()->removeDynamicPresetAdapters();
    getProperties()->removeDynamicUsbAdapters();

    if (dataLinkInterface == nullptr)
    {
        getProperties()->addDummyDynamicUsbAdapters();
        return VoidResult::createOk();
    }

    deviceInterface->getStatus()->resetStats();
    deviceInterface->getAccumulatedRegisterChangesAndReset();

    ValueResult<DeviceType> deviceTypeResult;
    {
        const auto transaction = openConnectionExclusiveTransactionWtc640();

        deviceTypeResult = testDeviceType(transaction.getConnectionExclusiveTransaction());
    }
    if (!deviceTypeResult.isOk())
    {
        getProperties()->m_connectionLostSent = false;
        getProperties()->m_dataLinkInterface = nullptr;
        protocolInterface->setDataLinkInterface(getProperties()->m_dataLinkInterface);

        getProperties()->addDummyDynamicUsbAdapters();

        return deviceTypeResult.toVoidResult();
    }

    m_connectionStateTransactionData->setCurrentDeviceType(deviceTypeResult.getValue());
    deviceInterface->setMemorySpace(MemorySpaceWtc640::getDeviceSpace(m_connectionStateTransactionData->getCurrentDeviceType()));

    getProperties()->addDynamicUsbAdapters();
    if (deviceTypeResult.getValue() == DevicesWtc640::MAIN_USER)
    {
        getProperties()->addDynamicPresetAdapters(deviceInterface);
    }

    return VoidResult::createOk();
}

PropertiesWtc640::ConnectionExclusiveTransactionWtc640::ConnectionExclusiveTransactionWtc640(const ConnectionExclusiveTransaction& connectionExclusiveTransaction) :
    m_connectionExclusiveTransaction(connectionExclusiveTransaction)
{
}

const Properties::ConnectionExclusiveTransaction& PropertiesWtc640::ConnectionExclusiveTransactionWtc640::getConnectionExclusiveTransaction() const
{
    return m_connectionExclusiveTransaction;
}

VoidResult PropertiesWtc640::ConnectionExclusiveTransactionWtc640::setCoreBaudrate(Baudrate::Item baudrate) const
{
    auto* protocolInterface = boost::polymorphic_downcast<connection::ProtocolInterfaceTCSI*>(getProperties()->getDeviceInterfaceWtc640()->getProtocolInterface().get());
    auto* datalinkUart = dynamic_cast<connection::DataLinkUart*>(protocolInterface->getDataLinkInterface().get());

    if (datalinkUart == nullptr)
    {
        return VoidResult::createError("Unable to set baudrate - no uart connection!", "");
    }

    const auto currentDatalinkBaudrateResult = datalinkUart->getBaudrate();
    if (currentDatalinkBaudrateResult.isOk() && currentDatalinkBaudrateResult.getValue() == baudrate)
    {
        return VoidResult::createOk();
    }

    // nelze zapisovat pres propery - nejde pri state transakci !
    std::vector<uint8_t> data(MemorySpaceWtc640::UART_BAUDRATE_CURRENT.getSize(), 0);
    data.at(0) = static_cast<uint8_t>(baudrate);
    const auto coreBaudrateResult = m_connectionExclusiveTransaction.writeData<uint8_t>(data, MemorySpaceWtc640::UART_BAUDRATE_CURRENT.getFirstAddress());
    if (!coreBaudrateResult.isOk())
    {
        return coreBaudrateResult;
    }

    const auto datalinkBaudrateResult = datalinkUart->setBaudrate(baudrate);
    if (!datalinkBaudrateResult.isOk())
    {
        return datalinkBaudrateResult;
    }

    const ElapsedTimer timer(std::chrono::milliseconds(5000));
    while (!timer.timedOut())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (getProperties()->testDeviceType(m_connectionExclusiveTransaction).isOk())
        {
            return VoidResult::createOk();
        }
    }

    return VoidResult::createError("Set baudrate failed!", utils::format("baudrate: {} - invalid device type", Baudrate::getBaudrateSpeed(baudrate)));
}

VoidResult PropertiesWtc640::ConnectionExclusiveTransactionWtc640::activateCommonTriggerAndWaitTillFinished(CommonTrigger::Item trigger) const
{
    const auto triggerResult = activateTrigger<CommonTrigger>(trigger);
    if (!triggerResult.isOk())
    {
        return triggerResult;
    }

    if (trigger == CommonTrigger::Item::CLEAN_USER_DP)
    {
        getConnectionExclusiveTransaction().getPropertiesTransaction().resetValue(PropertyIdWtc640::DEAD_PIXELS_CURRENT);
    }

    return waitTillTriggerFinished<CommonTrigger>(trigger);
}

VoidResult PropertiesWtc640::ConnectionExclusiveTransactionWtc640::activateResetTriggerAndWaitTillFinished(ResetTrigger::Item trigger) const
{
    const auto triggerResult = activateTrigger<ResetTrigger>(trigger);
    if (!triggerResult.isOk())
    {
        return triggerResult;
    }

    static_cast<void>(waitTillTriggerFinished<ResetTrigger>(trigger));
    // trigger was succesfully activated, waitTillTriggerFinished may fail, because core is restarting -> OK
    return VoidResult::createOk();
}

template<class TriggerType>
VoidResult PropertiesWtc640::ConnectionExclusiveTransactionWtc640::activateTrigger(typename TriggerType::Item trigger) const
{
    const auto triggerRegister = getTriggerAddressRange<TriggerType>(trigger);
    if (!triggerRegister.isOk())
    {
        return triggerRegister.toVoidResult();
    }

    const auto triggerMask = TriggerType::getMask(trigger);

    std::vector<uint32_t> data(1, triggerMask);
    return m_connectionExclusiveTransaction.writeData<uint32_t>(data, triggerRegister.getValue().getFirstAddress());
}

template<class TriggerType>
VoidResult PropertiesWtc640::ConnectionExclusiveTransactionWtc640::waitTillTriggerFinished(typename TriggerType::Item trigger) const
{
    const ElapsedTimer timer(std::chrono::milliseconds(10'000));
    while (!timer.timedOut())
    {
        const auto triggerResult = m_connectionExclusiveTransaction.readData<uint32_t>(MemorySpaceWtc640::STATUS.getFirstAddress(), 1);
        if (!triggerResult.isOk())
        {
            return triggerResult.toVoidResult();
        }
        assert(triggerResult.getValue().size() == 1);

        if (!StatusWtc640(triggerResult.getValue().front()).isAnyTriggerActive())
        {
            return VoidResult::createOk();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return VoidResult::createError("Trigger unfinished!", utils::format("timedout: {}[ms]", timer.getElapsedMilliseconds()));
}

template<class TriggerType>
ValueResult<connection::AddressRange> PropertiesWtc640::ConnectionExclusiveTransactionWtc640::getTriggerAddressRange(typename TriggerType::Item trigger) const
{
    using ResultType = ValueResult<connection::AddressRange>;

    const auto deviceType = m_connectionExclusiveTransaction.getPropertiesTransaction().getProperties()->getCurrentDeviceType(m_connectionExclusiveTransaction.getPropertiesTransaction());
    if (!deviceType.has_value())
    {
        return ResultType::createError("Access denied!", "unknown device");
    }

    const auto addressRange = TriggerType::getAddressRange(trigger, deviceType.value());
    assert(!addressRange.isOk() || addressRange.getValue().getSize() == sizeof(uint32_t));
    return addressRange;
}

ValueResult<std::vector<std::vector<uint16_t>>> PropertiesWtc640::ConnectionExclusiveTransactionWtc640::captureImages(int imagesCount, ProgressController progressController) const
{
    using ResultType = ValueResult<std::vector<std::vector<uint16_t>>>;

    const auto& transaction = m_connectionExclusiveTransaction.getPropertiesTransaction();
    const auto imageFreezeValue = transaction.getValue<bool>(core::PropertyIdWtc640::IMAGE_FREEZE);
    if (imageFreezeValue.hasResult() && imageFreezeValue.getResult().isOk() && imageFreezeValue.getResult().getValue() && imagesCount != 1)
    {
        const auto RESULT = ResultType::createError("Single frozen image capture is only allowed", "");
        progressController.sendErrorMessage(RESULT.toString());
        return RESULT;
    }

    uint32_t captureAddress {0};
    {
        auto task = progressController.createTaskUnbound("Image capture", true);
        const auto captureAddressResult = captureImagesAndReadAddress(imagesCount);
        if (!captureAddressResult.isOk())
        {
            const auto RESULT = ResultType::createFromError(captureAddressResult);
            task.sendErrorMessage(RESULT.toString());
            return RESULT;
        }

        captureAddress = captureAddressResult.getValue();
    }

    return readCapturedFrames(imagesCount, captureAddress, progressController);
}

ValueResult<PostProcessingMatrices> PropertiesWtc640::ConnectionExclusiveTransactionWtc640::getPostProcessingMatrices(ProgressController progressController) const
{
    static constexpr int16_t FACTOR = 1 << 14;

    using ResultType = ValueResult<PostProcessingMatrices>;

    auto task = progressController.createTaskBound("Getting post processing matrices", core::DevicesWtc640::WIDTH * core::DevicesWtc640::HEIGHT * 4 * 2, false);
    auto futureResult = m_connectionExclusiveTransaction.getPropertiesTransaction().readDataWithProgress<uint16_t>(core::connection::MemorySpaceWtc640::RAM_CALIBRATION_MATRICE.getFirstAddress(), core::DevicesWtc640::WIDTH*core::DevicesWtc640::HEIGHT*4, task);
    const auto valueResult = futureResult.get();
    if(!valueResult.isOk())
    {
        return ResultType::createError("Could not retrieve post processing matrice from RAM!", valueResult.toString());
    }

    auto matrices = PostProcessingMatrices();
    matrices.nuc.reserve(DevicesWtc640::HEIGHT*DevicesWtc640::WIDTH);
    matrices.onuc.reserve(DevicesWtc640::HEIGHT*DevicesWtc640::WIDTH);
    matrices.offset.reserve(DevicesWtc640::HEIGHT*DevicesWtc640::WIDTH);
    const auto data = valueResult.getValue();

    for(size_t i = 1; i < data.size(); i += 4)
    {
        matrices.onuc.push_back(static_cast<int16_t>(data[i]));
        matrices.nuc.push_back(static_cast<float>(static_cast<int16_t>(data[i+1])) / FACTOR);
        matrices.offset.push_back(static_cast<int16_t>(data[i + 2]));
    }

    return matrices;
}

ValueResult<uint32_t> PropertiesWtc640::ConnectionExclusiveTransactionWtc640::captureImagesAndReadAddress(uint8_t framesToCaptureCount) const
{
    using ResultType = ValueResult<uint32_t>;

    static_assert(MemorySpaceWtc640::NUMBER_OF_FRAMES_TO_CAPTURE.getSize() == sizeof(uint32_t));
    const std::vector<uint32_t> framesToCaptureValue(1, framesToCaptureCount);
    assert(MemorySpaceWtc640::NUMBER_OF_FRAMES_TO_CAPTURE.getSize() == sizeof(uint32_t));
    if (const auto result = m_connectionExclusiveTransaction.writeData<uint32_t>(framesToCaptureValue, MemorySpaceWtc640::NUMBER_OF_FRAMES_TO_CAPTURE.getFirstAddress()); !result.isOk())
    {
        return ResultType::createError("Image capture failed!", utils::format("count write: {}", result.getDetailErrorMessage()));
    }

    if (const auto result = activateCommonTriggerAndWaitTillFinished(CommonTrigger::Item::FRAME_CAPTURE_START); !result.isOk())
    {
        return ResultType::createError("Image capture failed!", utils::format("trigger: {}", result.getDetailErrorMessage()));
    }

    const auto captureAddress = m_connectionExclusiveTransaction.readData<uint32_t>(MemorySpaceWtc640::CAPTURE_FRAME_ADDRESS.getFirstAddress(), 1);
    if (!captureAddress.isOk())
    {
        return ResultType::createError("Image capture failed!", utils::format("address read: {}", captureAddress.getDetailErrorMessage()));
    }
    assert(captureAddress.getValue().size() == 1);

    return captureAddress.getValue().front();
}

ValueResult<std::vector<std::vector<uint16_t>>> PropertiesWtc640::ConnectionExclusiveTransactionWtc640::readCapturedFrames(uint8_t framesToCaptureCount, uint32_t captureAddress,
                                                                                                                                                        ProgressController progressController) const
{
    using ResultType = ValueResult<std::vector<std::vector<uint16_t>>>;

    const auto pixelsCount = DevicesWtc640::WIDTH * DevicesWtc640::HEIGHT;
    const auto bytesPerImage = pixelsCount * sizeof(uint16_t);

    auto task = progressController.createTaskBound("Image capture", bytesPerImage * framesToCaptureCount, true);

    std::vector<std::vector<uint16_t>> images;
    images.reserve(framesToCaptureCount);

    for (size_t i = 0; i < framesToCaptureCount; ++i)
    {
        const auto frameResult = m_connectionExclusiveTransaction.readDataWithProgress<uint16_t>(captureAddress + i * bytesPerImage, pixelsCount,
                                                                                                 task);
        if (!frameResult.isOk())
        {
            const auto RESULT = ResultType::createError("Image capture failed!", utils::format("frame {} read: {}", i + 1, frameResult.getDetailErrorMessage()));
            task.sendErrorMessage(RESULT.toString());
            return RESULT;
        }

        images.push_back(frameResult.getValue());
    }

    return images;
}

PropertiesWtc640::ConnectionStateTransaction PropertiesWtc640::ConnectionExclusiveTransactionWtc640::openConnectionStateTransaction() const
{
    const ConnectionStateTransaction stateTransaction(getProperties()->createConnectionStateTransactionDataFromConnectionExclusiveTransaction(m_connectionExclusiveTransaction));
    stateTransaction.disconnectCore();

    return stateTransaction;
}

PropertiesWtc640* PropertiesWtc640::ConnectionExclusiveTransactionWtc640::getProperties() const
{
    return boost::polymorphic_downcast<PropertiesWtc640*>(m_connectionExclusiveTransaction.getPropertiesTransaction().getProperties().get());
}

bool PropertiesWtc640::ArticleNumber::isAllComponentsOk() const
{
    return sensor.isOk() &&
            coreType.isOk() &&
            detectorSensitivity.isOk() &&
            focus.isOk() &&
            maxFramerate.isOk();
}

ValueResult<std::string> PropertiesWtc640::ArticleNumber::toString() const
{
    using ResultType = ValueResult<std::string>;

    if (!sensor.isOk())
    {
        return ResultType::createFromError(sensor);
    }

    if (!coreType.isOk())
    {
        return ResultType::createFromError(coreType);
    }

    if (!detectorSensitivity.isOk())
    {
        return ResultType::createFromError(detectorSensitivity);
    }

    if (!focus.isOk())
    {
        return ResultType::createFromError(focus);
    }

    if (!maxFramerate.isOk())
    {
        return ResultType::createFromError(maxFramerate);
    }

    std::vector<std::string> list;
    list.emplace_back(ARTICLE_NUMBER_SENSORS.at(sensor.getValue()));
    list.emplace_back(ARTICLE_NUMBER_CORE_TYPES.at(coreType.getValue()));
    list.emplace_back(ARTICLE_NUMBER_DETECTOR_SENSITIVITIES.at(detectorSensitivity.getValue()));
    list.emplace_back(ARTICLE_NUMBER_FOCUSES.at(focus.getValue()));
    list.emplace_back(ARTICLE_NUMBER_FRAMERATES.at(maxFramerate.getValue()));

    return utils::joinStringVector(list, "-");
}

ValueResult<PropertiesWtc640::ArticleNumber> PropertiesWtc640::ArticleNumber::createFromString(const std::string& string)
{
    using ResultType = ValueResult<PropertiesWtc640::ArticleNumber>;

    const auto articleNumberString = utils::stringToUpperTrimmed(string);

    const boost::basic_regex regex(getArticleNumberRegexPattern());
    if (!boost::regex_search(string, regex))
    {
        return ResultType::createError("Invalid format", utils::format("{}\nExpected pattern: {}", string, regex.expression()));
    }

    const auto sensorResult = getSensorFromArticleNumber(articleNumberString);
    if (!sensorResult.isOk())
    {
        return ResultType::createFromError(sensorResult);
    }

    const auto coreTypeResult = getCoreTypeFromArticleNumber(articleNumberString);
    if (!coreTypeResult.isOk())
    {
        return ResultType::createFromError(coreTypeResult);
    }

    const auto detectorSensitivityResult = getDetectorSensitivityFromArticleNumber(articleNumberString);
    if (!detectorSensitivityResult.isOk())
    {
        return ResultType::createFromError(detectorSensitivityResult);
    }

    const auto focusResult = getFocusFromArticleNumber(articleNumberString);
    if (!focusResult.isOk())
    {
        return ResultType::createFromError(focusResult);
    }

    const auto maxFramerateResult = getMaxFramerateFromArticleNumber(articleNumberString);
    if (!maxFramerateResult.isOk())
    {
        return ResultType::createFromError(maxFramerateResult);
    }

    ArticleNumber articleNumber;
    articleNumber.sensor = sensorResult.getValue();
    articleNumber.coreType = coreTypeResult.getValue();
    articleNumber.detectorSensitivity = detectorSensitivityResult.getValue();
    articleNumber.focus = focusResult.getValue();
    articleNumber.maxFramerate = maxFramerateResult.getValue();

    return articleNumber;
}

bool PropertiesWtc640::PresetId::isDefined() const
{
    return lens != Lens::Item::NOT_DEFINED && range != Range::Item::NOT_DEFINED;
}

ValueResult<connection::AddressRange> CommonTrigger::getAddressRange(Item trigger, DeviceType deviceType)
{
    using ResultType = ValueResult<connection::AddressRange>;

    switch (trigger)
    {
        case Item::NUC_OFFSET_UPDATE:
        case Item::CLEAN_USER_DP:
        case Item::SET_SELECTED_PRESET:
        case Item::MOTORFOCUS_CALIBRATION:
        case Item::FRAME_CAPTURE_START:
            if (deviceType != DevicesWtc640::MAIN_USER)
            {
                return ResultType::createError("Access denied!", utils::format("not main device, CommonTrigger: {}", static_cast<int>(trigger)));
            }
            return connection::MemorySpaceWtc640::TRIGGER;

        default:
            assert(false);
            return ResultType::createError("Unknown trigger!", utils::format("CommonTrigger: {}", static_cast<int>(trigger)));
    }
}

uint32_t CommonTrigger::getMask(Item trigger)
{
    switch (trigger)
    {
        case Item::NUC_OFFSET_UPDATE:
            return 1 << 2;

        case Item::CLEAN_USER_DP:
            return 1 << 3;

        case Item::SET_SELECTED_PRESET:
            return 1 << 4;

        case Item::MOTORFOCUS_CALIBRATION:
            return 1 << 5;

        case Item::FRAME_CAPTURE_START:
            return 1 << 6;

        default:
            assert(false);
            return 0;
    }
}

ValueResult<connection::AddressRange> ResetTrigger::getAddressRange(Item trigger, DeviceType deviceType)
{
    using ResultType = ValueResult<connection::AddressRange>;

    switch (trigger)
    {
        case Item::STAY_IN_LOADER:
        case Item::RESET_FROM_LOADER:
            if (deviceType != DevicesWtc640::LOADER)
            {
                return ResultType::createError("Access denied!", utils::format("not loader device, ResetTrigger: {}", static_cast<int>(trigger)));
            }
            return connection::MemorySpaceWtc640::TRIGGER;

        case Item::SOFTWARE_RESET:
            if (deviceType != DevicesWtc640::MAIN_USER)
            {
                return ResultType::createError("Access denied!", utils::format("not main device, ResetTrigger: {}", static_cast<int>(trigger)));
            }
            return connection::MemorySpaceWtc640::TRIGGER;
        case Item::RESET_TO_LOADER:
            return connection::MemorySpaceWtc640::TRIGGER;
        case Item::RESET_TO_FACTORY_DEFAULT:
            if (deviceType != DevicesWtc640::MAIN_USER)
            {
                return ResultType::createError("Access denied!", utils::format("not main device, ResetTrigger: {}", static_cast<int>(trigger)));
            }
            return connection::MemorySpaceWtc640::TRIGGER;

        default:
            assert(false);
            return ResultType::createError("Unknown trigger!", utils::format("ResetTrigger: {}", static_cast<int>(trigger)));
    }
}

uint32_t ResetTrigger::getMask(Item trigger)
{
    switch (trigger)
    {
        case Item::RESET_FROM_LOADER:
        case Item::SOFTWARE_RESET:
            return 1 << 0;

        case Item::STAY_IN_LOADER:
        case Item::RESET_TO_LOADER:
            return 1 << 1;

        case Item::RESET_TO_FACTORY_DEFAULT:
            return 1 << 7;

        default:
            assert(false);
            return 0;
    }
}
} // namespace core
