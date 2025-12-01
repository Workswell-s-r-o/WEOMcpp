#ifndef CORE_CONNECTION_MEMORYSPACEWTC640_H
#define CORE_CONNECTION_MEMORYSPACEWTC640_H

#include "core/wtc640/devicewtc640.h"
#include "core/connection/addressrange.h"
#include "core/device.h"
#include "core/misc/result.h"

#include <map>


namespace core
{

namespace connection
{

enum class MemoryTypeWtc640
{
    REGISTERS_CONFIGURATION = 1 << 0,
    REGISTERS_ULIS          = 1 << 2,
    FLASH                   = 1 << 3,
    RAM                     = 1 << 4,
};


struct MemoryDescriptorWtc640
{
    AddressRange addressRange;

    MemoryTypeWtc640 type {MemoryTypeWtc640::REGISTERS_CONFIGURATION};

    uint32_t minimumDataSize {0};
    uint32_t maximumDataSize {0};

    std::strong_ordering operator<=>(const MemoryDescriptorWtc640& other) const = default;

    MemoryDescriptorWtc640(const AddressRange& addressRange, MemoryTypeWtc640 type);

    static uint32_t getMinimumDataSize(MemoryTypeWtc640 type);
    static uint32_t getMaximumDataSize(MemoryTypeWtc640 type);
};


class MemorySpaceWtc640
{
    MemorySpaceWtc640(const std::vector<MemoryDescriptorWtc640>& memoryDescriptors);

public:
    ValueResult<MemoryDescriptorWtc640> getMemoryDescriptor(const AddressRange& addressRange) const;

    const std::vector<MemoryDescriptorWtc640>& getMemoryDescriptors() const;

    static MemorySpaceWtc640 getDeviceSpace(const std::optional<DeviceType>& deviceType);

    static constexpr AddressRange CONFIGURATION_REGISTERS = AddressRange::firstToLast(0x00000000, 0x00000FFF);

    static constexpr uint32_t ADDRESS_SENSOR_ULIS_START = 0x50000000;
    static constexpr AddressRange SENSOR_ULIS = AddressRange::firstToLast(ADDRESS_SENSOR_ULIS_START, ADDRESS_SENSOR_ULIS_START + 0xF9);

    static constexpr uint32_t FLASH_WORD_SIZE = 4;
    static constexpr uint32_t FLASH_MAX_DATA_SIZE = 256 - FLASH_WORD_SIZE;
    static constexpr AddressRange FLASH_MEMORY = AddressRange::firstToLast(0xD0000000, 0xDFFFFFFF);
    static constexpr uint32_t ADDRESS_FLASH_REGISTERS_START = FLASH_MEMORY.getFirstAddress() + 0x00800000;

    static constexpr AddressRange RAM = AddressRange::firstToLast(0xE0000000, 0xFFFFFFFF);

    // Calibration matrice in RAM
    static constexpr AddressRange RAM_CALIBRATION_MATRICE = AddressRange::firstAndSize(RAM.getFirstAddress() + DevicesWtc640::WIDTH*DevicesWtc640::HEIGHT*2*14 + DevicesWtc640::WIDTH*DevicesWtc640::HEIGHT*2*2, 640*480*2*4);

    // Loader
    static constexpr AddressRange LOADER_FIRMWARE_DATA     = AddressRange::firstToLast(0xD000'0000, 0xDFFF'FFFF);

    // Control - 0x00xx
    static constexpr AddressRange DEVICE_IDENTIFICATOR = AddressRange::firstAndSize(0x0000, 4); // + Loader
    static constexpr AddressRange TRIGGER              = AddressRange::firstAndSize(0x0004, 4); // + Loader
    
    static constexpr AddressRange STATUS               = AddressRange::firstAndSize(0x000C, 4); // + Loader

    // General - 0x01xx
    static constexpr AddressRange MAIN_FIRMWARE_VERSION          = AddressRange::firstAndSize(0x0100, 4); // + Loader
    static constexpr AddressRange PLUGIN_TYPE                    = AddressRange::firstAndSize(0x0104, 4); // + Loader
    static constexpr AddressRange MAIN_FIRMWARE_TYPE             = AddressRange::firstAndSize(0x0108, 4);
    static constexpr AddressRange FPGA_BOARD_TEMPERATURE         = AddressRange::firstAndSize(0x010C, 4);
    static constexpr AddressRange SHUTTER_TEMPERATURE            = AddressRange::firstAndSize(0x0110, 4);
    static constexpr AddressRange SERIAL_NUMBER_CURRENT          = AddressRange::firstAndSize(0x0114, 32);
    static constexpr AddressRange ARTICLE_NUMBER_CURRENT         = AddressRange::firstAndSize(0x0134, 32);
    
    static constexpr AddressRange LED_R_BRIGHTNESS_CURRENT       = AddressRange::firstAndSize(0x0164, 4);
    static constexpr AddressRange LED_G_BRIGHTNESS_CURRENT       = AddressRange::firstAndSize(0x0168, 4);
    static constexpr AddressRange LED_B_BRIGHTNESS_CURRENT       = AddressRange::firstAndSize(0x016C, 4);
    static constexpr AddressRange LOADER_FIRMWARE_VERSION        = AddressRange::firstAndSize(0x0170, 4);

    static constexpr AddressRange SERIAL_NUMBER_IN_FLASH          = SERIAL_NUMBER_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange ARTICLE_NUMBER_IN_FLASH         = ARTICLE_NUMBER_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange LED_R_BRIGHTNESS_IN_FLASH       = LED_R_BRIGHTNESS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange LED_G_BRIGHTNESS_IN_FLASH       = LED_G_BRIGHTNESS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange LED_B_BRIGHTNESS_IN_FLASH       = LED_B_BRIGHTNESS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // Video - 0x02xx
    static constexpr AddressRange PALETTE_INDEX_CURRENT    = AddressRange::firstAndSize(0x0200, 4);

    static constexpr AddressRange FRAME_RATE_CURRENT            = AddressRange::firstAndSize(0x0204, 4);
    static constexpr AddressRange IMAGE_FLIP_CURRENT            = AddressRange::firstAndSize(0x0208, 4);
    static constexpr AddressRange IMAGE_FREEZE                  = AddressRange::firstAndSize(0x020C, 4);
    static constexpr AddressRange VIDEO_FORMAT_CURRENT          = AddressRange::firstAndSize(0x0210, 4);
    static constexpr AddressRange TEST_PATTERN                  = AddressRange::firstAndSize(0x0214, 4);
    static constexpr AddressRange FPS_LOCK                      = AddressRange::firstAndSize(0x220, 4);
    static constexpr AddressRange RETICLE_MODE_CURRENT          = AddressRange::firstAndSize(0x0234, 4);
    static constexpr AddressRange CROSS_SHIFT_X_AXIS_CURRENT    = AddressRange::firstAndSize(0x0238, 4);
    static constexpr AddressRange CROSS_SHIFT_Y_AXIS_CURRENT    = AddressRange::firstAndSize(0x023C, 4);

    static constexpr AddressRange PALETTE_INDEX_IN_FLASH        = PALETTE_INDEX_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange FRAME_RATE_IN_FLASH           = FRAME_RATE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange IMAGE_FLIP_IN_FLASH           = IMAGE_FLIP_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange VIDEO_FORMAT_IN_FLASH         = VIDEO_FORMAT_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange RETICLE_MODE_IN_FLASH         = RETICLE_MODE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange CROSS_SHIFT_X_AXIS_IN_FLASH   = CROSS_SHIFT_X_AXIS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange CROSS_SHIFT_Y_AXIS_IN_FLASH   = CROSS_SHIFT_Y_AXIS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // NUC - 0x03xx
    static constexpr AddressRange TIME_FROM_LAST_NUC_OFFSET_UPDATE = AddressRange::firstAndSize(0x0304, 4);
    static constexpr AddressRange NUC_UPDATE_MODE_CURRENT          = AddressRange::firstAndSize(0x0308, 4);
    static constexpr AddressRange NUC_ENABLE                       = AddressRange::firstAndSize(0x030C, 4);
    static constexpr AddressRange NUC_UPDATE_MODE_ENABLE           = AddressRange::firstAndSize(0x0310, 4);
    static constexpr AddressRange INTERNAL_SHUTTER_POSITION        = AddressRange::firstAndSize(0x0314, 4);
    static constexpr AddressRange SNUC_ENABLE                      = AddressRange::firstAndSize(0x0318, 4);
    static constexpr AddressRange NUC_MAX_PERIOD_CURRENT           = AddressRange::firstAndSize(0x0320, 4);
    static constexpr AddressRange NUC_ADAPTIVE_THRESHOLD_CURRENT   = AddressRange::firstAndSize(0x0324, 4);

    static constexpr AddressRange NUC_UPDATE_MODE_IN_FLASH         = NUC_UPDATE_MODE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange NUC_MAX_PERIOD_IN_FLASH          = NUC_MAX_PERIOD_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange NUC_ADAPTIVE_THRESHOLD_IN_FLASH  = NUC_ADAPTIVE_THRESHOLD_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // Connection - 0x04xx
    static constexpr AddressRange UART_BAUDRATE_CURRENT = AddressRange::firstAndSize(0x0400, 4); // + Loader

    static constexpr AddressRange UART_BAUDRATE_IN_FLASH = UART_BAUDRATE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // Filters - 0x06xx
    static constexpr AddressRange TIME_DOMAIN_AVERAGE_CURRENT                 = AddressRange::firstAndSize(0x0600, 4);
    static constexpr AddressRange IMAGE_EQUALIZATION_TYPE_CURRENT             = AddressRange::firstAndSize(0x0604, 4);
    static constexpr AddressRange MGC_CONTRAST_BRIGHTNESS_CURRENT             = AddressRange::firstAndSize(0x0608, 4);
    static constexpr AddressRange FRAME_BLOCK_MEDIAN_CONBRIGHT                = AddressRange::firstAndSize(0x060C, 4);
    static constexpr AddressRange AGC_NH_SMOOTHING_CURRENT                    = AddressRange::firstAndSize(0x0610, 4);
    static constexpr AddressRange SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT        = AddressRange::firstAndSize(0x0614, 4);
    static constexpr AddressRange LINEAR_GAIN_WEIGHT_CURRENT                  = AddressRange::firstAndSize(0x0620, 4);
    static constexpr AddressRange CLIP_LIMIT_CURRENT                          = AddressRange::firstAndSize(0x0624, 4);
    static constexpr AddressRange PLATEAU_TAIL_REJECTION_CURRENT              = AddressRange::firstAndSize(0x0628, 4);
    static constexpr AddressRange SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT = AddressRange::firstAndSize(0x062C, 4);
    static constexpr AddressRange SMART_MEDIAN_THRESHOLD_CURRENT              = AddressRange::firstAndSize(0x0630, 4);
    static constexpr AddressRange GAMMA_CORRECTION_CURRENT                    = AddressRange::firstAndSize(0x0634, 4);
    static constexpr AddressRange MAX_AMPLIFICATION_CURRENT                   = AddressRange::firstAndSize(0x0638, 4);
    static constexpr AddressRange PLATEAU_SMOOTHING_CURRENT                   = AddressRange::firstAndSize(0x063C, 4);

    static constexpr AddressRange TIME_DOMAIN_AVERAGE_IN_FLASH                 = TIME_DOMAIN_AVERAGE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange IMAGE_EQUALIZATION_TYPE_IN_FLASH             = IMAGE_EQUALIZATION_TYPE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange MGC_CONTRAST_BRIGHTNESS_IN_FLASH             = MGC_CONTRAST_BRIGHTNESS_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange AGC_NH_SMOOTHING_IN_FLASH                    = AGC_NH_SMOOTHING_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH        = SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange LINEAR_GAIN_WEIGHT_IN_FLASH                  = LINEAR_GAIN_WEIGHT_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange CLIP_LIMIT_IN_FLASH                          = CLIP_LIMIT_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange PLATEAU_TAIL_REJECTION_IN_FLASH              = PLATEAU_TAIL_REJECTION_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH = SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange SMART_MEDIAN_THRESHOLD_IN_FLASH              = SMART_MEDIAN_THRESHOLD_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange GAMMA_CORRECTION_IN_FLASH                    = GAMMA_CORRECTION_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange MAX_AMPLIFICATION_IN_FLASH                   = MAX_AMPLIFICATION_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);
    static constexpr AddressRange PLATEAU_SMOOTHING_IN_FLASH                   = PLATEAU_SMOOTHING_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // Capture - 0x07xx
    static constexpr AddressRange NUMBER_OF_FRAMES_TO_CAPTURE = AddressRange::firstAndSize(0x0700, 4);
    static constexpr AddressRange CAPTURE_FRAME_ADDRESS       = AddressRange::firstAndSize(0x0704, 4);

    // DPR - 0x08xx
    static constexpr AddressRange ENABLE_DP_REPLACEMENT_CURRENT = AddressRange::firstAndSize(0x0800, 4);

    static constexpr AddressRange ENABLE_DP_REPLACEMENT_IN_FLASH = ENABLE_DP_REPLACEMENT_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    // Focus - 0x09xx
    static constexpr AddressRange MOTOR_FOCUS_MODE    = AddressRange::firstAndSize(0x0900, 4);
    static constexpr AddressRange CURRENT_MF_POSITION = AddressRange::firstAndSize(0x0904, 4);
    static constexpr AddressRange TARGET_MF_POSITION  = AddressRange::firstAndSize(0x0908, 4);
    static constexpr AddressRange MAXIMAL_MF_POSITION = AddressRange::firstAndSize(0x090C, 4);
    static constexpr AddressRange LENS_SERIAL_NUMBER  = AddressRange::firstAndSize(0x0910, 32);
    static constexpr AddressRange LENS_ARTICLE_NUMBER = AddressRange::firstAndSize(0x0930, 32);

    // Presets - 0x0Axx
    static constexpr AddressRange SELECTED_PRESET_INDEX_CURRENT       = AddressRange::firstAndSize(0x0A00, 4);
    static constexpr AddressRange CURRENT_PRESET_INDEX                = AddressRange::firstAndSize(0x0A04, 4);
    static constexpr AddressRange SELECTED_ATTRIBUTE_AND_PRESET_INDEX = AddressRange::firstAndSize(0x0A08, 4);
    static constexpr AddressRange ATTRIBUTE_ADDRESS                   = AddressRange::firstAndSize(0x0A0C, 4);
    static constexpr AddressRange NUMBER_OF_PRESETS_AND_ATTRIBUTES    = AddressRange::firstAndSize(0x0A10, 8);

    static constexpr AddressRange SELECTED_PRESET_INDEX_IN_FLASH = SELECTED_PRESET_INDEX_CURRENT.moved(ADDRESS_FLASH_REGISTERS_START);

    static constexpr uint32_t PRESET_MATRIX_SIZE = DevicesWtc640::WIDTH * DevicesWtc640::HEIGHT * 2;
    static constexpr uint32_t PRESET_SNUC_TABLE_SIZE = 256 * 2 * 2;

    // Palettes data
    static constexpr unsigned PALETTES_FACTORY_MAX_COUNT = 14;
    static constexpr unsigned PALETTES_USER_MAX_COUNT = 2;
    static constexpr AddressRange PALETTES_REGISTERS = AddressRange::firstToLast(0x30000000, 0x300040FF);
    static constexpr AddressRange getPaletteDataCurrent(unsigned paletteIndex);
    static constexpr AddressRange getPaletteNameCurrent(unsigned paletteIndex);
    static constexpr AddressRange getPaletteDataInFlash(unsigned paletteIndex);
    static constexpr AddressRange getPaletteNameInFlash(unsigned paletteIndex);
    static constexpr uint32_t PALETTES_FLASH_OFFSET = 0xA0941000;
    static constexpr uint32_t PALETTE_DATA_SIZE = 1024;
    static constexpr uint32_t PALETTE_NAME_SIZE = 16;

    // DPR data
    static constexpr uint32_t MAX_DEADPIXELS_COUNT = 2047;
    static constexpr uint32_t MAX_REPLACEMENTS_COUNT = 4095;
    static constexpr uint32_t DEADPIXEL_SIZE = 4;
    static constexpr uint32_t DEADPIXEL_REPLACEMENT_SIZE = 8;

    static constexpr AddressRange DEAD_PIXELS_CURRENT                  = AddressRange::firstAndSize(0x2200'0000, (MAX_DEADPIXELS_COUNT + 1 /* terminate pixel */) * DEADPIXEL_SIZE);
    static constexpr AddressRange DEAD_PIXELS_REPLACEMENTS_CURRENT     = AddressRange::firstAndSize(0x2300'0000, (MAX_REPLACEMENTS_COUNT + 1 /* terminate replacement */) * DEADPIXEL_REPLACEMENT_SIZE);
    static constexpr AddressRange DEAD_PIXELS_IN_FLASH                 = AddressRange::firstAndSize(0xD080'B000, DEAD_PIXELS_CURRENT.getSize());
    static constexpr AddressRange DEAD_PIXELS_REPLACEMENTS_IN_FLASH    = AddressRange::firstAndSize(0xD080'D000, DEAD_PIXELS_REPLACEMENTS_CURRENT.getSize());

    // loader property
    static constexpr AddressRange BOOT_TO_LOADER_IN_FLASH = AddressRange::firstAndSize(0xD080'0000, 4);
private:
    std::vector<MemoryDescriptorWtc640> m_memoryDescriptors;
};

// Impl

constexpr AddressRange MemorySpaceWtc640::getPaletteDataCurrent(unsigned paletteIndex)
{
    assert(paletteIndex < (PALETTES_FACTORY_MAX_COUNT + PALETTES_USER_MAX_COUNT));
    return AddressRange::firstAndSize(PALETTES_REGISTERS.getFirstAddress() + paletteIndex * PALETTE_DATA_SIZE, PALETTE_DATA_SIZE);
}

constexpr AddressRange MemorySpaceWtc640::getPaletteNameCurrent(unsigned paletteIndex)
{
    assert(paletteIndex < (PALETTES_FACTORY_MAX_COUNT + PALETTES_USER_MAX_COUNT));
    return AddressRange::firstAndSize(PALETTES_REGISTERS.getFirstAddress() + 0x4000 + paletteIndex * PALETTE_NAME_SIZE, PALETTE_NAME_SIZE);
}

constexpr AddressRange MemorySpaceWtc640::getPaletteDataInFlash(unsigned paletteIndex)
{
    return getPaletteDataCurrent(paletteIndex).moved(PALETTES_FLASH_OFFSET);
}

constexpr AddressRange MemorySpaceWtc640::getPaletteNameInFlash(unsigned paletteIndex)
{
    return getPaletteNameCurrent(paletteIndex).moved(PALETTES_FLASH_OFFSET);
}

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_MEMORYSPACEWTC640_H
