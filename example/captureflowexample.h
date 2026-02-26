#ifndef CAPTUREFLOWEXAMPLE_H
#define CAPTUREFLOWEXAMPLE_H

#include "core/wtc640/propertieswtc640.h"
#include "core/misc/imainthreadindicator.h"
#include "core/misc/progresscontroller.h"

#include <cstdint>
#include <atomic>
#include <memory>
#include <thread>

class CaptureFlowMainThreadIndicator : public core::IMainThreadIndicator
{
public:
    CaptureFlowMainThreadIndicator();
    [[nodiscard]] bool isInGuiThread() override;

private:
    std::thread::id m_mainThreadId;
};

class CaptureFlowExample
{
public:
    CaptureFlowExample();
    bool run(const core::connection::SerialPortInfo& portInfo);
    void requestStop();
    bool isStopRequested() const;

private:
    core::VoidResult connect(const core::connection::SerialPortInfo& portInfo);
    core::VoidResult captureImages(uint8_t imagesCount);
    core::VoidResult runResetTrigger();
    core::VoidResult addDummyDeadPixelAndClean(size_t iteration);
    core::VoidResult loopPresetsWithDummyDeadPixel();

    std::shared_ptr<CaptureFlowMainThreadIndicator> m_mainThreadIndicator;
    std::shared_ptr<core::PropertiesWtc640> m_properties;
    std::shared_ptr<core::ProgressNotifier> m_progressNotifier;
    core::ProgressController m_progressController;
    std::atomic<bool> m_stopRequested {false};
};

#endif // CAPTUREFLOWEXAMPLE_H
