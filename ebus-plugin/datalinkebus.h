#ifndef CORE_CONNECTION_DATALINKEBUS_H
#define CORE_CONNECTION_DATALINKEBUS_H

#include "core/connection/iebusplugin.h"

#include <PvDeviceSerialPort.h>

#include <atomic>

class PvDevice;
class PvDeviceAdapter;
class PvDeviceInfo;
class PvResult;
class PvStream;
class PvPipeline;

namespace core
{
namespace connection
{

class DataLinkEbusEventSink;

class DataLinkEbus : public IDataLinkWithBaudrateAndStreamSource
{
    using BaseClass = IDataLinkWithBaudrateAndStreamSource;

private:
    explicit DataLinkEbus();

public:
    virtual ~DataLinkEbus() override;
    DataLinkEbus(const DataLinkEbus&) = delete;
    DataLinkEbus& operator=(const DataLinkEbus&) = delete;

    virtual bool isOpened() const override;
    virtual void closeConnection() override;
    virtual size_t getMaxDataSize() const override;
    [[nodiscard]] virtual VoidResult read(std::span<uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) override;
    [[nodiscard]] virtual VoidResult write(std::span<const uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) override;
    virtual void dropPendingData() override;
    virtual bool isConnectionLost() const override;
    [[nodiscard]] virtual ValueResult<Baudrate::Item> getBaudrate() const override;
    [[nodiscard]] virtual VoidResult setBaudrate(Baudrate::Item baudrate) override;
    [[nodiscard]] virtual ValueResult<std::shared_ptr<IStream>> getOrCreateStream() override;
    [[nodiscard]] virtual ValueResult<std::shared_ptr<IStream>> getStream() override;

    [[nodiscard]] static ValueResult<std::shared_ptr<DataLinkEbus>> createConnection(const EbusDevice& device, Baudrate::Item baudrate, PvDeviceSerial port);

private:
    class EbusStream : public IStream, StreamRequirements<EbusStream>
    {
        explicit EbusStream(const std::shared_ptr<PvDevice>& device);

    public:
        virtual ~EbusStream() override;

        virtual void setPathToExecutableFolder(const std::string& path) override;

        [[nodiscard]] virtual VoidResult startStream(ImageData::Type dataType) override;
        [[nodiscard]] virtual VoidResult stopStream() override;
        [[nodiscard]] virtual bool isRunning() const override;
        [[nodiscard]] virtual VoidResult readImageData(ImageData& imageData) override;

        [[nodiscard]] static ValueResult<std::shared_ptr<EbusStream>> createStream(const std::shared_ptr<PvDevice>& device);
        static constexpr uint16_t WIDTH_INPUT_STREAM = 640;
        static constexpr uint16_t HEIGHT_INPUT_STREAM = 480;

    private:
        std::shared_ptr<PvDevice> m_device;

        struct StreamData
        {
            std::shared_ptr<PvStream> stream;
            std::shared_ptr<PvPipeline> pipeline;
            ImageData::Type dataType;

            explicit StreamData(const std::shared_ptr<PvStream>& stream, const std::shared_ptr<PvPipeline>& pipeline, ImageData::Type dataType);
        };        
        std::optional<StreamData> m_streamData;
        PvString m_pathToExecutableFolder = "";
    };

    void closeConnectionImpl();

    static const std::string baudrateToEbusName(Baudrate::Item baudrate);
    static const Baudrate::Item ebusNameToBaudrate(std::string ebusName);

    static VoidResult createErrorFromResult(const std::string& action, const PvResult& result);
    static VoidResult createNotOpenedError(const std::string& action);

    static void logError(const core::VoidResult& error, const std::string& additionalInfo = std::string());

    static constexpr size_t MAXIMUM_MESSAGE_SIZE = 0x0106; // from WIC SDK

    std::shared_ptr<PvDevice> m_device;
    std::unique_ptr<PvDeviceAdapter> m_adapter;
    std::unique_ptr<PvDeviceSerialPort> m_serialPort;

    std::weak_ptr<EbusStream> m_ebusStream;

    std::atomic<bool> m_connectionLost{ false };

    std::unique_ptr<DataLinkEbusEventSink> m_eventSink;
};

} // namespace connection
} // namespace core

#endif // CORE_CONNECTION_DATALINKEBUS_H
