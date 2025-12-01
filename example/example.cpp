#include "example.h"
#include "core/properties/properties.inl"
#include "core/properties/transactionchanges.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/stream/istream.h"
#include "core/misc/progresscontroller.h"
#include "core/logging.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <string>

MainThreadIndicator::MainThreadIndicator() : m_mainThreadId(std::this_thread::get_id())
{
}

bool MainThreadIndicator::isInGuiThread()
{
    return std::this_thread::get_id() == m_mainThreadId;
}

Example::Example()
{
    logging::initLogging();
    logging::setChannelFilter(std::string(logging::CORE_CONNECTION_CHANNEL_NAME), logging::severityLevel::debug);
    logging::setChannelFilter(std::string(logging::CORE_PROPERTIES_CHANNEL_NAME), logging::severityLevel::debug);
    m_mainThreadIndicator = std::make_shared<MainThreadIndicator>();
    m_properties = core::PropertiesWtc640::createInstance(core::Properties::Mode::ASYNC_QUEUED, m_mainThreadIndicator, nullptr);
    m_properties->transactionFinished.connect([](const core::TransactionSummary& summary)
    {
        if(summary.getTransactionChanges().valueChanged(core::PropertyIdWtc640::PALETTE_INDEX_CURRENT))
        {
            WW_LOG_PROPERTIES_INFO << "Current palette index changed!";
        }
    });
}

Example::~Example()
{
    m_keepRunning = false;
    if (m_mainThread.joinable())
    {
        m_mainThread.join();
    }
    if (m_videoThread.joinable())
    {
        m_videoThread.join();
    }
}

void Example::run(const std::string& serialNumber, const std::string& systemLocation)
{
    core::connection::SerialPortInfo portInfo;
    portInfo.serialNumber = serialNumber;
    portInfo.systemLocation = systemLocation;
    std::vector<core::connection::SerialPortInfo> ports;
    ports.push_back(portInfo);

    auto notifier = core::ProgressNotifier::createProgressNotifier();

    auto progressController = notifier->getOrCreateProgressController();
    {
        auto connectionStateTransaction = m_properties->createConnectionStateTransaction();
        auto result = connectionStateTransaction.connectUartAuto(ports, progressController);

        if (!result.isOk())
        {
            WW_LOG_CONNECTION_FATAL << "Failed to connect: " << result.toString();
            return;
        }
    }

    {
        auto exclusiveTransaction = m_properties->createConnectionExclusiveTransactionWtc640(false);
        auto streamResult = m_properties->getOrCreateStream(exclusiveTransaction.getConnectionExclusiveTransaction());
        if (!streamResult.isOk())
        {
            WW_LOG_CONNECTION_FATAL << "Main thread: Failed to get stream: " << streamResult.toString();
            return;
        }
        m_videoStream = streamResult.getValue();
        if(auto startStreamResult = m_videoStream->startStream(core::ImageData::Type::RGB); !startStreamResult.isOk())
        {
            WW_LOG_CONNECTION_FATAL << "Main thread: Failed to start stream: " << startStreamResult.toString();
            return;
        }
    }

    m_videoThread = std::thread(&Example::videoThread, this);
    m_mainThread = std::thread(&Example::mainThread, this);
}

void Example::mainThread()
{
    while (m_keepRunning)
    {
        WW_LOG_PROPERTIES_INFO << "Main thread: Taking exclusive lock to change palette...";
        {
            auto exclusiveTransaction = m_properties->createConnectionExclusiveTransactionWtc640(false);
            auto& transaction = exclusiveTransaction.getConnectionExclusiveTransaction().getPropertiesTransaction();

            auto currentIndexResult = transaction.getValue<unsigned int>(core::PropertyIdWtc640::PALETTE_INDEX_CURRENT);
            if (currentIndexResult.containsValue())
            {
                unsigned int currentIndex = currentIndexResult.getValue();
                WW_LOG_PROPERTIES_INFO << "Main thread: Current palette index is " << currentIndex;

                // Increment and set new palette index
                unsigned int nextIndex = (currentIndex + 1) % core::PropertyIdWtc640::getPalettesCount();
                WW_LOG_PROPERTIES_INFO << "Main thread: Setting palette index to " << nextIndex;
                if(auto result = transaction.setValue<unsigned int>(core::PropertyIdWtc640::PALETTE_INDEX_CURRENT, nextIndex); !result.isOk())
                {
                    WW_LOG_PROPERTIES_FATAL << "Main thread: Failed to change palette!";
                }
            }
            // Temporarily sleep to demonstrate transaction failure in video thread
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        WW_LOG_PROPERTIES_INFO << "Main thread: Released exclusive lock.";
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


void Example::videoThread()
{
    while (m_keepRunning)
    {
        if (auto transaction = m_properties->tryCreatePropertiesTransaction(std::chrono::milliseconds(1)))
        {
            auto temperature = transaction->getValue<double>(core::PropertyIdWtc640::SHUTTER_TEMPERATURE);
            if (temperature.containsValue())
            {
                WW_LOG_PROPERTIES_INFO << "Video thread: (Normal Lock) Shutter temperature = " << temperature.getValue();
            }
        }
        else
        {
            WW_LOG_PROPERTIES_WARNING << "Video thread: Failed to create properties transaction (likely blocked by exclusive lock).";
        }

        if (m_videoStream && m_videoStream->isRunning())
        {
            core::ImageData imageData{core::ImageData::Type::RGB};
            auto readResult = m_videoStream->readImageData(imageData);
            if (!readResult.isOk())
            {
                WW_LOG_CONNECTION_FATAL << "Video thread: Failed to read image data: " << readResult.toString();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <serial_number> <system_location>" << std::endl;
        return 1;
    }
    Example example;
    example.run(argv[1], argv[2]);
    std::cout << "Press ENTER to exit..." << std::endl;
    while(std::cin.get() != '\n')
    {
        continue;
    }
    return 0;
}
