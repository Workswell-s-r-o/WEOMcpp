#ifndef PALETTELOOPEXAMPLE_H
#define PALETTELOOPEXAMPLE_H

#include "core/wtc640/propertieswtc640.h"
#include "core/misc/imainthreadindicator.h"
#include <thread>
#include <memory>

class MainThreadIndicator : public core::IMainThreadIndicator
{
public:
    MainThreadIndicator();
    [[nodiscard]] bool isInGuiThread() override;
private:
    std::thread::id m_mainThreadId;
};

#include <atomic>

class PaletteLoopExample
{
public:
    PaletteLoopExample();
    ~PaletteLoopExample();
    void run(const std::string& serialNumber, const std::string& systemLocation);

private:
    void mainThread();
    void videoThread();

    std::shared_ptr<core::PropertiesWtc640> m_properties;
    std::shared_ptr<MainThreadIndicator> m_mainThreadIndicator;
    std::thread m_mainThread;
    std::thread m_videoThread;
    std::shared_ptr<core::IStream> m_videoStream;
    std::atomic<bool> m_keepRunning {true};
};

#endif // PALETTELOOPEXAMPLE_H
