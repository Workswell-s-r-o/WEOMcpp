#if defined(__APPLE__)
// MacOS system libs do not support baud rate listed below. We force it manually.
#define B921600 921600
#endif

#include "core/connection/datalinkuart.h"

#include "core/connection/deviceutils.h"
#include "core/logging.h"
#include "core/utils.h"

#include <boost/system/windows_error.hpp>
#include <boost/exception/diagnostic_information.hpp>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <memory>

namespace core
{

namespace connection
{
const std::string SETTINGS_ACTION = "Connection settings";

const char* const FFMPEG_GRAY16LE_PIXEL_FORMAT = "gray16le";
const char* const FFMPEG_YUYV422_PIXEL_FORMAT = "yuyv422";

const unsigned CLOSE_SERIAL_PORT_MIN_TIMEOUT = 500;

DataLinkUart::DataLinkUart(const SerialPortInfo& portInfo)
    : AsioDataLinkWithBaudrateAndStreamSource(1)
    , m_portInfo(portInfo)
{
}

DataLinkUart::~DataLinkUart()
{
    closeConnectionImpl();
}

ValueResult<std::shared_ptr<DataLinkUart>> DataLinkUart::createConnection(const SerialPortInfo& portInfo, Baudrate::Item baudrate)
{
    using ResultType = ValueResult<std::shared_ptr<DataLinkUart>>;

    auto connection = std::shared_ptr<DataLinkUart>(new DataLinkUart(portInfo));
    connection->m_serialPort = std::make_unique<boost::asio::serial_port>(connection->m_ioContext);

    boost::system::error_code errorCode;
    connection->m_serialPort->open(portInfo.systemLocation, errorCode);

    if (errorCode.failed())
    {
        connection->m_serialPort.reset();
        connection->m_ioContext.reset();

        return ResultType::createFromError(connection->createBoostAsioError("Connection", errorCode));
    }

    TRY_RESULT(connection->setBaudrate(baudrate));

    connection->m_serialPort->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::type::none), errorCode);
    if (errorCode.failed())
    {
        return ResultType::createFromError(connection->createBoostAsioError(SETTINGS_ACTION, errorCode));
    }

    connection->m_serialPort->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::type::none), errorCode);
    if (errorCode.failed())
    {
        return ResultType::createFromError(connection->createBoostAsioError(SETTINGS_ACTION, errorCode));
    }

    connection->m_serialPort->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::type::one), errorCode);
    if (errorCode.failed())
    {
        return ResultType::createFromError(connection->createBoostAsioError(SETTINGS_ACTION, errorCode));
    }

    connection->m_serialPort->set_option(boost::asio::serial_port_base::character_size(8), errorCode);
    if (errorCode.failed())
    {
        return ResultType::createFromError(connection->createBoostAsioError(SETTINGS_ACTION, errorCode));
    }

    return connection;
}

ValueResult<std::shared_ptr<IStream>> DataLinkUart::createNewStream()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    const auto devNameWithFormat = findVideoDeviceNameWithFormat(m_portInfo.serialNumber);
    TRY_GET_RESULT(const auto result, UartStream::createStream(devNameWithFormat.first, devNameWithFormat.second));
    return ResultType(result);
}

DataLinkUart::UartStream::UartStream(const std::string& deviceName, const std::string inputFormat)
    : m_deviceName(deviceName)
    , m_inputFormat(inputFormat)
{
}

DataLinkUart::UartStream::~UartStream()
{
    if(auto result = stopStream(); !result.isOk())
    {
        WW_LOG_CONNECTION_WARNING << "Error on stream stopping: " << result;
    }
}

VoidResult DataLinkUart::UartStream::startStream(ImageData::Type dataType)
{
    std::lock_guard<std::mutex> lock(m_streamMutex);
    static constexpr std::string_view OPEN_INPUT_ERROR = "Failed to open input device!";
    static constexpr std::string_view FIND_STREAM_INFO_ERROR = "Failed to retrieve input stream info!";

    m_dataType = dataType;
    const char* pixelFormat = nullptr;
    switch(m_dataType)
    {
        case ImageData::Type::Raw14Bit:
            pixelFormat = FFMPEG_GRAY16LE_PIXEL_FORMAT;
            break;

        case ImageData::Type::YUYV422:
        case ImageData::Type::RGB:
            pixelFormat =  FFMPEG_YUYV422_PIXEL_FORMAT;
            break;

        default:
            assert(false && "Unknown video format!");
            return VoidResult::createError("Unknown video format!");
    }

    avdevice_register_all();

    AVDictionary *avDictionaryOptions = nullptr;
    std::string bufferSize = std::to_string(WIDTH_INPUT_STREAM * HEIGHT_INPUT_STREAM * sizeof(uint8_t) * 2 * 2);
    av_log_set_level(AV_LOG_QUIET);
    av_dict_set(&avDictionaryOptions, "rtbufsize", bufferSize.c_str(), 0); // buffer size is 2 frames
    av_dict_set(&avDictionaryOptions, "pixel_format", pixelFormat, 0);
    av_dict_set(&avDictionaryOptions, "framerate", "60", 0);
    av_dict_set(&avDictionaryOptions, "thread_queue_size", "128", 0);

    auto* inputFormat = av_find_input_format(m_inputFormat.c_str());
    if (const auto result = avformat_open_input(&m_inputContext, m_deviceName.c_str(), inputFormat, &avDictionaryOptions); result < 0)
    {
        return VoidResult::createError(OPEN_INPUT_ERROR.data(), utils::format("avformat_open_input return is {}", result));
    }

    if (const auto result = avformat_find_stream_info(m_inputContext, nullptr); result < 0)
    {
        return VoidResult::createError(FIND_STREAM_INFO_ERROR.data(), utils::format("avformat_find_stream_info return is {}", result));
    }
    if (m_dataType == ImageData::Type::RGB)
    {
        const auto codec = avcodec_find_decoder(m_inputContext->streams[0]->codecpar->codec_id);
        m_codecContext = avcodec_alloc_context3(codec);
        if(const auto result = avcodec_parameters_to_context(m_codecContext, m_inputContext->streams[0]->codecpar); result != 0)
        {
            return VoidResult::createError("Failed to open codec", utils::format("avcodec_parameters_to_context return is {}", result));
        }
        if(const auto result = avcodec_open2(m_codecContext, codec, NULL); result != 0)
        {
            return VoidResult::createError("Failed to open codec", utils::format("avcodec_open2 return is {}", result));
        }
    }
    return VoidResult::createOk();
}

VoidResult DataLinkUart::UartStream::stopStream()
{
    std::lock_guard<std::mutex> lock(m_streamMutex);

    avformat_close_input(&m_inputContext);
    avcodec_free_context(&m_codecContext);
    m_codecContext = nullptr;
    m_convertContext = nullptr;

    return VoidResult::createOk();
}

bool DataLinkUart::UartStream::isRunning() const
{
    return m_inputContext;
}

VoidResult DataLinkUart::UartStream::readImageData(ImageData& imageData)
{
    imageData.type = m_dataType;
    switch(m_dataType)
    {
    case ImageData::Type::Raw14Bit:
    case ImageData::Type::YUYV422:
    {
        auto result = getFrameData(imageData.data);
        if(!result.isOk())
        {
            return result;
        }
        break;
    }

    case ImageData::Type::RGB:
    {
        auto result = getRgbFrameData(imageData.data);
        if(!result.isOk())
        {
            return result;
        }
        break;
    }

    default:
    {
        assert(false && "Unknown video format!");
        return VoidResult::createError("Unknown video format!");
    }
    }

    if (imageData.data.empty())
    {
        return VoidResult::createError("Image data is empty!");
    }
    return VoidResult::createOk();
}

ValueResult<std::shared_ptr<DataLinkUart::UartStream>> DataLinkUart::UartStream::createStream(const std::string& deviceName, const std::string inputFormat)
{
    using ReturnType = ValueResult<std::shared_ptr<DataLinkUart::UartStream>>;

    return std::make_shared<DataLinkUart::UartStream>(deviceName, inputFormat);
}

namespace
{
template<typename T>
class RaiiObject final
{
public:
    using DtorType = void (*)(T*);

    RaiiObject(DtorType dtor) : _dtor(dtor) {}
    RaiiObject(T&& value, DtorType dtor) : _value(std::forward<T>(value)), _dtor(dtor) {}

    ~RaiiObject()
    {
        _dtor(&_value);
    }

    T* ptr()
    {
        return &_value;
    }

    T& get()
    {
        return _value;
    }

    auto operator->()
    {
        if constexpr(std::is_pointer_v<T>)
            return _value;
        else
            return &_value;
    }

private:
    T _value;
    DtorType _dtor;
};
} // namespace

VoidResult DataLinkUart::UartStream::getFrameData(std::vector<uint8_t>& data)
{
    AVPacket packet;
    {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        if (!isRunning())
        {
            return VoidResult::createError("Stream is not running!");
        }

        if (int result = av_read_frame(m_inputContext, &packet); result < 0)
        {
            return VoidResult::createError("Failed frame acquisition!", utils::format("av_read_frame return is {}", result));
        }
    }
    data.resize(packet.size);
    memcpy(data.data(), packet.data, packet.size);
    av_packet_unref(&packet);
    return VoidResult::createOk();
}

VoidResult DataLinkUart::UartStream::getRgbFrameData(std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_streamMutex);
    if (!isRunning())
    {
        return VoidResult::createError("Stream is not running!");
    }

    assert(m_codecContext != nullptr);

    auto frame = RaiiObject{av_frame_alloc(), av_frame_free};
    auto frameRGB =  RaiiObject{av_frame_alloc(), av_frame_free};

    frameRGB->width = m_codecContext->width;
    frameRGB->height = m_codecContext->height;
    frameRGB->format = AV_PIX_FMT_RGB24;

    auto numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frameRGB->width, frameRGB->height, 1);
    data.resize(numBytes);

    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, data.data(), AV_PIX_FMT_RGB24, frameRGB->width, frameRGB->height, 1);

    auto packet = RaiiObject{av_packet_unref};
    if (int result = av_read_frame(m_inputContext, packet.ptr()); result != 0)
    {
        return VoidResult::createError("Failed frame acquisition!", utils::format("av_read_frame return is {}", result));
    }

    if (auto result = avcodec_send_packet(m_codecContext, packet.ptr()); result != 0)
    {
        return VoidResult::createError("Failed frame acquisition!", utils::format("avcodec_send_packet return is {}", result));
    }

    if (auto result = avcodec_receive_frame(m_codecContext, frame.get()); result != 0)
    {
        return VoidResult::createError("Failed frame acquisition!", utils::format("avcodec_receive_frame return is {}", result));
    }

    m_convertContext = sws_getCachedContext(m_convertContext, frameRGB->width, frameRGB->height, m_codecContext->pix_fmt, frameRGB->width, frameRGB->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(m_convertContext, frame->data, frame->linesize, 0, frameRGB->height, frameRGB->data, frameRGB->linesize);

    return  VoidResult::createOk();
}

ValueResult<Baudrate::Item> DataLinkUart::getBaudrate() const
{
    using ResultType = ValueResult<Baudrate::Item>;

    if (!isOpened())
    {
        return ResultType::createFromError(createNotOpenedError(SETTINGS_ACTION));
    }

    boost::system::error_code errorCode;
    boost::asio::serial_port_base::baud_rate currentBaudrate;
    m_serialPort->get_option(currentBaudrate, errorCode);

    if (errorCode.failed())
    {
        return ResultType::createFromError(createBoostAsioError(SETTINGS_ACTION, errorCode));
    }

    for (const auto& baudrate : Baudrate::ALL_ITEMS)
    {
        if (Baudrate::getBaudrateSpeed(baudrate) == currentBaudrate.value())
        {
            return baudrate;
        }
    }

    return ResultType::createError("Invalid connection baudrate", utils::format("baudrate value: {}", currentBaudrate.value()));
}

VoidResult DataLinkUart::setBaudrate(Baudrate::Item baudrate)
{
    if (!isOpened())
    {
        return createNotOpenedError(SETTINGS_ACTION);
    }

    boost::system::error_code errorCode;
    m_serialPort->set_option(boost::asio::serial_port_base::baud_rate(Baudrate::getBaudrateSpeed(baudrate)), errorCode);

    if (errorCode.failed())
    {
        return createBoostAsioError(SETTINGS_ACTION, errorCode);
    }

    return VoidResult::createOk();
}

const SerialPortInfo& DataLinkUart::getPortInfo() const
{
    return m_portInfo;
}

bool DataLinkUart::isOpened() const
{
    return m_serialPort->is_open();
}

size_t DataLinkUart::getMaxDataSize() const
{
    return std::numeric_limits<size_t>::max();
}

void DataLinkUart::closeConnectionImpl()
{
    if (m_serialPort)
    {
        if(auto stream = m_stream.lock())
        {
            if(auto result = stream->stopStream(); !result.isOk())
            {
                WW_LOG_CONNECTION_WARNING << "Error on stream stopping: " << result;
            }
        }

        boost::system::error_code closeError;
        m_serialPort->close(closeError);
        if (closeError.failed())
        {
            WW_LOG_CONNECTION_WARNING << "Error on sereal port closing: " << closeError.message();
        }
        m_ioContext.stop();
        m_serialPort.reset();
    }
}

bool DataLinkUart::readAsync(std::chrono::steady_clock::duration timeout,
                             std::span<uint8_t> buffer,
                             boost::system::error_code& errorCode,
                             size_t& transferedSize)
{
    return doAsync(timeout, buffer, errorCode, transferedSize);
}

bool DataLinkUart::writeAsync(std::chrono::steady_clock::duration timeout,
                              std::span<const uint8_t> buffer,
                              boost::system::error_code& errorCode,
                              size_t& transferedSize)
{
   return doAsync(timeout, buffer, errorCode, transferedSize);
}

namespace
{
template<typename Functor>
void doAsyncImpl(boost::asio::serial_port& serialPort, std::span<uint8_t> buffer, Functor&& functor)
{
    boost::asio::async_read(serialPort, boost::asio::buffer(buffer.data(), buffer.size()), std::forward<Functor>(functor));
}

template<typename Functor>
void doAsyncImpl(boost::asio::serial_port& serialPort, std::span<const uint8_t> buffer, Functor&& functor)
{
    boost::asio::async_write(serialPort, boost::asio::buffer(buffer.data(), buffer.size()), std::forward<Functor>(functor));
}
} // namespace

template<typename T>
bool DataLinkUart::doAsync(std::chrono::steady_clock::duration timeout,
                             std::span<T> buffer,
                             boost::system::error_code& errorCode,
                             size_t& transferedSize)
{
    bool timedOut = false;
    boost::asio::deadline_timer timer(m_ioContext);
    boost::asio::deadline_timer closeTimer(m_ioContext);

    doAsyncImpl(*m_serialPort, buffer,
                [&timer, &closeTimer, &errorCode, &transferedSize](const boost::system::error_code& resultError, size_t result_n)
                {
                    WW_LOG_CONNECTION_DEBUG << "Call async handler";

                    boost::system::error_code cancelError;
                    closeTimer.cancel(cancelError);
                    timer.cancel(cancelError);
                    if (cancelError.failed())
                    {
                        WW_LOG_CONNECTION_WARNING << "Error on timer cancellation: " << cancelError.message();
                    }
                    if (resultError == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    errorCode = resultError;
                    transferedSize = result_n;
                });

    timer.expires_from_now(boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
    timer.async_wait([this, &timedOut](const boost::system::error_code& resultError)
                     {
                         boost::system::error_code cancelError;
                         m_serialPort->cancel(cancelError);
                         if (cancelError.failed())
                         {
                             WW_LOG_CONNECTION_WARNING << "Error on sereal port cancellation: " << cancelError.message();
                         }

                         if (resultError != boost::asio::error::operation_aborted)
                         {
                             timedOut = true;
                             WW_LOG_CONNECTION_DEBUG << "Operation was timeouted";
                         }
                     });

    closeTimer.expires_from_now(boost::posix_time::milliseconds(std::max<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count() * 2, CLOSE_SERIAL_PORT_MIN_TIMEOUT)));
    closeTimer.async_wait([this, &timedOut](const boost::system::error_code& resultError)
                          {
                              if (resultError != boost::asio::error::operation_aborted)
                              {
                                  boost::system::error_code closeError;
                                  m_serialPort->close(closeError);
                                  if (closeError.failed())
                                  {
                                      WW_LOG_CONNECTION_WARNING << "Error on sereal port closing: " << closeError.message();
                                  }
                                  WW_LOG_CONNECTION_DEBUG << "Close serial port";
                              }
                          });

    m_ioContext.restart();
    m_ioContext.run();

    return timedOut;
}

bool core::connection::DataLinkUart::isConnectionLostIndicator(boost::system::error_code errorCode) const
{
    return errorCode == make_error_code(boost::asio::error::no_permission) // for windows
#ifdef BOOST_WINDOWS_API
            || errorCode == make_error_code(boost::system::windows_error::bad_command) // for windows FX3
#endif
            || errorCode == boost::system::error_code(EIO, boost::asio::error::get_system_category()) // for linux
            || errorCode == boost::system::error_code(ENXIO, boost::asio::error::get_system_category()) // for macOS
            || errorCode == make_error_code(boost::asio::error::eof);      // for linux
}

std::pair<std::string, std::string> DataLinkUart::getVideoDeviceNameWithFormat() const
{
    WW_LOG_CONNECTION_FATAL << m_portInfo.systemLocation;
    if (m_portInfo.systemLocation.starts_with("/dev/tty"))
    {
        avdevice_register_all();

        AVDeviceInfoList* device_list = nullptr;
        auto* inputFormat = av_find_input_format("v4l2");
        int devSize = avdevice_list_input_sources(inputFormat, nullptr, nullptr, &device_list);
        for (int i = 0; i < devSize; ++i)
        {
            for(int j = 0; j < device_list[i].nb_devices; ++j)
            {
                if (std::string(device_list[i].devices[j]->device_description).find(m_portInfo.serialNumber) != std::string::npos)
                {
                    return {std::string(device_list[i].devices[j]->device_name), "v4l2"};
                }
            }
        }
    }
    return {utils::format("video=WEOM {}", m_portInfo.serialNumber), "dshow"};
}
} // namespace connection

} // namespace core
