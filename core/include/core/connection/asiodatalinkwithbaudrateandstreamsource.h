#pragma once

#include "core/stream/idatalinkwithbaudrateandstreamsource.h"

#include <boost/asio.hpp>

namespace core
{
namespace connection
{
class AsioDataLinkWithBaudrateAndStreamSource : public IDataLinkWithBaudrateAndStreamSource
{
public:
    AsioDataLinkWithBaudrateAndStreamSource(const unsigned serialPortTimeout);

    void closeConnection() final override;
    bool isConnectionLost() const final override;

    [[nodiscard]] VoidResult read(std::span<uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) final override;
    [[nodiscard]] VoidResult write(std::span<const uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) final override;
    void dropPendingData() final override;

    [[nodiscard]] ValueResult<std::shared_ptr<IStream>> getOrCreateStream() final override;
    [[nodiscard]] ValueResult<std::shared_ptr<IStream>> getStream() final override;

protected:
    [[nodiscard]] static VoidResult createNotOpenedError(const std::string& action);
    [[nodiscard]] VoidResult createBoostAsioError(const std::string& action, const boost::system::error_code& errorCode) const;
    [[nodiscard]] static VoidResult createBoostAsioErrorCustom(const std::string& action, const boost::system::error_code& errorCode);
    [[nodiscard]] static VoidResult createTimedOutError(const std::string& action, bool noResponse, const std::chrono::steady_clock::duration& timeout);

private:
    virtual void closeConnectionImpl() = 0;
    virtual bool isConnectionLostIndicator(boost::system::error_code errorCode) const = 0;
    virtual ValueResult<std::shared_ptr<IStream>> createNewStream() = 0;

    virtual bool readAsync(std::chrono::steady_clock::duration timeout,
                           std::span<uint8_t> buffer,
                           boost::system::error_code& errorCode,
                           size_t& transferedSize) = 0;
    virtual bool writeAsync(std::chrono::steady_clock::duration timeout,
                    std::span<const uint8_t> buffer,
                    boost::system::error_code& errorCode,
                    size_t& transferedSize) = 0;


protected:
    boost::asio::io_context m_ioContext;
    const unsigned m_serialPortTimeout;
    std::atomic<bool> m_connectionLost{ false };
    std::weak_ptr<IStream> m_stream;
};
} // namespace connection
} // namespace core
