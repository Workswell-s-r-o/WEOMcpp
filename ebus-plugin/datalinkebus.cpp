#include "datalinkebus.h"

#include "core/connection/resultdeviceinfo.h"
#include "core/misc/elapsedtimer.h"
#include "core/logging.h"
#include "core/utils.h"

#include <PvDeviceAdapter.h>
#include <PvDeviceInfoGEV.h>
#include <PvDeviceInfoU3V.h>
#include <PvSystem.h>
#include <PvDeviceEventSink.h>
#include <PvStream.h>
#include <PvDeviceGEV.h>
#include <PvStreamGEV.h>
#include <PvPipeline.h>
#include <PvConfigurationReader.h>

#include <boost/polymorphic_cast.hpp>

#include <thread>
#include <execution>

namespace core
{
namespace connection
{

class DataLinkEbusEventSink : public PvDeviceEventSink
{
public:
    DataLinkEbusEventSink(std::function<void()> disconnectedHandler)
        : PvDeviceEventSink()
        , m_disconnectedHandler(disconnectedHandler)
    {

    }
    // PvDeviceEventSink interface
    virtual void OnLinkDisconnected(PvDevice* aDevice) override
    {
        m_disconnectedHandler();
    }
private:
    std::function<void()> m_disconnectedHandler;
};

DataLinkEbus::DataLinkEbus() :
    BaseClass()
{
    m_serialPort = std::make_unique<PvDeviceSerialPort>();
    m_eventSink = std::make_unique<DataLinkEbusEventSink>([this]()
                                                          {
                                                              m_connectionLost = true;
                                                          });
}

DataLinkEbus::~DataLinkEbus()
{
    closeConnectionImpl();
}

ValueResult<std::shared_ptr<DataLinkEbus>> DataLinkEbus::createConnection(const EbusDevice& device, Baudrate::Item baudrate, PvDeviceSerial port)
{
    using ResultType = ValueResult<std::shared_ptr<DataLinkEbus>>;

    auto connection = std::shared_ptr<DataLinkEbus>(new DataLinkEbus);

    PvResult pvResult;
    try
    {
        auto* pvDevice = PvDevice::CreateAndConnect(device.connectionID.c_str(), &pvResult);
        if (pvDevice != nullptr)
        {
            connection->m_device = std::shared_ptr<PvDevice>(pvDevice, &PvDevice::Free);
        }
    }
    catch(std::exception e)
    {
        return ResultType::createError("Connection error", utils::format("Exception: {}", e.what()));
    }
    catch(...)
    {
        return ResultType::createError("Connection error", "Unknown exception");
    }

    auto logAndCreateResult = [](const PvResult& pvResult)
    {
        const auto error = createErrorFromResult("Connection", pvResult);
        logError(error);

        return ResultType::createFromError(error);
    };

    if (pvResult.IsFailure())
    {
        return logAndCreateResult(pvResult);
    }

    pvResult = connection->m_device->RegisterEventSink(connection->m_eventSink.get());
    if (pvResult.IsFailure())
    {
        return logAndCreateResult(pvResult);
    }

    connection->m_adapter = std::make_unique<PvDeviceAdapter>(connection->m_device.get());
    pvResult = connection->m_serialPort->Open(connection->m_adapter.get(), port);
    if (pvResult.IsFailure())
    {
        return logAndCreateResult(pvResult);
    }

    const auto baudrateResult = connection->setBaudrate(baudrate);
    if (!baudrateResult.isOk())
    {
        return ResultType::createFromError(baudrateResult);
    }

    return connection;
}

bool DataLinkEbus::isOpened() const
{
    return m_device && m_device->IsConnected();
}

void DataLinkEbus::closeConnection()
{
    closeConnectionImpl();
}

size_t DataLinkEbus::getMaxDataSize() const
{
    return MAXIMUM_MESSAGE_SIZE;
}

VoidResult DataLinkEbus::read(std::span<uint8_t> buffer, const std::chrono::steady_clock::duration& timeout)
{
    if (!isOpened())
    {
        return createNotOpenedError("read");
    }

    std::span<uint8_t> restOfBuffer(buffer);
    assert(!restOfBuffer.empty());

    const ElapsedTimer timer(timeout);
    static constexpr std::chrono::milliseconds delay(10);
    while (!timer.timedOut())
    {
        uint32_t bytesRead = 0;
        const auto pvReadResult = m_serialPort->Read(restOfBuffer.data(), restOfBuffer.size(), bytesRead, std::chrono::duration_cast<std::chrono::milliseconds>(timer.getRestOfTimeout()).count());
        WW_LOG_CONNECTION_DEBUG << utils::format("read: {}B {}ms", bytesRead, timer.getElapsedMilliseconds());
        if (pvReadResult.IsFailure())
        {
            const auto error = createErrorFromResult("Read", pvReadResult);
            logError(error, utils::format("bytes read:{} {}ms", bytesRead, timer.getElapsedMilliseconds()));

            return error;
        }
        restOfBuffer = restOfBuffer.last(restOfBuffer.size() - bytesRead);

        if (restOfBuffer.empty())
        {
            return VoidResult::createOk();
        }

        std::this_thread::sleep_for(delay);
    }

    const auto error = VoidResult::createError("Read error", utils::format("read terminated timedout: {}B {}ms", buffer.size() - restOfBuffer.size(), timer.getElapsedMilliseconds()),
                                               (restOfBuffer.size() == buffer.size())? &INFO_NO_RESPONSE : &INFO_TRANSMISSION_FAILED);
    logError(error);
    return error;
}

VoidResult DataLinkEbus::write(std::span<const uint8_t> buffer, const std::chrono::steady_clock::duration& timeout)
{
    if (!isOpened())
    {
        return createNotOpenedError("write");
    }

    uint32_t bytesWritten;
    const ElapsedTimer timer(timeout);
    const auto pvWriteResult = m_serialPort->Write(buffer.data(), static_cast<uint32_t>(buffer.size()), bytesWritten);
    WW_LOG_CONNECTION_DEBUG << utils::format("written: {}B {}ms", bytesWritten, timer.getElapsedMilliseconds());
    if (pvWriteResult.IsFailure())
    {
        const auto error = createErrorFromResult("Write", pvWriteResult);
        logError(error, utils::format("bytes written:{} {}ms", bytesWritten, timer.getElapsedMilliseconds()));

        return error;
    }

    return VoidResult::createOk();
}

void DataLinkEbus::dropPendingData()
{
    m_serialPort->FlushRxBuffer();
}

bool DataLinkEbus::isConnectionLost() const
{
    return m_connectionLost;
}

ValueResult<Baudrate::Item> DataLinkEbus::getBaudrate() const
{
    if (!m_device || !m_device->IsConnected())
    {
        return ValueResult<Baudrate::Item>::createFromError(createNotOpenedError("Connection settings"));
    }

    PvGenParameterArray* parameters = m_device->GetParameters();
    PvString baud;
    const auto pvResult = parameters->GetEnumValue("BulkBaudRate", baud);
    if (pvResult.IsFailure())
    {
        const auto error = createErrorFromResult("Connection settings", pvResult);
        logError(error);

        return ValueResult<Baudrate::Item>::createFromError(error);
    }

    return ebusNameToBaudrate(baud.GetAscii());
}

VoidResult DataLinkEbus::setBaudrate(Baudrate::Item baudrate)
{
    if (!m_device || !m_device->IsConnected())
    {
        return createNotOpenedError("Connection settings");
    }

    PvGenParameterArray* parameters = m_device->GetParameters();
    const auto pvResult = parameters->SetEnumValue("BulkBaudRate", baudrateToEbusName(baudrate).c_str());
    if (pvResult.IsFailure())
    {
        const auto error = createErrorFromResult("Connection settings", pvResult);
        logError(error);

        return error;
    }

    return VoidResult::createOk();
}

ValueResult<std::shared_ptr<IStream>> DataLinkEbus::getOrCreateStream()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    auto streamResult = getStream();
    if(!streamResult.isOk())
    {
        if (const auto result = EbusStream::createStream(m_device); result.isOk())
        {
            m_ebusStream = result.getValue();
            return ResultType(result.getValue());
        }
        else
        {
            return ResultType::createFromError(result);
        }
    }

    return streamResult;
}

ValueResult<std::shared_ptr<IStream>> DataLinkEbus::getStream()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    auto stream = m_ebusStream.lock();
    if(stream != nullptr)
    {
        return ResultType(stream);
    }
    return ResultType::createError("No stream is present!");
}

const std::string DataLinkEbus::baudrateToEbusName(Baudrate::Item baudrate)
{
    switch (baudrate)
    {
        case Baudrate::Item::B_115200:
            return "Baud115200";

        case Baudrate::Item::B_921600:
            return "Baud921600";

        default:
            assert(false);
            return "";
    }
}

const Baudrate::Item DataLinkEbus::ebusNameToBaudrate(std::string ebusName)
{
    ebusName = ebusName.replace(ebusName.find("Baud"), 2, "");
    switch (std::stoi(ebusName))
    {
        case 115200:
            return Baudrate::Item::B_115200;

        case 921600:
            return Baudrate::Item::B_921600;

        default:
            assert(false);
            return Baudrate::Item::B_115200;
    }
}

VoidResult DataLinkEbus::createErrorFromResult(const std::string& action, const PvResult& result)
{
    return VoidResult::createError(utils::format("{} error", action, utils::format("eBUS error code: {} ({})", result.GetCodeString().GetAscii(), result.GetDescription().GetAscii()),
                                   &INFO_TRANSMISSION_FAILED));
}

VoidResult DataLinkEbus::createNotOpenedError(const std::string& action)
{
    return VoidResult::createError(utils::format("Unable to {} - no connection", action, "eBUS !opened",
                                   &INFO_NO_CONNECTION));
}

void DataLinkEbus::logError(const VoidResult& error, const std::string& additionalInfo)
{
    WW_LOG_CONNECTION_WARNING << utils::format("{} ({}) {}", error.getGeneralErrorMessage(), error.getDetailErrorMessage(), additionalInfo);
}

void DataLinkEbus::closeConnectionImpl()
{
    if(auto instance = m_ebusStream.lock(); instance && instance->isRunning())
    {
        if(auto result = instance->stopStream(); !result.isOk())
        {
            WW_LOG_CONNECTION_FATAL << "Failed to stopStream in DatalinkEbus closeConnectionImpl with error - " << result.toString();
        }
    }

    if (m_serialPort->IsOpened())
    {
        m_serialPort->Close();
    }

    m_adapter.reset();
    if (m_device && m_device->IsConnected())
    {
        m_device->UnregisterEventSink(m_eventSink.get());
        m_device->Disconnect();
    }
}

DataLinkEbus::EbusStream::EbusStream(const std::shared_ptr<PvDevice>& device) :
    m_device(device)
{
}

DataLinkEbus::EbusStream::~EbusStream()
{
}

DataLinkEbus::EbusStream::StreamData::StreamData(const std::shared_ptr<PvStream>& stream, const std::shared_ptr<PvPipeline>& pipeline, ImageData::Type dataType) :
    stream(stream),
    pipeline(pipeline),
    dataType(dataType)
{
}

void DataLinkEbus::EbusStream::setPathToExecutableFolder(const std::string& path)
{
    m_pathToExecutableFolder = PvString(path.c_str());
}

VoidResult DataLinkEbus::EbusStream::startStream(ImageData::Type dataType)
{
    if (m_streamData.has_value() && m_streamData->dataType == dataType)
    {
        return VoidResult::createOk();
    }

    // if a stream is running stop it, so I can load a new configuration
    PvGenParameterArray* deviceParams = m_device->GetParameters();
    deviceParams->ExecuteCommand("AcquisitionStop");
    m_device->StreamDisable();

    m_streamData = std::nullopt;

    PvConfigurationReader reader;
    PvResult loadResult;
    switch (dataType)
    {
        case ImageData::Type::Raw14Bit:
            loadResult = reader.Load(std::string(m_pathToExecutableFolder.GetAscii()).append("//ebus_mono14.pvcfg").c_str());
            break;

        case ImageData::Type::PaletteIndices:
            loadResult = reader.Load(std::string(m_pathToExecutableFolder.GetAscii()).append("//ebus_mono8.pvcfg").c_str());
            break;

        default:
            return VoidResult::createError("Unknown stream data type!", std::to_string(static_cast<int>(dataType)));
    }
    if (loadResult.IsFailure())
    {
        return VoidResult::createError("Stream configuration error!", loadResult.GetCodeString().GetAscii());
    }

    if (!m_device || !m_device->IsConnected())
    {
        auto* deviceGEV = dynamic_cast<PvDeviceGEV*>(m_device.get());
        if (deviceGEV == nullptr)
        {
            return VoidResult::createError("Stream reconnect failed!", "Device is not PvDeviceGEV");
        }

        PvString deviceIP = deviceGEV->GetIPAddress();
        m_device.reset();

        PvSystem system;
        system.Find();
        const PvDeviceInfo* foundInfo = nullptr;
        for (uint32_t i = 0; i < system.GetInterfaceCount() && !foundInfo; ++i)
        {
            const PvInterface* iface = system.GetInterface(i);
            for (uint32_t j = 0; j < iface->GetDeviceCount(); ++j)
            {
                const PvDeviceInfo* info = iface->GetDeviceInfo(j);
                auto* infoGEV = dynamic_cast<const PvDeviceInfoGEV*>(info);
                if (infoGEV && infoGEV->GetIPAddress() == deviceIP)
                {
                    foundInfo = info;
                    break;
                }
            }
        }

        if (foundInfo == nullptr)
        {
            return VoidResult::createError("Stream reconnect failed!", utils::format("Device with IP {} not found", deviceIP.GetAscii()));
        }

        PvResult connectResult;
        PvDevice* newDevice = PvDevice::CreateAndConnect(foundInfo, &connectResult);
        if (newDevice == nullptr || connectResult.IsFailure())
        {
            return VoidResult::createError("Stream reconnect failed!", connectResult.GetCodeString().GetAscii());
        }

        m_device.reset(newDevice, &PvDevice::Free);
    }

    for (int i = 0; i < reader.GetDeviceCount(); ++i)
    {
        const auto restoreResult = reader.Restore(i, m_device.get());
        if (!restoreResult.IsOK())
        {
            return VoidResult::createError("Stream configuration error!", utils::format("{} - {}" ,restoreResult.GetCodeString().GetAscii(), restoreResult.GetDescription().GetAscii()));
        }
    }

    auto* deviceGEV = dynamic_cast<PvDeviceGEV*>(m_device.get());
    assert(deviceGEV != nullptr);

    std::shared_ptr<PvStream> stream;
    {
        PvResult result;
        auto* pvStream = PvStream::CreateAndOpen(deviceGEV->GetIPAddress(), &result);
        if (pvStream == nullptr)
        {
            return VoidResult::createError("Unable to open stream!", result.GetCodeString().GetAscii());
        }

        stream = std::shared_ptr<PvStream>(pvStream, &PvStream::Free);

        if (result.IsFailure())
        {
            return VoidResult::createError("Unable to open stream!", result.GetCodeString().GetAscii());
        }
    }

    PvStreamGEV* pvStreamGEV = boost::polymorphic_downcast<PvStreamGEV*>(stream.get());
    deviceGEV->NegotiatePacketSize();
    deviceGEV->SetStreamDestination(pvStreamGEV->GetLocalIPAddress(), pvStreamGEV->GetLocalPort());

    auto pipeline = std::shared_ptr<PvPipeline>(new PvPipeline(stream.get()));
    pipeline->SetBufferCount(16);
    pipeline->SetBufferSize(m_device->GetPayloadSize());

    PvResult pipelineResult = pipeline->Start();
    if (pipelineResult.IsFailure())
    {
        return VoidResult::createError("Unable to start stream!",
                                       utils::format("pipeline error: {}", pipelineResult.GetCodeString().GetAscii()));
    }

    // start the stream again now that the configuration is applied again
    PvResult enableResult = m_device->StreamEnable();
    if (enableResult.IsFailure())
    {
        return VoidResult::createError("Unable to start stream!", utils::format("stream enable error: {}", enableResult.GetCodeString().GetAscii()));
    }

    auto* startAcquisition = dynamic_cast<PvGenCommand*>(deviceParams->Get("AcquisitionStart"));
    if (!startAcquisition)
    {
        return VoidResult::createError("Unable to start stream!", "AcquisitionStart error");
    }

    PvResult commandResult = startAcquisition->Execute();
    if (commandResult.IsFailure())
    {
        return VoidResult::createError("Unable to start stream!", utils::format("start acquisition error: {} {}", commandResult.GetCodeString().GetAscii(), commandResult.GetDescription().GetAscii()));
    }

    m_streamData.emplace(stream, pipeline, dataType);
    return VoidResult::createOk();
}

VoidResult DataLinkEbus::EbusStream::stopStream()
{
    if (!m_device || !m_device->IsConnected())
    {
        return VoidResult::createOk();
    }

    PvGenParameterArray* deviceParams = m_device->GetParameters();
    PvGenCommand* stopAcquisition = dynamic_cast<PvGenCommand*>(deviceParams->Get("AcquisitionStop"));
    if (stopAcquisition)
    {
        stopAcquisition->Execute();
    }

    m_device->StreamDisable();

    if (m_streamData.has_value())
    {
        auto& pipeline = m_streamData->pipeline;
        if (pipeline)
        {
            pipeline->Stop();
        }
    }

    m_streamData = std::nullopt;
    return VoidResult::createOk();
}

bool DataLinkEbus::EbusStream::isRunning() const
{
    return m_streamData.has_value();
}

VoidResult DataLinkEbus::EbusStream::readImageData(ImageData& imageData)
{
    if (!m_streamData.has_value())
    {
        return VoidResult::createError("Stream is not running!");
    }

    const auto& streamData = m_streamData.value();

    PvResult operationResult;
    PvBuffer* pvBuffer = nullptr;
    const auto bufferResult = streamData.pipeline->RetrieveNextBuffer(&pvBuffer, 1000, &operationResult);
    if (bufferResult.IsFailure())
    {
        return VoidResult::createError("Retrieve buffer error!", bufferResult.GetCodeString().GetAscii());
    }
    if (operationResult.IsFailure())
    {
        return VoidResult::createError("Retrieve buffer error!", operationResult.GetCodeString().GetAscii());
    }
    while(operationResult.IsPending())
    {
        continue;
    }

    imageData.type = streamData.dataType;

    uint8_t* buffer = pvBuffer->GetDataPointer();
    imageData.data.resize(pvBuffer->GetSize());
    std::memcpy(imageData.data.data(), buffer, pvBuffer->GetSize());
    streamData.pipeline->ReleaseBuffer(pvBuffer);

    return VoidResult::createOk();
}

ValueResult<std::shared_ptr<DataLinkEbus::EbusStream>> DataLinkEbus::EbusStream::createStream(const std::shared_ptr<PvDevice>& device)
{
    using ResultType = ValueResult<std::shared_ptr<DataLinkEbus::EbusStream>>;

    auto* deviceGEV = dynamic_cast<PvDeviceGEV*>(device.get());
    if (deviceGEV == nullptr)
    {
        return ResultType::createError("Invalid device!", "device is not PvDeviceGEV");
    }

    return std::shared_ptr<DataLinkEbus::EbusStream>(new EbusStream(device));
}

} // namespace connection
} // namespace core
