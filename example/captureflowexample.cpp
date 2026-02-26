#include "captureflowexample.h"
#include "portarghelper.h"

#include "core/wtc640/deadpixels.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/properties/properties.inl"
#include "core/logging.h"

#include <cstdio>
#include <string>
#include <vector>
#include <thread>

CaptureFlowMainThreadIndicator::CaptureFlowMainThreadIndicator() :
    m_mainThreadId(std::this_thread::get_id())
{
}

bool CaptureFlowMainThreadIndicator::isInGuiThread()
{
    return std::this_thread::get_id() == m_mainThreadId;
}

namespace
{

std::string presetIdToString(const core::PropertiesWtc640::PresetId& presetId)
{
    auto enumUserNameOrUnknown = [](const auto& allItems, const auto item)
    {
        const auto it = allItems.find(item);
        if (it != allItems.end())
        {
            return it->second.userName;
        }
        return std::string("UNKNOWN(") + std::to_string(static_cast<int>(item)) + ")";
    };

    return "lens=" + enumUserNameOrUnknown(core::Lens::ALL_ITEMS, presetId.lens) +
           ", lensVariant=" + enumUserNameOrUnknown(core::LensVariant::ALL_ITEMS, presetId.lensVariant) +
           ", version=" + enumUserNameOrUnknown(core::PresetVersion::ALL_ITEMS, presetId.version) +
           ", range=" + enumUserNameOrUnknown(core::Range::ALL_ITEMS, presetId.range);
}

} // namespace

CaptureFlowExample::CaptureFlowExample() :
    m_mainThreadIndicator(std::make_shared<CaptureFlowMainThreadIndicator>()),
    m_properties(core::PropertiesWtc640::createInstance(core::Properties::Mode::ASYNC_QUEUED, m_mainThreadIndicator, nullptr)),
    m_progressNotifier(core::ProgressNotifier::createProgressNotifier()),
    m_progressController(m_progressNotifier->getOrCreateProgressController())
{
}

bool CaptureFlowExample::run(const core::connection::SerialPortInfo& portInfo)
{
    if (isStopRequested())
    {
        WW_LOG_CONNECTION_WARNING << "Capture flow example cancelled before start.";
        return true;
    }

    if (!connect(portInfo).isOk())
    {
        if (isStopRequested())
        {
            WW_LOG_CONNECTION_WARNING << "Capture flow example stopped while connecting.";
            return true;
        }
        return false;
    }

    if (isStopRequested())
    {
        WW_LOG_CONNECTION_WARNING << "Capture flow example cancelled after connect.";
        return true;
    }

    if (!captureImages(2).isOk())
    {
        if (isStopRequested())
        {
            WW_LOG_CONNECTION_WARNING << "Capture flow example stopped during image capture.";
            return true;
        }
        return false;
    }

    if (isStopRequested())
    {
        WW_LOG_CONNECTION_WARNING << "Capture flow example cancelled after capture.";
        return true;
    }

    if (!runResetTrigger().isOk())
    {
        if (isStopRequested())
        {
            WW_LOG_CONNECTION_WARNING << "Capture flow example stopped during reset flow.";
            return true;
        }
        return false;
    }

    if (isStopRequested())
    {
        WW_LOG_CONNECTION_WARNING << "Capture flow example cancelled after reset flow.";
        return true;
    }

    if (!loopPresetsWithDummyDeadPixel().isOk())
    {
        if (isStopRequested())
        {
            WW_LOG_CONNECTION_WARNING << "Capture flow example stopped during preset loop.";
            return true;
        }
        return false;
    }

    if (isStopRequested())
    {
        WW_LOG_CONNECTION_WARNING << "Capture flow example stopped.";
        return true;
    }

    WW_LOG_PROPERTIES_INFO << "Capture flow example finished successfully.";
    return true;
}

void CaptureFlowExample::requestStop()
{
    m_stopRequested.store(true);
    m_progressNotifier->cancelProgress();
}

bool CaptureFlowExample::isStopRequested() const
{
    return m_stopRequested.load();
}

core::VoidResult CaptureFlowExample::connect(const core::connection::SerialPortInfo& portInfo)
{
    auto stateTransaction = m_properties->createConnectionStateTransaction();
    std::vector<core::connection::SerialPortInfo> ports {portInfo};

    const auto result = stateTransaction.connectUartAuto(ports, m_progressController);
    if (!result.isOk())
    {
        WW_LOG_CONNECTION_FATAL << "Connect failed: " << result.toString();
        return result;
    }

    WW_LOG_CONNECTION_INFO << "Connected to port " << portInfo.systemLocation;
    return core::VoidResult::createOk();
}

core::VoidResult CaptureFlowExample::captureImages(uint8_t imagesCount)
{
    auto exclusiveTransaction = m_properties->createConnectionExclusiveTransactionWtc640(false);
    const auto captureResult = exclusiveTransaction.captureImages(imagesCount, m_progressController);
    if (!captureResult.isOk())
    {
        WW_LOG_PROPERTIES_FATAL << "Capture failed: " << captureResult.toString();
        return captureResult.toVoidResult();
    }

    const auto& images = captureResult.getValue();
    WW_LOG_PROPERTIES_INFO << "Captured " << images.size() << " image(s).";
    for (size_t i = 0; i < images.size(); ++i)
    {
        const auto sample = images.at(i).empty() ? 0 : images.at(i).front();
        WW_LOG_PROPERTIES_INFO << "Image #" << (i + 1) << " pixels: " << images.at(i).size() << ", first sample: " << sample;
    }

    return core::VoidResult::createOk();
}

core::VoidResult CaptureFlowExample::runResetTrigger()
{
    auto exclusiveTransaction = m_properties->createConnectionExclusiveTransactionWtc640(false);
    const auto resetResult = exclusiveTransaction.activateResetTriggerAndWaitTillFinished(core::ResetTrigger::Item::SOFTWARE_RESET);
    if (!resetResult.isOk())
    {
        WW_LOG_PROPERTIES_FATAL << "SOFTWARE_RESET trigger failed: " << resetResult.toString();
        return resetResult;
    }

    auto stateTransaction = exclusiveTransaction.openConnectionStateTransaction();
    const auto reconnectResult = stateTransaction.reconnectCoreAfterReset(std::nullopt);
    if (!reconnectResult.isOk())
    {
        WW_LOG_PROPERTIES_FATAL << "Reconnect after SOFTWARE_RESET failed: " << reconnectResult.toString();
        return reconnectResult;
    }

    WW_LOG_PROPERTIES_INFO << "SOFTWARE_RESET trigger completed and reconnect succeeded.";
    return core::VoidResult::createOk();
}

core::VoidResult CaptureFlowExample::addDummyDeadPixelAndClean(size_t iteration)
{
    auto exclusiveTransaction = m_properties->createConnectionExclusiveTransactionWtc640(false);
    auto& transaction = exclusiveTransaction.getConnectionExclusiveTransaction().getPropertiesTransaction();

    const auto deadPixelsResult = transaction.getValue<core::DeadPixels>(core::PropertyIdWtc640::DEAD_PIXELS_CURRENT);
    if (!deadPixelsResult.containsValue())
    {
        return core::VoidResult::createError("Failed to read DEAD_PIXELS_CURRENT.");
    }

    core::DeadPixels deadPixels = deadPixelsResult.getValue();
    const auto resolution = deadPixels.getResolutionInPixels();
    if (!resolution.isValid())
    {
        return core::VoidResult::createError("Dead pixels resolution is invalid.");
    }

    core::PixelCoordinates dummyCoordinates;
    dummyCoordinates.row = static_cast<unsigned>((iteration * 17) % static_cast<size_t>(resolution.height));
    dummyCoordinates.column = static_cast<unsigned>((iteration * 31) % static_cast<size_t>(resolution.width));

    core::DeadPixel dummyPixel(dummyCoordinates);
    if (const auto insertResult = deadPixels.insertPixel(dummyPixel); !insertResult.isOk())
    {
        return insertResult;
    }
    deadPixels.recomputeReplacements();

    if (const auto setResult = transaction.setValue<core::DeadPixels>(core::PropertyIdWtc640::DEAD_PIXELS_CURRENT, deadPixels); !setResult.isOk())
    {
        return setResult;
    }

    WW_LOG_PROPERTIES_INFO << "Added dummy DP row=" << dummyCoordinates.row
                           << ", col=" << dummyCoordinates.column
                           << ". DEAD_PIXELS_CURRENT size is now "
                           << deadPixels.getDeadPixelToReplacementsMap().size();

    if (const auto nucResult = exclusiveTransaction.activateCommonTriggerAndWaitTillFinished(core::CommonTrigger::Item::NUC_OFFSET_UPDATE); !nucResult.isOk())
    {
        return nucResult;
    }

    if (const auto cleanResult = exclusiveTransaction.activateCommonTriggerAndWaitTillFinished(core::CommonTrigger::Item::CLEAN_USER_DP); !cleanResult.isOk())
    {
        return cleanResult;
    }

    const auto cleanedDeadPixelsResult = transaction.getValue<core::DeadPixels>(core::PropertyIdWtc640::DEAD_PIXELS_CURRENT);
    if (!cleanedDeadPixelsResult.containsValue())
    {
        return core::VoidResult::createError("Failed to read DEAD_PIXELS_CURRENT after CLEAN_USER_DP.");
    }

    const auto cleanedCount = cleanedDeadPixelsResult.getValue().getDeadPixelToReplacementsMap().size();
    WW_LOG_PROPERTIES_INFO << "After CLEAN_USER_DP, DEAD_PIXELS_CURRENT size = " << cleanedCount;
    if (cleanedCount == 0)
    {
        WW_LOG_PROPERTIES_INFO << "Dead pixels property is now empty.";
    }
    else
    {
        WW_LOG_PROPERTIES_WARNING << "Dead pixels property is not empty after clean.";
    }

    return core::VoidResult::createOk();
}

core::VoidResult CaptureFlowExample::loopPresetsWithDummyDeadPixel()
{
    const auto transaction = m_properties->createPropertiesTransaction();
    const auto presetsResult = transaction.getValue<std::vector<core::PropertiesWtc640::PresetId>>(core::PropertyIdWtc640::ALL_VALID_LENS_RANGES);
    if (!presetsResult.containsValue())
    {
        return core::VoidResult::createError("Failed to read ALL_VALID_LENS_RANGES.");
    }

    const auto& presets = presetsResult.getValue();
    if (presets.empty())
    {
        return core::VoidResult::createError("No valid lens ranges available.");
    }

    WW_LOG_PROPERTIES_INFO << "Preset loop will iterate over " << presets.size() << " presets.";
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (isStopRequested())
        {
            return core::VoidResult::createError("Stopped by user.");
        }

        const auto& presetId = presets.at(i);
        WW_LOG_PROPERTIES_INFO << "Preset iteration " << (i + 1) << "/" << presets.size() << ": " << presetIdToString(presetId);

        if (const auto deadPixelWorkflowResult = addDummyDeadPixelAndClean(i + 1); !deadPixelWorkflowResult.isOk())
        {
            WW_LOG_PROPERTIES_FATAL << "Dead pixel workflow failed: " << deadPixelWorkflowResult.toString();
            return deadPixelWorkflowResult;
        }

        const auto setPresetResult = m_properties->setLensRangeCurrent(presetId, m_progressController);
        if (!setPresetResult.isOk())
        {
            WW_LOG_PROPERTIES_FATAL << "Failed to set preset " << presetIdToString(presetId) << ": " << setPresetResult.toString();
            return setPresetResult;
        }

        WW_LOG_PROPERTIES_INFO << "Applied preset: " << presetIdToString(presetId);
    }

    return core::VoidResult::createOk();
}

int main(int argc, char* argv[])
{
    logging::initLogging();
    logging::setChannelFilter(std::string(logging::CORE_CONNECTION_CHANNEL_NAME), logging::severityLevel::debug);
    logging::setChannelFilter(std::string(logging::CORE_PROPERTIES_CHANNEL_NAME), logging::severityLevel::debug);

    const auto portInfo = example::getPortFromArgsOrLogUsage(argc, argv);
    if (!portInfo.has_value())
    {
        return 1;
    }

    CaptureFlowExample example;
    bool runResult = false;
    std::thread worker([&]()
    {
        runResult = example.run(portInfo.value());
    });

    WW_LOG_CONNECTION_CRITICAL << "Press ENTER to stop CaptureFlowExample.";
    while (true)
    {
        const int input = std::getchar();
        if (input == '\n' || input == EOF)
        {
            break;
        }
    }

    WW_LOG_CONNECTION_WARNING << "Stop requested by user.";
    example.requestStop();
    worker.join();

    return runResult ? 0 : 2;
}
