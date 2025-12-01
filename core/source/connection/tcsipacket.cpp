#include "core/connection/tcsipacket.h"

#include "core/connection/resultdeviceinfo.h"
#include "core/connection/addressrange.h"
#include "core/utils.h"

#include <boost/endian/conversion.hpp>
#include <array>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace core
{

namespace connection
{

const std::map<TCSIPacket::Status, std::string> TCSIPacket::STATUS_TO_STRING
{
    {Status::OK, "OK"},
    {Status::CAMERA_NOT_READY, "CAMERA NOT READY"},
    {Status::UNKNOWN_COMMAND, "UNKNOWN COMMAND"},
    {Status::WRONG_CHECKSUM, "WRONG CHECKSUM"},
    {Status::WRONG_ADDRESS, "WRONG ADDRESS"},
    {Status::WRONG_ARGUMENT_COUNT, "WRONG ARGUMENT COUNT"},
    {Status::FLASH_BURST_ERROR, "FLASH BURST ERROR"},
    {Status::INVALID_SETTINGS, "INVALID SETTINGS"},
    {Status::INCORRECT_VALUE, "INCORRECT VALUE"},
};

TCSIPacket::TCSIPacket(const std::vector<uint8_t>& packetData) :
    m_packetData(packetData)
{
}

TCSIPacket TCSIPacket::createReadRequest(uint8_t packetId, uint32_t address, uint8_t payloadDataSize)
{
    const auto request = createPacket(static_cast<uint8_t>(Command::READ), packetId, address, std::array<uint8_t, 1>{payloadDataSize});
    assert(request.validateAsRequest().isOk());
    return request;
}

TCSIPacket TCSIPacket::createWriteRequest(uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData)
{
    const auto request = createPacket(static_cast<uint8_t>(Command::WRITE), packetId, address, payloadData);
    assert(request.validateAsRequest().isOk());
    return request;
}

TCSIPacket TCSIPacket::createFlashBurstStartRequest(uint8_t packetId, uint32_t address, uint32_t dataSizeInWords)
{
    std::vector<uint8_t> burstCountData(sizeof(dataSizeInWords), 0);
    *reinterpret_cast<uint32_t*>(burstCountData.data()) = boost::endian::native_to_big(dataSizeInWords);

    const auto request = createPacket(static_cast<uint8_t>(Command::FLASH_BURST_START), packetId, address, burstCountData);
    assert(request.validateAsRequest().isOk());
    return request;
}

TCSIPacket TCSIPacket::createFlashBurstEndRequest(uint8_t packetId, uint32_t address)
{
    const auto request = createPacket(static_cast<uint8_t>(Command::FLASH_BURST_END), packetId, address, std::span<const uint8_t>{});
    assert(request.validateAsRequest().isOk());
    return request;
}

TCSIPacket TCSIPacket::createOkResponse(uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData)
{
    const auto response = createPacket(static_cast<uint8_t>(Status::OK), packetId, address, payloadData);
    assert(response.validateAsOkResponse(address, payloadData.size()).isOk());
    return response;
}

TCSIPacket TCSIPacket::createErrorResponse(uint8_t packetId, uint32_t address, Status status)
{
    const auto response = createPacket(static_cast<uint8_t>(status), packetId, address, std::span<const uint8_t>{});
    assert(response.validateAsOkResponse(address, 0).isOk() == (status == Status::OK));
    return response;
}

TCSIPacket TCSIPacket::createPacket(uint8_t statusOrCommand, uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData)
{
    std::vector<uint8_t> packetData(MINIMUM_PACKET_SIZE + payloadData.size(), 0);

    packetData.at(SYNCHRONIZATION_AND_ID_POSITION) = (SYNCHRONIZATION_MASK & SYNCHRONIZATION_VALUE) | (PACKET_ID_MASK & packetId);
    packetData.at(STATUS_OR_COMMAND_POSITION) = statusOrCommand;

    *reinterpret_cast<uint32_t*>(packetData.data() + ADDRESS_POSITION) = boost::endian::native_to_little(address);

    packetData.at(COUNT_POSITION) = payloadData.size();
    std::copy(payloadData.begin(), payloadData.end(), packetData.begin() + DATA_POSITION);

    packetData.back() = calculateCheckSum(packetData);

    const TCSIPacket packet(packetData);
    assert(packet.validate().isOk());
    assert(packet.getStatusOrCommand() == statusOrCommand);
    assert(packet.getAddress() == address);
    assert(equal(packet.getPayloadDataImpl().begin(), packet.getPayloadDataImpl().end(), payloadData.begin()));

    return packet;
}

uint8_t TCSIPacket::calculateCheckSum(const std::span<const uint8_t> packetData)
{
    assert(!packetData.empty());

    return std::accumulate(packetData.begin(), packetData.end() - 1, 0U);
}

VoidResult TCSIPacket::validate() const
{
    auto createError = [](const std::string& detailMessage)
    {
        return VoidResult::createError("Invalid packet!", detailMessage.c_str(),
                                       &INFO_TRANSMISSION_FAILED);
    };

    if (m_packetData.size() < MINIMUM_PACKET_SIZE)
    {
        return createError(utils::format("invalid size: {}", m_packetData.size()));
    }

    if ((m_packetData.at(SYNCHRONIZATION_AND_ID_POSITION) & SYNCHRONIZATION_MASK) != (SYNCHRONIZATION_VALUE & SYNCHRONIZATION_MASK))
    {
        return createError(utils::format("invalid synchronization value: {} expected: {}", valueToHexString(m_packetData.at(SYNCHRONIZATION_AND_ID_POSITION) & SYNCHRONIZATION_MASK), valueToHexString(SYNCHRONIZATION_VALUE & SYNCHRONIZATION_MASK)));
    }

    if (getStatusOrCommand() != static_cast<uint8_t>(Command::READ) && getStatusOrCommand() != static_cast<uint8_t>(Command::WRITE) &&
        getStatusOrCommand() != static_cast<uint8_t>(Command::FLASH_BURST_START) && getStatusOrCommand() != static_cast<uint8_t>(Command::FLASH_BURST_END) &&
        STATUS_TO_STRING.find(static_cast<Status>(getStatusOrCommand())) == STATUS_TO_STRING.end())
    {
        return createError(utils::format("invalid command/status: {}", valueToHexString(getStatusOrCommand())));
    }

    if (m_packetData.at(COUNT_POSITION) != getPayloadDataImpl().size())
    {
        return createError(utils::format("invalid count value: {} current data size: {}", m_packetData.at(COUNT_POSITION), getPayloadDataImpl().size()));
    }

    if (const auto calculatedChecksum = calculateCheckSum(m_packetData); m_packetData.back() != calculatedChecksum)
    {
        return createError(utils::format("invalid checksum: {} calculated: {}", std::to_string(m_packetData.back()), std::to_string(calculatedChecksum)));
    }

    return VoidResult::createOk();
}

VoidResult TCSIPacket::validateAsResponse(uint32_t address) const
{
    const auto validationResult = validate();
    if (!validationResult.isOk())
    {
        return createResponseError(validationResult.getDetailErrorMessage(), validationResult.getSpecificInfo());
    }

    auto statusIt = STATUS_TO_STRING.find(static_cast<Status>(getStatusOrCommand()));
    if (statusIt == STATUS_TO_STRING.end())
    {
        return createResponseError(utils::format("invalid TCSI - invalid response status: {} address: {}",
                                               valueToHexString(getStatusOrCommand()),
                                               AddressRange::addressToHexString(getAddress())),
                                   &INFO_TRANSMISSION_FAILED);
    }

    if (getAddress() != address)
    {
        return createResponseError(utils::format("invalid TCSI - response address: {} expected: {}",
                                               AddressRange::addressToHexString(getAddress()),
                                               AddressRange::addressToHexString(address)),
                                   &INFO_TRANSMISSION_FAILED);
    }

    return VoidResult::createOk();
}

VoidResult TCSIPacket::validateAsOkResponse(uint32_t address, uint8_t payloadDataSize) const
{
    const auto validationResult = validateAsResponse(address);
    if (!validationResult.isOk())
    {
        return createResponseError(validationResult.getDetailErrorMessage(), validationResult.getSpecificInfo());
    }

    if (static_cast<Status>(getStatusOrCommand()) != Status::OK)
    {
        return createResponseError(utils::format("TCSI response error code: {} - {} address: {}",
                                               valueToHexString(getStatusOrCommand()),
                                               STATUS_TO_STRING.at(static_cast<Status>(getStatusOrCommand())),
                                               AddressRange::addressToHexString(getAddress())),
                                   getInfo(static_cast<Status>(getStatusOrCommand())));
    }

    if (getPayloadDataImpl().size() != payloadDataSize)
    {
        return createResponseError(utils::format("TCSI response data size: {} expected: {} address: {}",
                                               getPayloadDataImpl().size(),
                                               payloadDataSize,
                                               AddressRange::addressToHexString(getAddress())),
                                   &INFO_TRANSMISSION_FAILED);
    }

    return VoidResult::createOk();
}

const ResultSpecificInfo* TCSIPacket::getInfo(Status status)
{
    switch (status)
    {
        case Status::CAMERA_NOT_READY:
            return &INFO_DEVICE_IS_BUSY;

        case Status::WRONG_ADDRESS:
            return &INFO_ACCESS_DENIED;

        case Status::UNKNOWN_COMMAND:
        case Status::WRONG_CHECKSUM:
        case Status::WRONG_ARGUMENT_COUNT:
        case Status::FLASH_BURST_ERROR:
            return &INFO_TRANSMISSION_FAILED;

        case Status::INVALID_SETTINGS:
            return &INFO_INVALID_SETTINGS;

        case Status::INCORRECT_VALUE:
            return &INFO_INVALID_DATA;

        default:
            assert(false);
            return nullptr;
    }
}

VoidResult TCSIPacket::createResponseError(const std::string& detailMessage, const ResultSpecificInfo* info)
{
    return VoidResult::createError("Response error!", detailMessage.c_str(), info);
}

VoidResult TCSIPacket::validateAsRequest() const
{
    auto createError = [](const std::string& detailMessage)
    {
        return VoidResult::createError("Request error!", detailMessage.c_str());
    };

    const auto validationResult = validate();
    if (!validationResult.isOk())
    {
        return createError(validationResult.getDetailErrorMessage());
    }

    if (getStatusOrCommand() == static_cast<uint8_t>(Command::READ))
    {
        if (getPayloadDataImpl().size() != 1)
        {
            return createError(utils::format("invalid TCSI - invalid read request data size: {} address: {}",
                                           getPayloadDataImpl().size(),
                                           AddressRange::addressToHexString(getAddress())));
        }
    }
    else if (getStatusOrCommand() == static_cast<uint8_t>(Command::WRITE))
    {
        if (getPayloadDataImpl().size() == 0)
        {
            return createError(utils::format("invalid TCSI - invalid write request data size: {} address: {}",
                                           getPayloadDataImpl().size(),
                                           AddressRange::addressToHexString(getAddress())));
        }
    }
    else if (getStatusOrCommand() == static_cast<uint8_t>(Command::FLASH_BURST_START))
    {
        if (getPayloadDataImpl().size() != 4)
        {
            return createError(utils::format("invalid TCSI - invalid flash burst start request data size: {} address: {}",
                                           getPayloadDataImpl().size(),
                                           AddressRange::addressToHexString(getAddress())));
        }
    }
    else if (getStatusOrCommand() == static_cast<uint8_t>(Command::FLASH_BURST_END))
    {
        if (getPayloadDataImpl().size() != 0)
        {
            return createError(utils::format("invalid TCSI - invalid flash burst end request data size: {} address: {}",
                                           getPayloadDataImpl().size(),
                                           AddressRange::addressToHexString(getAddress())));
        }
    }
    else
    {
        return createError(utils::format("invalid TCSI - invalid request command: {} address: {}",
                                       valueToHexString(getStatusOrCommand()),
                                       AddressRange::addressToHexString(getAddress())));
    }

    return VoidResult::createOk();
}

ValueResult<uint8_t> TCSIPacket::getExpectedDataSize() const
{
    auto createError = [](const std::string& detailMessage)
    {
        return ValueResult<uint8_t>::createError("Invalid packet data!", detailMessage.c_str(),
                                                 &INFO_TRANSMISSION_FAILED);
    };

    if (m_packetData.size() < HEADER_SIZE)
    {
        return createError(utils::format("not enough data - size: {}", m_packetData.size()));
    }

    if ((m_packetData.at(SYNCHRONIZATION_AND_ID_POSITION) & SYNCHRONIZATION_MASK) != (SYNCHRONIZATION_VALUE & SYNCHRONIZATION_MASK))
    {
        return createError(utils::format("invalid synchronization value: {} expected: {}",
                                        valueToHexString(m_packetData.at(SYNCHRONIZATION_AND_ID_POSITION) & SYNCHRONIZATION_MASK),
                                        valueToHexString(SYNCHRONIZATION_VALUE & SYNCHRONIZATION_MASK)));
    }

    if (getStatusOrCommand() != static_cast<uint8_t>(Command::READ) && getStatusOrCommand() != static_cast<uint8_t>(Command::WRITE) &&
        getStatusOrCommand() != static_cast<uint8_t>(Command::FLASH_BURST_START) && getStatusOrCommand() != static_cast<uint8_t>(Command::FLASH_BURST_END) &&
        STATUS_TO_STRING.find(static_cast<Status>(getStatusOrCommand())) == STATUS_TO_STRING.end())
    {
        return createError(utils::format("invalid command/status: {}", valueToHexString(getStatusOrCommand())));
    }

    return m_packetData.at(COUNT_POSITION);
}

uint8_t TCSIPacket::getPacketId() const
{
    assert(validate().isOk());

    return m_packetData.at(SYNCHRONIZATION_AND_ID_POSITION) & PACKET_ID_MASK;
}

std::span<const uint8_t> TCSIPacket::getPayloadData() const
{
    assert(validate().isOk());

    return getPayloadDataImpl();
}

const std::vector<uint8_t>& TCSIPacket::getPacketData() const
{
    return m_packetData;
}

std::vector<uint8_t>& TCSIPacket::getPacketData()
{
    return m_packetData;
}

std::string TCSIPacket::toString() const
{
    std::string result;

    for (const auto& value : m_packetData)
    {
        result += valueToHexString(value) += " ";
    }

    return result;
}

std::span<const uint8_t> TCSIPacket::getPayloadDataImpl() const
{
    return std::span<const uint8_t>(m_packetData).subspan(HEADER_SIZE, m_packetData.size() - MINIMUM_PACKET_SIZE);
}

uint8_t TCSIPacket::getStatusOrCommand() const
{
    return m_packetData.at(STATUS_OR_COMMAND_POSITION);
}

uint32_t TCSIPacket::getAddress() const
{
    return boost::endian::native_to_little(*reinterpret_cast<const uint32_t*>(m_packetData.data() + ADDRESS_POSITION));
}

std::string TCSIPacket::valueToHexString(uint8_t value)
{
    return utils::numberToHex(value, true);
}

} // namespace connection

} // namespace core
