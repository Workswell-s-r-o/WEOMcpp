#ifndef CORE_CONNECTION_IPROTOCOLINTERFACE_H
#define CORE_CONNECTION_IPROTOCOLINTERFACE_H

#include "core/misc/result.h"

#include <chrono>
#include <span>


namespace core
{

namespace connection
{

class IProtocolInterface
{
public:
    virtual ~IProtocolInterface() {}

    virtual uint32_t getMaxDataSize() const = 0;

    [[nodiscard]] virtual VoidResult readData(std::span<uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout) = 0;
    [[nodiscard]] virtual VoidResult writeData(const std::span<const uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout) = 0;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_IPROTOCOLINTERFACE_H
