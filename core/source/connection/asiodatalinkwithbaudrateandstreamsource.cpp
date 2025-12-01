#include "core/connection/asiodatalinkwithbaudrateandstreamsource.h"

#include "core/connection/resultdeviceinfo.h"
#include "core/logging.h"
#include "core/misc/elapsedtimer.h"
#include "core/utils.h"

namespace core
{
namespace connection
{
const std::string WRITE_ACTION = "Write";
const std::string READ_ACTION = "Read";

AsioDataLinkWithBaudrateAndStreamSource::AsioDataLinkWithBaudrateAndStreamSource(const unsigned serialPortTimeout)
    : m_serialPortTimeout(serialPortTimeout)
{}

void AsioDataLinkWithBaudrateAndStreamSource::closeConnection()
{
    closeConnectionImpl();
}

bool AsioDataLinkWithBaudrateAndStreamSource::isConnectionLost() const
{
    return m_connectionLost;
}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::read(std::span<uint8_t> buffer, const std::chrono::steady_clock::duration& timeout)
{
    if (!isOpened())
    {
        return createNotOpenedError(READ_ACTION);
    }

    std::span<uint8_t> restOfBuffer = buffer;

    const ElapsedTimer timer(timeout);
    while (!timer.timedOut() && !restOfBuffer.empty())
    {
        boost::system::error_code errorCode;
        size_t transferedSize = 0;

        const bool timedOut = readAsync(timer.getRestOfTimeout(), restOfBuffer, errorCode, transferedSize);
        WW_LOG_CONNECTION_DEBUG << utils::format("read: {}B {}ms", transferedSize, timer.getElapsedMilliseconds());
        if (timedOut)
        {
            WW_LOG_CONNECTION_WARNING << "read timed out";
        }

        if (errorCode.failed())
        {
            if (isConnectionLostIndicator(errorCode))
            {
                m_connectionLost = true;
            }

            return createBoostAsioError(READ_ACTION, errorCode);
        }

        restOfBuffer = restOfBuffer.last(restOfBuffer.size() - transferedSize);
    }

    if (restOfBuffer.size() > 0)
    {
        WW_LOG_CONNECTION_WARNING << utils::format("timed out read: {}B {}ms", restOfBuffer.size(), timer.getElapsedMilliseconds());
        return createTimedOutError(READ_ACTION, restOfBuffer.size() == buffer.size(), timeout);
    }

    return VoidResult::createOk();
}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::write(std::span<const uint8_t> buffer, const std::chrono::steady_clock::duration& timeout)
{
    if (!isOpened())
    {
        return createNotOpenedError(WRITE_ACTION);
    }

    std::span<const uint8_t> restOfBuffer = buffer;

    const ElapsedTimer timer(timeout);
    while (!timer.timedOut() && !restOfBuffer.empty())
    {
        boost::system::error_code errorCode;
        size_t transferedSize = 0;

        const bool timedOut = writeAsync(timer.getRestOfTimeout(), restOfBuffer, errorCode, transferedSize);
        WW_LOG_CONNECTION_DEBUG << utils::format("written: {}B {}ms", transferedSize, timer.getElapsedMilliseconds());
        if (timedOut)
        {
            WW_LOG_CONNECTION_WARNING << "write timed out";
        }

        if (errorCode.failed())
        {
            if (isConnectionLostIndicator(errorCode))
            {
                m_connectionLost = true;
            }

            return createBoostAsioError(WRITE_ACTION, errorCode);
        }

        restOfBuffer = restOfBuffer.last(restOfBuffer.size() - transferedSize);
    }


    if (restOfBuffer.size() > 0)
    {
        WW_LOG_CONNECTION_WARNING << utils::format("timed out written: {}B {}ms", restOfBuffer.size(), timer.getElapsedMilliseconds());
        return createTimedOutError(WRITE_ACTION, restOfBuffer.size() == buffer.size(), timeout);
    }

    return VoidResult::createOk();
}

void AsioDataLinkWithBaudrateAndStreamSource::dropPendingData()
{
    static std::array<uint8_t, 256> buffer;

    auto dataToString = [](std::span<uint8_t> data)
    {
        std::vector<std::string> list;

        for (const uint8_t value : data)
        {
            list.push_back(utils::format("0x{}", value, std::hex));
        }

        return list.empty() ? "" : utils::format("[{}]", utils::joinStringVector(list, ", "));
    };

    while (true)
    {
        boost::system::error_code errorCode;
        size_t transferedSize = 0;

        readAsync(std::chrono::milliseconds(m_serialPortTimeout), buffer, errorCode, transferedSize);
        WW_LOG_CONNECTION_DEBUG << utils::format("dropped: {}B {}", transferedSize, dataToString(std::span<uint8_t>(buffer.begin(), transferedSize)));

        if (transferedSize == 0)
        {
            return;
        }
    }
}

ValueResult<std::shared_ptr<IStream>> AsioDataLinkWithBaudrateAndStreamSource::getOrCreateStream()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    auto streamResult = getStream();
    if(!streamResult.isOk())
    {
        TRY_GET_RESULT(const auto result, createNewStream());
        m_stream = result;
        return result;
    }
    return streamResult;
}

ValueResult<std::shared_ptr<IStream>> AsioDataLinkWithBaudrateAndStreamSource::getStream()
{
    using ResultType = ValueResult<std::shared_ptr<IStream>>;

    auto stream = m_stream.lock();
    if(stream != nullptr)
    {
        return ResultType(stream);
    }
    return ResultType::createError("No stream is present!");

}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::createNotOpenedError(const std::string& action)
{
    return VoidResult::createError(utils::format("Unable to {} - no connection", action), "uart !opened",
                                   &INFO_NO_CONNECTION);
}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::createBoostAsioError(const std::string& action, const boost::system::error_code& errorCode) const
{
    return VoidResult::createError(utils::format("{} error", action), utils::format("uart Asio({}): {}", errorCode.message(), errorCode.value()),
                                   m_connectionLost ? &INFO_NO_CONNECTION : &INFO_TRANSMISSION_FAILED);
}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::createBoostAsioErrorCustom(const std::string& action, const boost::system::error_code& errorCode)
{
    return VoidResult::createError(action, utils::format("Asio({}): {}", errorCode.message(), errorCode.value()));
}

VoidResult AsioDataLinkWithBaudrateAndStreamSource::createTimedOutError(const std::string& action, bool noResponse, const std::chrono::steady_clock::duration& timeout)
{
    return VoidResult::createError(utils::format("{} error", action), utils::format("uart timed out: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()),
                                   noResponse? &INFO_NO_RESPONSE : &INFO_TRANSMISSION_FAILED);
}
} // namespace connection
} // namespace core
