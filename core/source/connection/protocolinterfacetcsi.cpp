#include "core/connection/protocolinterfacetcsi.h"

#include "core/connection/addressrange.h"
#include "core/connection/resultdeviceinfo.h"
#include "core/connection/idatalinkinterface.h"
#include "core/utils.h"
#include "core/misc/elapsedtimer.h"
#include "core/logging.h"

#include <algorithm>

namespace core
{

namespace connection
{

ProtocolInterfaceTCSI::ProtocolInterfaceTCSI(const std::shared_ptr<Status>& status) :
    m_status(status)
{
}

const std::shared_ptr<IDataLinkInterface>& ProtocolInterfaceTCSI::getDataLinkInterface() const
{
    return m_dataLinkInterface;
}

void ProtocolInterfaceTCSI::setDataLinkInterface(const std::shared_ptr<IDataLinkInterface>& dataLinkInterface)
{
    std::scoped_lock lock(m_mutex);

    m_dataLinkInterface = dataLinkInterface;

    m_straightNoResponsesCount = 0;
    m_connectionLost = false;
}

uint32_t ProtocolInterfaceTCSI::getMaxDataSize() const
{
    if (!m_dataLinkInterface || m_dataLinkInterface->getMaxDataSize() < TCSIPacket::MINIMUM_PACKET_SIZE)
    {
        return 0;
    }

    const uint32_t maxDatalinkDataSize = m_dataLinkInterface->getMaxDataSize() - TCSIPacket::MINIMUM_PACKET_SIZE;
    const uint32_t maxTcsiDataSize = std::numeric_limits<uint8_t>::max();

    return std::min(maxDatalinkDataSize, maxTcsiDataSize);
}

VoidResult ProtocolInterfaceTCSI::readData(std::span<uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout)
{
    if (!m_dataLinkInterface)
    {
        return VoidResult::createError("Unable to read - no connection!", "no datalink interface", &INFO_NO_CONNECTION);
    }

    if (data.empty())
    {
        assert(false && "trying to read nothing? - weird");
        return VoidResult::createOk();
    }

    const auto responsePacket = readDataImpl(data.size(), address, timeout);
    if (!responsePacket.isOk())
    {
        return responsePacket.toVoidResult();
    }

    assert(responsePacket.getValue().getPayloadData().size() == data.size());
    std::copy(responsePacket.getValue().getPayloadData().begin(), responsePacket.getValue().getPayloadData().end(), data.begin());

    return VoidResult::createOk();
}

VoidResult ProtocolInterfaceTCSI::writeData(const std::span<const uint8_t> data, uint32_t address, const std::chrono::steady_clock::duration& timeout)
{
    if (data.empty())
    {
        assert(false && "trying to write nothing? - weird");
        return VoidResult::createOk();
    }

    if (!m_dataLinkInterface)
    {
        return VoidResult::createError("Unable to write - no connection!", "no datalink interface", &INFO_NO_CONNECTION);
    }

    std::scoped_lock lock(m_mutex);

    auto writeRequest = TCSIPacket::createWriteRequest(++m_lastPacketId, address, data);
    return writeDataImpl(writeRequest, address, timeout);
}

VoidResult ProtocolInterfaceTCSI::writeFlashBurstStart(uint32_t address, uint32_t dataSizeInWords, const std::chrono::steady_clock::duration& timeout)
{
    if (!m_dataLinkInterface)
    {
        return VoidResult::createError("Unable to write - no connection!", "no datalink interface", &INFO_NO_CONNECTION);
    }

    std::scoped_lock lock(m_mutex);

    auto writeRequest = TCSIPacket::createFlashBurstStartRequest(++m_lastPacketId, address, dataSizeInWords);
    return writeDataImpl(writeRequest, address, timeout);
}

VoidResult ProtocolInterfaceTCSI::writeFlashBurstEnd(uint32_t address, const std::chrono::steady_clock::duration& timeout)
{
    if (!m_dataLinkInterface)
    {
        return VoidResult::createError("Unable to write - no connection!", "no datalink interface", &INFO_NO_CONNECTION);
    }

    std::scoped_lock lock(m_mutex);

    auto writeRequest = TCSIPacket::createFlashBurstEndRequest(++m_lastPacketId, address);
    return writeDataImpl(writeRequest, address, timeout);
}

bool ProtocolInterfaceTCSI::isConnectionLost() const
{
    return m_connectionLost;
}

const std::shared_ptr<Status>& ProtocolInterfaceTCSI::getStatus() const
{
    return m_status;
}

ValueResult<TCSIPacket> ProtocolInterfaceTCSI::readDataImpl(uint32_t dataSize, uint32_t address, const std::chrono::steady_clock::duration& timeout)
{
    using ResultType = ValueResult<TCSIPacket>;

    std::scoped_lock lock(m_mutex);

    m_status->incrementOperationsCount();

    auto readRequest = TCSIPacket::createReadRequest(++m_lastPacketId, address, dataSize);
    m_lastPacketId = readRequest.getPacketId();
    WW_LOG_CONNECTION_INFO << "Read sending: " << readRequest.toString();

    const ElapsedTimer timer(timeout);
    const auto readRequestResult = m_dataLinkInterface->write(readRequest.getPacketData(), timeout);
    if (!readRequestResult.isOk())
    {
        m_status->addWriteError(readRequestResult);

        return ResultType::createFromError(readRequestResult);
    }

    return receiveResponse(m_lastPacketId, address, dataSize, timer.getRestOfTimeout(), "Read");
}

VoidResult ProtocolInterfaceTCSI::writeDataImpl(TCSIPacket& writeRequest, uint32_t address, const std::chrono::steady_clock::duration& timeout)
{
    m_status->incrementOperationsCount();

    m_lastPacketId = writeRequest.getPacketId();
    WW_LOG_CONNECTION_INFO << "Write sending: " << writeRequest.toString();

    const ElapsedTimer timer(timeout);
    const auto writeRequestResult = m_dataLinkInterface->write(writeRequest.getPacketData(), timeout);
    if (!writeRequestResult.isOk())
    {
        m_status->addWriteError(writeRequestResult);

        return writeRequestResult;
    }

    return receiveResponse(m_lastPacketId, address, 0, timer.getRestOfTimeout(), "Write").toVoidResult();
}

ValueResult<TCSIPacket> ProtocolInterfaceTCSI::receiveResponse(uint8_t packetId, uint32_t address, uint32_t dataSize, const std::chrono::steady_clock::duration& timeout, const std::string& action)
{
    const ElapsedTimer timer(timeout);
    while (true)
    {
        const auto responsePacketResult = receiveResponsePacket(timer, action);
        if (!responsePacketResult.isOk())
        {
            return responsePacketResult;
        }

        const auto responseValidationResult = responsePacketResult.getValue().validateAsResponse(address);
        if (!responseValidationResult.isOk())
        {
            WW_LOG_CONNECTION_WARNING <<
                utils::format("Invalid response: {} (expected packetId: {} address: {} dataSize: {})",
                            responsePacketResult.getValue().toString(),
                            packetId,
                            AddressRange::addressToHexString(address),
                            dataSize);

            const auto error = createResponseError(action, responseValidationResult.getDetailErrorMessage(), responseValidationResult.getSpecificInfo());

            m_status->addResponseError(error.toVoidResult());

            dropPendingData(timer.getRestOfTimeout());
            return error;
        }

        if (responsePacketResult.getValue().getPacketId() == packetId)
        {
            const auto okValidationResult = responsePacketResult.getValue().validateAsOkResponse(address, dataSize);
            if (okValidationResult.isOk())
            {
                return responsePacketResult.getValue();
            }
            else
            {
                const auto error = createResponseError(action, okValidationResult.getDetailErrorMessage(), okValidationResult.getSpecificInfo());

                m_status->addResponseError(error.toVoidResult());
                return error;
            }
        }

        WW_LOG_CONNECTION_WARNING << utils::format("Response dropped: {} (expected packetId: {})",
                                                 responsePacketResult.getValue().toString(),
                                                 packetId);
    }
}

ValueResult<TCSIPacket> ProtocolInterfaceTCSI::receiveResponsePacket(const ElapsedTimer& timer, const std::string& action)
{
    // try read empty response (ERROR / OK confirmation) or first part of non-empty response
    std::vector<uint8_t> receivedData(TCSIPacket::MINIMUM_PACKET_SIZE, 0);
    const auto readResponseResult = m_dataLinkInterface->read(receivedData, timer.getRestOfTimeout());
    if (!readResponseResult.isOk())
    {
        m_status->addReadError(readResponseResult);

        if (readResponseResult.getSpecificInfo() != nullptr)
        {
            const auto* resultDeviceInfo = dynamic_cast<const ResultDeviceInfo*>(readResponseResult.getSpecificInfo());
            if (resultDeviceInfo != nullptr && resultDeviceInfo->error == ResultDeviceInfo::Error::NO_RESPONSE)
            {
                ++m_straightNoResponsesCount;

                if (m_straightNoResponsesCount > MAX_STRAIGHT_NO_RESPONSES_COUNT)
                {
                    WW_LOG_CONNECTION_WARNING << utils::format("Straight no responses: {}x - connection lost", m_straightNoResponsesCount);

                    m_connectionLost = true;
                }
                else
                {
                    WW_LOG_CONNECTION_WARNING << utils::format("Straight no responses: {}x", m_straightNoResponsesCount);
                }
            }
        }

        dropPendingData(timer.getRestOfTimeout());
        return createResponseError(action, readResponseResult.getDetailErrorMessage(), readResponseResult.getSpecificInfo());
    }
    m_straightNoResponsesCount = 0;

    TCSIPacket responsePacket(receivedData);
    const auto expectedDataSize = responsePacket.getExpectedDataSize();
    if (!expectedDataSize.isOk())
    {
        WW_LOG_CONNECTION_WARNING << utils::format("{} received: {} (expectedDataSize NOK)", action, responsePacket.toString());

        const auto error = createResponseError(action, expectedDataSize.getDetailErrorMessage(), expectedDataSize.getSpecificInfo());

        m_status->addResponseError(error.toVoidResult());

        dropPendingData(timer.getRestOfTimeout());
        return error;
    }
    else if (expectedDataSize.getValue() > 0)
    {
        // try read rest of response
        const auto packetSize = receivedData.size();
        receivedData.resize(packetSize + expectedDataSize.getValue(), 0);

        if (const auto readRestOfResponseResult = m_dataLinkInterface->read(std::span<uint8_t>(receivedData).subspan(packetSize, expectedDataSize.getValue()), timer.getRestOfTimeout()); !readRestOfResponseResult.isOk())
        {
            WW_LOG_CONNECTION_INFO << utils::format("{} received: {}", action, responsePacket.toString());

            const auto error = createResponseError(action, readRestOfResponseResult.getDetailErrorMessage(), readRestOfResponseResult.getSpecificInfo());

            m_status->addReadError(error.toVoidResult());

            dropPendingData(timer.getRestOfTimeout());
            return error;
        }

        responsePacket = TCSIPacket(receivedData);
    }
    WW_LOG_CONNECTION_INFO << utils::format("{} received: {}", action, responsePacket.toString());

    return responsePacket;
}

void ProtocolInterfaceTCSI::dropPendingData(const std::chrono::steady_clock::duration& restOfTimeout)
{
    std::this_thread::sleep_for(restOfTimeout);

    m_dataLinkInterface->dropPendingData();
}

ValueResult<TCSIPacket> ProtocolInterfaceTCSI::createResponseError(const std::string& action, const std::string& detailErrorMessage, const ResultSpecificInfo* info)
{
    return ValueResult<TCSIPacket>::createError(utils::format("{} error!", action), detailErrorMessage, info);
}

} // namespace connection

} // namespace core
