#ifndef CORE_CONNECTION_DEVICEINTERFACEWTC640_H
#define CORE_CONNECTION_DEVICEINTERFACEWTC640_H

#include "core/connection/ideviceinterface.h"
#include "core/wtc640/memoryspacewtc640.h"
#include "core/connection/status.h"

#include <bitset>
#include <shared_mutex>


namespace core
{

namespace connection
{

class ProtocolInterfaceTCSI;

class DeviceInterfaceWtc640 : public IDeviceInterface
{
    using BaseClass = IDeviceInterface;

public:
    explicit DeviceInterfaceWtc640(const std::shared_ptr<ProtocolInterfaceTCSI>& protocolInterface, const std::shared_ptr<Status>& status);

    const std::shared_ptr<ProtocolInterfaceTCSI>& getProtocolInterface() const;

    const MemorySpaceWtc640& getMemorySpace() const;
    void setMemorySpace(const MemorySpaceWtc640& memorySpace);

    [[nodiscard]] virtual VoidResult readData(std::span<uint8_t> data, uint32_t address, ProgressTask progress) override;
    [[nodiscard]] virtual VoidResult writeData(const std::span<const uint8_t> data, uint32_t address, ProgressTask progress) override;
    [[nodiscard]] virtual ValueResult<std::vector<uint8_t>> readSomeData(uint32_t address, ProgressTask progress) override;

    std::optional<uint32_t> getAccumulatedRegisterChangesAndReset();

    const std::shared_ptr<Status>& getStatus() const;

    static constexpr uint32_t FLASH_BYTES_PER_SECTOR = 65536;

private:
    using Duration = std::chrono::steady_clock::duration;
    using ErrorWindow = std::bitset<8>;
    static constexpr size_t MAX_ERRORS_IN_WINDOW = 4;

    [[nodiscard]] VoidResult writeDataImpl(const std::span<const uint8_t> data, uint32_t address, const Duration& expectedOperationDuration,
                                           const uint32_t maxDataSize, Duration& busyDelayTotal, ErrorWindow& lastErrors, ProgressTask progress);
    [[nodiscard]] VoidResult readDataImpl(std::span<uint8_t> data, uint32_t address, uint32_t maxDataSize, ProgressTask progress);

    [[nodiscard]] VoidResult handleErrorResponse(VoidResult operationResult, ErrorWindow& lastErrors, Duration& busyDelayTotal, const std::string& operationName);
    [[nodiscard]] ValueResult<MemoryDescriptorWtc640> getMemoryDescriptorWithChecks(uint32_t address, std::optional<size_t> dataSize, const std::string& operationName) const;
    uint32_t getMaxDataSize(const MemoryDescriptorWtc640& memoryDescriptor) const;

    static uint32_t getSectorIndex(uint32_t address);

    static constexpr Duration TIMEOUT_DEFAULT = std::chrono::milliseconds(1'000);
    static constexpr Duration TIMEOUT_WRITING_FLASH = std::chrono::milliseconds(5'000);

    static constexpr Duration BUSY_DEVICE_DELAY = std::chrono::milliseconds(500);
    static constexpr Duration BUSY_DEVICE_TIMEOUT = std::chrono::milliseconds(10'000);

    static const std::string WRITE_ERROR;
    static const std::string READ_ERROR;

    std::shared_ptr<ProtocolInterfaceTCSI> m_protocolInterface;

    MemorySpaceWtc640 m_memorySpace;
    std::shared_mutex m_flashMutex;

    std::shared_ptr<Status> m_status;

    std::optional<uint32_t> m_accumulatedRegisterChanges;
    DeadlockDetectionMutex m_registerChangesMutex;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_DEVICEINTERFACEWTC640_H
