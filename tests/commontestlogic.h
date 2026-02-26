#pragma once

#include "testfixture.h"
#include "core/stream/istream.h"
#include "core/utils.h"
#include "core/wtc640/devicewtc640.h"
#include "core/wtc640/deadpixels.h"

#include <algorithm>
#include <functional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

template<typename T>
void testReadResetSet(core::Properties::ConnectionExclusiveTransaction& exclusiveTransaction, core::PropertyId id)
{
    auto propertiesTransaction = exclusiveTransaction.getPropertiesTransaction();
    const bool readable = propertiesTransaction.isPropertyReadable(id);
    const bool writable = propertiesTransaction.isPropertyWritable(id);

    if (writable && readable)
    {
        auto result = propertiesTransaction.getValue<T>(id);
        BOOST_CHECK_MESSAGE(result.containsValue(), utils::format("Getting value failed with error - {} for {}", result.getResult().toString(), id.getIdString()));
        if (!result.containsValue())
        {
            return;
        }
        const auto original = result.getValue();

        {
            propertiesTransaction.resetValue(id);
        }

        {
            auto result = propertiesTransaction.setValue<T>(id, original);
            BOOST_CHECK_MESSAGE(result.isOk(), utils::format("setValue failed with error - {} for {}", result.toString(), id.getIdString()));
        }
    }
    else if(readable)
    {
        auto result = propertiesTransaction.getValue<T>(id);
        BOOST_CHECK_MESSAGE(result.containsValue(), utils::format("Getting value failed with error - {} for {}", result.getResult().toString(), id.getIdString()));
    }
    else if(writable)
    {
        BOOST_CHECK_MESSAGE(false, utils::format("Property id {} is write only, no idea what to set!", id.getIdString()));
    }
    else
    {
        BOOST_CHECK_MESSAGE(false, utils::format("Property id {} is not readable or writable!", id.getIdString()));
    }
}

using Dispatcher = std::function<void(core::Properties::ConnectionExclusiveTransaction&, core::PropertyId)>;
const std::unordered_map<std::type_index, Dispatcher> DISPATCH_TABLE =
{
    { typeid(bool), [](auto& transaction, auto propertyid){ testReadResetSet<bool>(transaction, propertyid); } },
    { typeid(unsigned int), [](auto& transaction, auto propertyid){ testReadResetSet<unsigned int>(transaction, propertyid); } },
    { typeid(signed int), [](auto& transaction, auto propertyid){ testReadResetSet<signed int>(transaction, propertyid); } },
    { typeid(double), [](auto& transaction, auto propertyid){ testReadResetSet<double>(transaction, propertyid); } },
    { typeid(std::string), [](auto& transaction, auto propertyid){ testReadResetSet<std::string>(transaction, propertyid); } },
    { typeid(boost::posix_time::ptime), [](auto& transaction, auto propertyid){ testReadResetSet<boost::posix_time::ptime>(transaction, propertyid); } },

    { typeid(core::StatusWtc640), [](auto& transaction, auto propertyid){ testReadResetSet<core::StatusWtc640>(transaction, propertyid); } },
    { typeid(core::Version), [](auto& transaction, auto propertyid){ testReadResetSet<core::Version>(transaction, propertyid); } },
    { typeid(core::Palette), [](auto& transaction, auto propertyid){ testReadResetSet<core::Palette>(transaction, propertyid); } },
    { typeid(core::DeadPixels), [](auto& transaction, auto propertyid){ testReadResetSet<core::DeadPixels>(transaction, propertyid); } },

    { typeid(core::PropertiesWtc640::ImageFlip), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::ImageFlip>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::ArticleNumber), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::ArticleNumber>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::Conbright), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::Conbright>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::PresetId), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::PresetId>(transaction, propertyid); } },
    { typeid(std::vector<core::PropertiesWtc640::PresetId>), [](auto& transaction, auto propertyid){ testReadResetSet<std::vector<core::PropertiesWtc640::PresetId>>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::NucMatrix), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::NucMatrix>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::SnucMatrix), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::SnucMatrix>(transaction, propertyid); } },
    { typeid(core::PropertiesWtc640::GskTable), [](auto& transaction, auto propertyid){ testReadResetSet<core::PropertiesWtc640::GskTable>(transaction, propertyid); } },

    { typeid(core::LoginRole::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::LoginRole::Item>(transaction, propertyid); } },
    { typeid(core::Plugin::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Plugin::Item>(transaction, propertyid); } },
    { typeid(core::FirmwareType::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::FirmwareType::Item>(transaction, propertyid); } },
    { typeid(core::ImageGenerator::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::ImageGenerator::Item>(transaction, propertyid); } },
    { typeid(core::VideoFormat::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::VideoFormat::Item>(transaction, propertyid); } },
    { typeid(core::Framerate::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Framerate::Item>(transaction, propertyid); } },
    { typeid(core::Sensor::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Sensor::Item>(transaction, propertyid); } },
    { typeid(core::Core::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Core::Item>(transaction, propertyid); } },
    { typeid(core::DetectorSensitivity::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::DetectorSensitivity::Item>(transaction, propertyid); } },
    { typeid(core::Focus::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Focus::Item>(transaction, propertyid); } },
    { typeid(core::ShutterUpdateMode::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::ShutterUpdateMode::Item>(transaction, propertyid); } },
    { typeid(core::InternalShutterState::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::InternalShutterState::Item>(transaction, propertyid); } },
    { typeid(core::ReticleMode::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::ReticleMode::Item>(transaction, propertyid); } },
    { typeid(core::TimeDomainAveraging::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::TimeDomainAveraging::Item>(transaction, propertyid); } },
    { typeid(core::ImageEqualizationType::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::ImageEqualizationType::Item>(transaction, propertyid); } },
    { typeid(core::MotorFocusMode::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::MotorFocusMode::Item>(transaction, propertyid); } },
    { typeid(core::SensorCint::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::SensorCint::Item>(transaction, propertyid); } },
    { typeid(core::Baudrate::Item), [](auto& transaction, auto propertyid){ testReadResetSet<core::Baudrate::Item>(transaction, propertyid); } },
};

void testAllProperties(auto* fixture)
{
    auto& properties = fixture->properties;
    BOOST_REQUIRE(properties);

    auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(false).getConnectionExclusiveTransaction();
    auto& propertiesTransaction = exclusiveTransaction.getPropertiesTransaction();

    const auto allProperties = propertiesTransaction.getAllProperyIds();

    for (const auto& propertyId : allProperties)
    {
        BOOST_TEST_MESSAGE("Testing property: " + propertyId.getIdString());

        const bool readable = propertiesTransaction.isPropertyReadable(propertyId);
        const bool writable = propertiesTransaction.isPropertyWritable(propertyId);

        if (!readable && !writable)
        {
            BOOST_TEST_MESSAGE("  Skipping - property is neither readable nor writable.");
            continue;
        }

        const auto& typeInfo = propertiesTransaction.getPropertyTypeInfo(propertyId);
        auto it = DISPATCH_TABLE.find(typeInfo);

        if (it == DISPATCH_TABLE.end())
        {
            BOOST_TEST_MESSAGE("  Unsupported type: " + std::string(typeInfo.name()) + ". Skipping.");
            continue;
        }

        BOOST_TEST_MESSAGE("  Executing typed test for: " + std::string(typeInfo.name()));
        it->second(exclusiveTransaction, propertyId);
    }
}

template<core::Plugin::Item PluginType, typename ConnectionPolicy>
void runCommonIntegrationTest(PropertiesFixture<PluginType, ConnectionPolicy>* fixture, bool streamRequired = true)
{
    auto& properties = fixture->properties;
    BOOST_REQUIRE(properties);

    std::shared_ptr<core::IStream> stream;
    bool streamStarted = false;
    {
        auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(false);

        auto reportStreamIssue = [&](const std::string& message)
        {
            if (streamRequired)
            {
                BOOST_REQUIRE_MESSAGE(false, message);
            }
            else
            {
                BOOST_WARN_MESSAGE(false, message);
            }
        };

        auto streamResult = properties->getOrCreateStream(exclusiveTransaction.getConnectionExclusiveTransaction());
        if (!streamResult.isOk())
        {
            reportStreamIssue(utils::format("Failed to get stream: {}", streamResult.toString()));
        }
        else
        {
            stream = streamResult.getValue();

            auto getStreamResult = properties->getStream(exclusiveTransaction.getConnectionExclusiveTransaction());
            if (!getStreamResult.isOk())
            {
                reportStreamIssue(utils::format("Failed to get existing stream: {}", getStreamResult.toString()));
            }

            auto startStreamResult = stream->startStream(core::ImageData::Type::RGB);
            if (!startStreamResult.isOk())
            {
                reportStreamIssue(utils::format("Failed to start stream: {}", startStreamResult.toString()));
            }
            else
            {
                streamStarted = true;

                BOOST_TEST_MESSAGE("Capturing 10 frames...");
                auto captureResult = exclusiveTransaction.captureImages(10, core::ProgressController());
                BOOST_CHECK_MESSAGE(captureResult.isOk(), utils::format("Failed to capture images: {}", captureResult.toString()));
            }
        }
    }

    {
        auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(true);
        BOOST_TEST_MESSAGE("Activating NUC_OFFSET_UPDATE trigger...");
        auto triggerResult = exclusiveTransaction.activateCommonTriggerAndWaitTillFinished(core::CommonTrigger::Item::NUC_OFFSET_UPDATE);
        BOOST_CHECK_MESSAGE(triggerResult.isOk(), utils::format("Failed to run NUC_OFFSET_UPDATE trigger: {}", triggerResult.toString()));
    }

    {
        BOOST_TEST_MESSAGE("Resetting core...");
        auto resetResult = properties->resetCore(core::ProgressController());
        BOOST_CHECK_MESSAGE(resetResult.isOk(), utils::format("Failed to reset core: {}", resetResult.toString()));
    }

    if (stream && streamStarted && stream->isRunning())
    {
        auto result = stream->stopStream();
        BOOST_CHECK_MESSAGE(result.isOk(), utils::format("Failed to stop stream: {}", result.toString()));
    }
    testAllProperties(fixture);
}

template<core::Plugin::Item PluginType, typename ConnectionPolicy>
void runResetReconnectStabilityTest(PropertiesFixture<PluginType, ConnectionPolicy>* fixture, unsigned iterations)
{
    auto& properties = fixture->properties;
    BOOST_REQUIRE(properties);
    BOOST_REQUIRE(iterations > 0);

    for (unsigned i = 0; i < iterations; ++i)
    {
        BOOST_TEST_MESSAGE(utils::format("Reset/reconnect stability iteration {}/{}", i + 1, iterations));

        {
            auto resetResult = properties->resetCore(core::ProgressController());
            BOOST_REQUIRE_MESSAGE(resetResult.isOk(), utils::format("Reset failed on iteration {}: {}", i + 1, resetResult.toString()));
        }

        auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(false);
        auto& transaction = exclusiveTransaction.getConnectionExclusiveTransaction().getPropertiesTransaction();

        auto statusResult = transaction.getValue<core::StatusWtc640>(core::PropertyIdWtc640::STATUS);
        BOOST_REQUIRE_MESSAGE(statusResult.containsValue(),
                              utils::format("Reading STATUS failed on iteration {}: {}", i + 1, statusResult.getResult().toString()));

        const auto deviceType = statusResult.getValue().getDeviceType();
        BOOST_REQUIRE_MESSAGE(deviceType.has_value(), utils::format("STATUS has no device type on iteration {}", i + 1));
        BOOST_CHECK_MESSAGE(deviceType.value() == core::DevicesWtc640::MAIN_USER,
                            utils::format("Unexpected device type after reset on iteration {}: {}", i + 1, static_cast<int>(deviceType.value().getInternalId())));

        auto triggerResult = exclusiveTransaction.activateCommonTriggerAndWaitTillFinished(core::CommonTrigger::Item::NUC_OFFSET_UPDATE);
        BOOST_CHECK_MESSAGE(triggerResult.isOk(), utils::format("NUC trigger failed on iteration {}: {}", i + 1, triggerResult.toString()));
    }
}
