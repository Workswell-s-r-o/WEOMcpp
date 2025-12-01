#include "core/wtc640/deviceinterfacewtc640.h"

#include "core/connection/resultdeviceinfo.h"
#include "core/connection/protocolinterfacetcsi.h"
#include "core/logging.h"
#include "core/utils.h"

#include <cmath>


namespace core
{

namespace connection
{

const std::string DeviceInterfaceWtc640::WRITE_ERROR = "Write error!";
const std::string DeviceInterfaceWtc640::READ_ERROR = "Read error!";

DeviceInterfaceWtc640::DeviceInterfaceWtc640(const std::shared_ptr<ProtocolInterfaceTCSI>& protocolInterface, const std::shared_ptr<Status>& status) :
    BaseClass(BaseClass::DeviceEndianity::LITTLE),
    m_protocolInterface(protocolInterface),
    m_memorySpace(MemorySpaceWtc640::getDeviceSpace(std::nullopt)),
    m_status(status)
{
}

const std::shared_ptr<ProtocolInterfaceTCSI>& DeviceInterfaceWtc640::getProtocolInterface() const
{
    return m_protocolInterface;
}

const MemorySpaceWtc640& DeviceInterfaceWtc640::getMemorySpace() const
{
    return m_memorySpace;
}

void DeviceInterfaceWtc640::setMemorySpace(const MemorySpaceWtc640& memorySpace)
{
    m_memorySpace = memorySpace;
}

VoidResult DeviceInterfaceWtc640::readData(std::span<uint8_t> data, uint32_t address, ProgressTask progress)
{
    const auto memoryDescriptor = getMemoryDescriptorWithChecks(address, data.size(), READ_ERROR);
    if (!memoryDescriptor.isOk())
    {
        return memoryDescriptor.toVoidResult();
    }

    const std::shared_lock lock(m_flashMutex);

    return readDataImpl(data, address, getMaxDataSize(memoryDescriptor.getValue()), progress);
}

VoidResult DeviceInterfaceWtc640::writeData(const std::span<const uint8_t> data, uint32_t address, ProgressTask progress)
{
    const auto memoryDescriptor = getMemoryDescriptorWithChecks(address, data.size(), WRITE_ERROR);
    if (!memoryDescriptor.isOk())
    {
        return memoryDescriptor.toVoidResult();
    }

    const uint32_t maxDataSize = getMaxDataSize(memoryDescriptor.getValue());
    Duration busyDelayTotal = std::chrono::milliseconds(0);
    ErrorWindow lastErrors;

    if (memoryDescriptor.getValue().type != MemoryTypeWtc640::FLASH)
    {
        const std::shared_lock lock(m_flashMutex);

        return writeDataImpl(data, address, TIMEOUT_DEFAULT,
                             maxDataSize, busyDelayTotal, lastErrors, progress);
    }

    const std::unique_lock lock(m_flashMutex);

    // write with flash burst:
    std::span<const uint8_t> restOfData(data);
    for (uint32_t currentAddress = address; !restOfData.empty(); )
    {
        if (currentAddress > address)
        {
            WW_LOG_CONNECTION_DEBUG << "burst next sector";
        }
        const uint32_t nextSectorStart = (1 + getSectorIndex(currentAddress)) * FLASH_BYTES_PER_SECTOR;
        assert(nextSectorStart > currentAddress);
        const uint32_t dataSizeToWritePerSector = std::min<uint32_t>(restOfData.size(), nextSectorStart - currentAddress);
        assert(dataSizeToWritePerSector > 0 && dataSizeToWritePerSector % memoryDescriptor.getValue().minimumDataSize == 0);

        m_status->incrementFlashBurstWritesCount();

        const uint32_t burstcount = dataSizeToWritePerSector / memoryDescriptor.getValue().minimumDataSize;

        auto writeDataResult = VoidResult::createError("initial send");
        int counter = 0;
        while (!writeDataResult.isOk() && counter < MAX_ERRORS_IN_WINDOW)
        {
            counter++;
            WW_LOG_CONNECTION_DEBUG << "burst write failed in sector with address - " << currentAddress << ", trying failed sector again, retry number: " << counter;

            while (true)
            {
                const auto writeBurstStartResult = m_protocolInterface->writeFlashBurstStart(currentAddress, burstcount, TIMEOUT_WRITING_FLASH);
                if (writeBurstStartResult.isOk())
                {
                    counter = 0;
                    break;
                }
                else
                {
                    const auto result = handleErrorResponse(writeBurstStartResult, lastErrors, busyDelayTotal, WRITE_ERROR);
                    if (!result.isOk())
                    {
                        return result;
                    }
                }
            }
            writeDataResult = writeDataImpl(restOfData.first(dataSizeToWritePerSector), currentAddress, TIMEOUT_WRITING_FLASH, maxDataSize, busyDelayTotal, lastErrors, progress);
        }
        if(counter == MAX_ERRORS_IN_WINDOW)
        {
            return writeDataResult;
        }

        while (true)
        {
            const auto writeBurstEndResult = m_protocolInterface->writeFlashBurstEnd(currentAddress, TIMEOUT_WRITING_FLASH);
            if (writeBurstEndResult.isOk())
            {
                break;
            }
            else
            {
                const auto result = handleErrorResponse(writeBurstEndResult, lastErrors, busyDelayTotal, WRITE_ERROR);
                if (!result.isOk())
                {
                    return result;
                }
            }
        }

        restOfData = restOfData.last(restOfData.size() - dataSizeToWritePerSector);
        currentAddress += dataSizeToWritePerSector;
    }

    return VoidResult::createOk();
}

ValueResult<std::vector<uint8_t>> DeviceInterfaceWtc640::readSomeData(uint32_t address, ProgressTask progress)
{
    using ResultType = ValueResult<std::vector<uint8_t>>;

    TRY_GET_RESULT(const auto memoryDescriptor, getMemoryDescriptorWithChecks(address, std::nullopt, READ_ERROR));

    const auto availableAddressRange = AddressRange::firstToLast(address, memoryDescriptor.addressRange.getLastAddress());
    const uint32_t dataSize = std::min(getMaxDataSize(memoryDescriptor), availableAddressRange.getSize());
    if (dataSize == 0)
    {
        return ResultType::createError(READ_ERROR, "Unexpected end of memory");
    }

    std::vector<uint8_t> data(dataSize, 0);

    const std::shared_lock lock(m_flashMutex);

    TRY_RESULT(readDataImpl(data, address, dataSize, progress));
    return data;
}

std::optional<uint32_t> DeviceInterfaceWtc640::getAccumulatedRegisterChangesAndReset()
{
    const std::scoped_lock lock(m_registerChangesMutex);

    const auto changes = m_accumulatedRegisterChanges;
    m_accumulatedRegisterChanges = std::nullopt;

    return changes;
}

const std::shared_ptr<Status>& DeviceInterfaceWtc640::getStatus() const
{
    return m_status;
}

VoidResult DeviceInterfaceWtc640::writeDataImpl(const std::span<const uint8_t> data, uint32_t address, const Duration& expectedOperationDuration,
                                                const uint32_t maxDataSize, Duration& busyDelayTotal, ErrorWindow& lastErrors, ProgressTask progress)
{
    std::span<const uint8_t> restOfData = data;
    for (uint32_t currentAddress = address; !restOfData.empty(); )
    {
        const auto dataSize = std::min<uint32_t>(restOfData.size(), maxDataSize);

        const auto writeResult = m_protocolInterface->writeData(restOfData.first(dataSize), currentAddress, expectedOperationDuration);
        lastErrors <<= 1;
        if (writeResult.isOk())
        {
            currentAddress += dataSize;
            restOfData = restOfData.last(restOfData.size() - dataSize);

            progress.advanceByIgnoreCancel(dataSize);
        }
        else
        {
            const auto result = handleErrorResponse(writeResult, lastErrors, busyDelayTotal, WRITE_ERROR);
            if (!result.isOk())
            {
                return result;
            }
        }
    }

    return VoidResult::createOk();
}

VoidResult DeviceInterfaceWtc640::readDataImpl(std::span<uint8_t> data, uint32_t address, uint32_t maxDataSize, ProgressTask progress)
{
    Duration busyDelayTotal = std::chrono::milliseconds(0);
    ErrorWindow lastErrors;

    std::span<uint8_t> restOfData = data;
    for (uint32_t currentAddress = address; !restOfData.empty(); )
    {
        const auto addressRange = AddressRange::firstAndSize(currentAddress, std::min<uint32_t>(restOfData.size(), maxDataSize));

        const auto dataRange = restOfData.first(addressRange.getSize());
        const auto readResult = m_protocolInterface->readData(dataRange, addressRange.getFirstAddress(), TIMEOUT_DEFAULT);
        lastErrors <<= 1;
        if (readResult.isOk())
        {
            currentAddress += addressRange.getSize();
            restOfData = restOfData.last(restOfData.size() - addressRange.getSize());

            if (addressRange.overlaps(MemorySpaceWtc640::STATUS))
            {
                assert(addressRange == MemorySpaceWtc640::STATUS);

                const std::scoped_lock lock(m_registerChangesMutex);

                if (!m_accumulatedRegisterChanges.has_value())
                {
                    m_accumulatedRegisterChanges = 0;
                }

                const auto& value = reinterpret_cast<const uint32_t&>(*dataRange.begin());
                m_accumulatedRegisterChanges = m_accumulatedRegisterChanges.value() | fromDeviceEndianity(value);
            }

            if (progress.advanceByIsCancelled(addressRange.getSize()))
            {
                return VoidResult::createError(READ_ERROR, "User cancelled");
            }
        }
        else
        {
            const auto result = handleErrorResponse(readResult, lastErrors, busyDelayTotal, READ_ERROR);
            if (!result.isOk())
            {
                return result;
            }
        }
    }

    return VoidResult::createOk();
}

VoidResult DeviceInterfaceWtc640::handleErrorResponse(VoidResult operationResult, ErrorWindow& lastErrors, Duration& busyDelayTotal, const std::string& operationName)
{
    WW_LOG_CONNECTION_WARNING << operationResult;

    if (operationResult.getSpecificInfo() != nullptr)
    {
        const auto* resultDeviceInfo = dynamic_cast<const ResultDeviceInfo*>(operationResult.getSpecificInfo());
        if (resultDeviceInfo != nullptr)
        {
            if (resultDeviceInfo->error == ResultDeviceInfo::Error::TRANSMISSION_FAILED || resultDeviceInfo->error == ResultDeviceInfo::Error::NO_RESPONSE)
            {
                lastErrors.set(0, 1);
                if (lastErrors.count() <= MAX_ERRORS_IN_WINDOW)
                {
                    return VoidResult::createOk();
                }
                else
                {
                    return VoidResult::createError("Too many errors!", utils::format("{} errors in last {} packets", lastErrors.count(), lastErrors.size()), operationResult.getSpecificInfo());
                }
            }
            else if (resultDeviceInfo->error == ResultDeviceInfo::Error::DEVICE_IS_BUSY)
            {
                busyDelayTotal += BUSY_DEVICE_DELAY;
                if (busyDelayTotal < BUSY_DEVICE_TIMEOUT)
                {
                    std::this_thread::sleep_for(BUSY_DEVICE_DELAY);

                    return VoidResult::createOk();
                }
                else
                {
                    return VoidResult::createError("Camera is busy!", utils::format("busyDelayTotal: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(busyDelayTotal).count()), operationResult.getSpecificInfo());
                }
            }
        }
    }

    return VoidResult::createError(operationName, operationResult.getDetailErrorMessage(), operationResult.getSpecificInfo());
}

ValueResult<MemoryDescriptorWtc640> DeviceInterfaceWtc640::getMemoryDescriptorWithChecks(uint32_t address, std::optional<size_t> dataSize, const std::string& operationName) const
{
    using ResultType = ValueResult<MemoryDescriptorWtc640>;

    if (!m_protocolInterface || m_protocolInterface->getMaxDataSize() == 0)
    {
        return ResultType::createError(operationName, "No connection! No protocol interface set or max packet size 0");
    }

    if (dataSize && dataSize.value() == 0)
    {
        return ResultType::createError(operationName, "Data size = 0");
    }

    if (dataSize && dataSize.value() - 1 > std::numeric_limits<uint32_t>::max() - address)
    {
        return ResultType::createError(operationName, "Memory overflow");
    }

    const ResultType memoryDescriptor = m_memorySpace.getMemoryDescriptor(AddressRange::firstAndSize(address, dataSize.value_or(1)));

    if (!memoryDescriptor.isOk())
    {
        return ResultType::createError(operationName, memoryDescriptor.getDetailErrorMessage());
    }

    if (address % memoryDescriptor.getValue().minimumDataSize != 0)
    {
        return ResultType::createError(operationName, utils::format("Invalid alignment - address: {} (must be multiple of {})", AddressRange::addressToHexString(address), memoryDescriptor.getValue().minimumDataSize));
    }

    if (dataSize && dataSize.value() % memoryDescriptor.getValue().minimumDataSize != 0)
    {
        return ResultType::createError(operationName, utils::format("Invalid alignment - size: {} (must be multiple of {})", dataSize.value(), memoryDescriptor.getValue().minimumDataSize));
    }

    return memoryDescriptor;
}

uint32_t DeviceInterfaceWtc640::getMaxDataSize(const MemoryDescriptorWtc640& memoryDescriptor) const
{
    const auto protocolMaxDataSize = (m_protocolInterface->getMaxDataSize() / memoryDescriptor.minimumDataSize) * memoryDescriptor.minimumDataSize;
    assert(protocolMaxDataSize > 0);
    return std::min(memoryDescriptor.maximumDataSize, protocolMaxDataSize);
}

uint32_t DeviceInterfaceWtc640::getSectorIndex(uint32_t address)
{
    return address / FLASH_BYTES_PER_SECTOR;
}

} // namespace connection

} // namespace core
