#include "core/wtc640/memoryspacewtc640.h"
#include "core/wtc640/devicewtc640.h"
#include "core/utils.h"

namespace core
{

namespace connection
{

MemorySpaceWtc640::MemorySpaceWtc640(const std::vector<MemoryDescriptorWtc640>& memoryDescriptors) :
    m_memoryDescriptors(memoryDescriptors)
{
}

ValueResult<MemoryDescriptorWtc640> MemorySpaceWtc640::getMemoryDescriptor(const AddressRange& addressRange) const
{
    using ResultType = ValueResult<MemoryDescriptorWtc640>;

    for (const auto& descriptor : m_memoryDescriptors)
    {
        if (descriptor.addressRange.contains(addressRange))
        {
            return descriptor;
        }
    }

    return ResultType::createError("Invalid address!", utils::format("range: {}", addressRange.toHexString()));
}

const std::vector<MemoryDescriptorWtc640>& MemorySpaceWtc640::getMemoryDescriptors() const
{
    return m_memoryDescriptors;
}

MemorySpaceWtc640 MemorySpaceWtc640::getDeviceSpace(const std::optional<DeviceType>& deviceType)
{
    if (deviceType.has_value())
    {
        std::vector<MemoryDescriptorWtc640> memoryDescriptors;

        if (deviceType.value() == DevicesWtc640::LOADER)
        {
            memoryDescriptors.push_back(MemoryDescriptorWtc640{DEVICE_IDENTIFICATOR,    MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{TRIGGER,                 MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{STATUS,                  MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{MAIN_FIRMWARE_VERSION,   MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{LOADER_FIRMWARE_VERSION, MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{PLUGIN_TYPE,             MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{UART_BAUDRATE_CURRENT,   MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{LOADER_FIRMWARE_DATA,     MemoryTypeWtc640::FLASH});

            return MemorySpaceWtc640(memoryDescriptors);
        }
        else
        {
            assert(deviceType.value() == DevicesWtc640::MAIN_USER);

            memoryDescriptors.push_back(MemoryDescriptorWtc640{CONFIGURATION_REGISTERS,             MemoryTypeWtc640::REGISTERS_CONFIGURATION});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{DEAD_PIXELS_CURRENT,                 MemoryTypeWtc640::REGISTERS_CONFIGURATION});
            memoryDescriptors.push_back(MemoryDescriptorWtc640{DEAD_PIXELS_REPLACEMENTS_CURRENT,    MemoryTypeWtc640::REGISTERS_CONFIGURATION});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{PALETTES_REGISTERS,      MemoryTypeWtc640::REGISTERS_CONFIGURATION});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{SENSOR_ULIS,             MemoryTypeWtc640::REGISTERS_ULIS});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{FLASH_MEMORY,            MemoryTypeWtc640::FLASH});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{RAM,                     MemoryTypeWtc640::RAM});

            memoryDescriptors.push_back(MemoryDescriptorWtc640{LOADER_FIRMWARE_DATA,     MemoryTypeWtc640::FLASH});

            return MemorySpaceWtc640(memoryDescriptors);
        }
    }

    std::vector<MemoryDescriptorWtc640> memoryDescriptors;

    memoryDescriptors.push_back(MemoryDescriptorWtc640{DEVICE_IDENTIFICATOR, MemoryTypeWtc640::REGISTERS_CONFIGURATION});
    memoryDescriptors.push_back(MemoryDescriptorWtc640{STATUS,               MemoryTypeWtc640::REGISTERS_CONFIGURATION});

    return MemorySpaceWtc640(memoryDescriptors);
}

MemoryDescriptorWtc640::MemoryDescriptorWtc640(const AddressRange& addressRange, MemoryTypeWtc640 type) :
    addressRange(addressRange),
    type(type),
    minimumDataSize(getMinimumDataSize(type)),
    maximumDataSize(getMaximumDataSize(type))
{
}

uint32_t MemoryDescriptorWtc640::getMinimumDataSize(MemoryTypeWtc640 type)
{
    switch (type)
    {
        case MemoryTypeWtc640::REGISTERS_CONFIGURATION:
            return 4;

        case MemoryTypeWtc640::REGISTERS_ULIS:
            return 1;

        case MemoryTypeWtc640::FLASH:
            return MemorySpaceWtc640::FLASH_WORD_SIZE;

        case MemoryTypeWtc640::RAM:
            return 8;

        default:
            assert(false);
            return 0;
    }
}

uint32_t MemoryDescriptorWtc640::getMaximumDataSize(MemoryTypeWtc640 type)
{
    switch (type)
    {
        case MemoryTypeWtc640::REGISTERS_CONFIGURATION:
            return 4;

        case MemoryTypeWtc640::REGISTERS_ULIS:
            return 1;

        case MemoryTypeWtc640::FLASH:
            return MemorySpaceWtc640::FLASH_MAX_DATA_SIZE;

        case MemoryTypeWtc640::RAM:
            return 256 - 8;

        default:
            assert(false);
            return 0;
    }
}

} // namespace connection

} // namespace core
