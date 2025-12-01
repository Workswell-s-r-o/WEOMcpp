#ifndef CORE_CONNECTION_IDATALINKINTERFACE_H
#define CORE_CONNECTION_IDATALINKINTERFACE_H

#include "core/misc/result.h"

#include <chrono>
#include <span>


namespace core
{

namespace connection
{

//!
//! @class IDataLinkInterface
//! @brief interface pro objekty reprezentujici nejnizsi komunikacni vrstvu
//!
class IDataLinkInterface
{
public:
    virtual ~IDataLinkInterface() {}

    virtual bool isOpened() const = 0;
    virtual void closeConnection() = 0;

    virtual size_t getMaxDataSize() const = 0;

    [[nodiscard]] virtual VoidResult read(std::span<uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) = 0;
    [[nodiscard]] virtual VoidResult write(std::span<const uint8_t> buffer, const std::chrono::steady_clock::duration& timeout) = 0;

    virtual void dropPendingData() = 0;

    virtual bool isConnectionLost() const = 0;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_IDATALINKINTERFACE_H
