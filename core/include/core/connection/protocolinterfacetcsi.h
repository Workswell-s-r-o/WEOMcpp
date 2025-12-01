#ifndef CORE_CONNECTION_PROTOCOINTERFACETCSI_H
#define CORE_CONNECTION_PROTOCOINTERFACETCSI_H

#include "core/connection/iprotocolinterface.h"
#include "core/connection/tcsipacket.h"
#include "core/connection/status.h"

#include <memory>

namespace core
{
class ElapsedTimer;

namespace connection
{

class IDataLinkInterface;

class ProtocolInterfaceTCSI : public IProtocolInterface
{
    using BaseClass = IProtocolInterface;

public:
    ProtocolInterfaceTCSI(const std::shared_ptr<Status>& status);

    const std::shared_ptr<IDataLinkInterface>& getDataLinkInterface() const;
    void setDataLinkInterface(const std::shared_ptr<IDataLinkInterface>& dataLinkInterface);

    virtual uint32_t getMaxDataSize() const override;

    [[nodiscard]] virtual VoidResult readData(std::span<uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout) override;
    [[nodiscard]] virtual VoidResult writeData(const std::span<const uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout) override;

    [[nodiscard]] VoidResult writeFlashBurstStart(uint32_t address, uint32_t dataSizeInWords, const std::chrono::steady_clock::duration& timeout);
    [[nodiscard]] VoidResult writeFlashBurstEnd(uint32_t address, const std::chrono::steady_clock::duration& timeout);

    bool isConnectionLost() const;

    const std::shared_ptr<Status>& getStatus() const;

private:
    using PacketValidationMethod = ValueResult<TCSIPacket> (ProtocolInterfaceTCSI::*)(const TCSIPacket& packet, uint32_t address, uint32_t dataSize, const std::string& action);
    [[nodiscard]] ValueResult<TCSIPacket> readDataImpl(uint32_t dataSize, uint32_t address, const std::chrono::steady_clock::duration& timeout);
    [[nodiscard]] VoidResult writeDataImpl(TCSIPacket& packet, uint32_t address, const std::chrono::steady_clock::duration& timeout);

    [[nodiscard]] ValueResult<TCSIPacket> receiveResponse(uint8_t packetId, uint32_t address, uint32_t dataSize, const std::chrono::steady_clock::duration& timeout, const std::string& action);
    [[nodiscard]] ValueResult<TCSIPacket> receiveResponsePacket(const ElapsedTimer& timer, const std::string& action);
    void dropPendingData(const std::chrono::steady_clock::duration& restOfTimeout);

    [[nodiscard]] static ValueResult<TCSIPacket> createResponseError(const std::string& action, const std::string& detailErrorMessage, const ResultSpecificInfo* info);

    static constexpr size_t MAX_STRAIGHT_NO_RESPONSES_COUNT = 2;

    std::shared_ptr<IDataLinkInterface> m_dataLinkInterface;
    std::shared_ptr<Status> m_status;
    uint8_t m_lastPacketId {0};

    size_t m_straightNoResponsesCount {0};
    bool m_connectionLost {false};

    mutable DeadlockDetectionMutex m_mutex;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_PROTOCOINTERFACETCSI_H
