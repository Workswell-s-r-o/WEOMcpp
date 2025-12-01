#ifndef CORE_CONNECTION_TCSIPACKET_H
#define CORE_CONNECTION_TCSIPACKET_H

#include "core/misc/result.h"

#include <vector>
#include <map>
#include <span>
#include <cstdint>


namespace core
{

namespace connection
{

class TCSIPacket
{
public:
    explicit TCSIPacket(const std::vector<uint8_t>& packetData);

    enum class Status : uint8_t
    {
        OK                   = 0x00,
        CAMERA_NOT_READY     = 0x01,
        UNKNOWN_COMMAND      = 0x02,
        WRONG_CHECKSUM       = 0x03,
        WRONG_ADDRESS        = 0x04,
        WRONG_ARGUMENT_COUNT = 0x05,
        FLASH_BURST_ERROR    = 0x06,
        INVALID_SETTINGS     = 0x07,
        INCORRECT_VALUE      = 0x08,
    };

    [[nodiscard]] static TCSIPacket createReadRequest(uint8_t packetId, uint32_t address, uint8_t payloadDataSize);
    [[nodiscard]] static TCSIPacket createWriteRequest(uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData);
    [[nodiscard]] static TCSIPacket createFlashBurstStartRequest(uint8_t packetId, uint32_t address, uint32_t dataSizeInWords);
    [[nodiscard]] static TCSIPacket createFlashBurstEndRequest(uint8_t packetId, uint32_t address);

    [[nodiscard]] static TCSIPacket createOkResponse(uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData);
    [[nodiscard]] static TCSIPacket createErrorResponse(uint8_t packetId, uint32_t address, Status status);

    [[nodiscard]] VoidResult validate() const;
    [[nodiscard]] VoidResult validateAsResponse(uint32_t address) const;
    [[nodiscard]] VoidResult validateAsOkResponse(uint32_t address, uint8_t payloadDataSize) const;
    [[nodiscard]] VoidResult validateAsRequest() const;
    [[nodiscard]] ValueResult<uint8_t> getExpectedDataSize() const;

    uint8_t getPacketId() const;
    std::span<const uint8_t> getPayloadData() const;

    const std::vector<uint8_t>& getPacketData() const;
    std::vector<uint8_t>& getPacketData();

    std::string toString() const;

private:
    std::span<const uint8_t> getPayloadDataImpl() const;

    [[nodiscard]] static const ResultSpecificInfo* getInfo(Status status);
    [[nodiscard]] static VoidResult createResponseError(const std::string& detailMessage, const ResultSpecificInfo* info);

    static constexpr size_t SYNCHRONIZATION_AND_ID_POSITION = 0;
    static constexpr size_t STATUS_OR_COMMAND_POSITION = 1;
    static constexpr size_t ADDRESS_POSITION = 2;
    static constexpr size_t COUNT_POSITION = 6;
    static constexpr size_t DATA_POSITION = 7;

public:
    static constexpr size_t HEADER_SIZE = DATA_POSITION; // 1B sync + 1B status + 4B address + 1B count
    static constexpr size_t MINIMUM_PACKET_SIZE = HEADER_SIZE + 1; // header + 1B checksum + 0B data

private:
    enum class Command : uint8_t
    {
        READ  = 0x80,
        WRITE = 0x81,
        FLASH_BURST_START = 0x82,
        FLASH_BURST_END   = 0x83,
    };

    uint8_t getStatusOrCommand() const;
    uint32_t getAddress() const;

    [[nodiscard]] static TCSIPacket createPacket(uint8_t statusOrCommand, uint8_t packetId, uint32_t address, std::span<const uint8_t> payloadData);
    [[nodiscard]] static uint8_t calculateCheckSum(const std::span<const uint8_t> packetData);

    static std::string valueToHexString(uint8_t value);

    static constexpr uint8_t SYNCHRONIZATION_VALUE = 0xA0;
    static constexpr uint8_t SYNCHRONIZATION_MASK  = 0xF0;
    static constexpr uint8_t PACKET_ID_MASK        = 0x0F;
    static const std::map<Status, std::string> STATUS_TO_STRING;

    std::vector<uint8_t> m_packetData;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_TCSIPACKET_H
