#include "core/wtc640/propertyidwtc640.h"

#include <array>
#include <set>
#include <cassert>

namespace core
{

const PropertyId PropertyIdWtc640::STATUS = PropertyId::createPropertyId("STATUS", "");
const PropertyId PropertyIdWtc640::LOGIN_ROLE = PropertyId::createPropertyId("LOGIN_ROLE", "");

const PropertyId PropertyIdWtc640::MAIN_FIRMWARE_VERSION = PropertyId::createPropertyId("MAIN_FIRMWARE_VERSION", "");
const PropertyId PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION = PropertyId::createPropertyId("USB_PLUGIN_FIRMWARE_VERSION", "");
const PropertyId PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER = PropertyId::createPropertyId("USB_PLUGIN_SERIAL_NUMBER", "");
const PropertyId PropertyIdWtc640::PLUGIN_TYPE = PropertyId::createPropertyId("PLUGIN_TYPE", "");
const PropertyId PropertyIdWtc640::MAIN_FIRMWARE_TYPE = PropertyId::createPropertyId("MAIN_FIRMWARE_TYPE", "");
const PropertyId PropertyIdWtc640::LOADER_FIRMWARE_VERSION = PropertyId::createPropertyId("LOADER_FIRMWARE_VERSION", "");

const PropertyId PropertyIdWtc640::FPGA_BOARD_TEMPERATURE = PropertyId::createPropertyId("FPGA_BOARD_TEMPERATURE", "");
const PropertyId PropertyIdWtc640::SHUTTER_TEMPERATURE = PropertyId::createPropertyId("SHUTTER_TEMPERATURE", "");

const PropertyId PropertyIdWtc640::SERIAL_NUMBER_CURRENT = PropertyId::createPropertyId("SERIAL_NUMBER_CURRENT", "");
const PropertyId PropertyIdWtc640::PRODUCTION_DATE = PropertyId::createPropertyId("PRODUCTION_DATE", "");
const PropertyId PropertyIdWtc640::SERIAL_NUMBER_IN_FLASH = PropertyId::createPropertyId("SERIAL_NUMBER_IN_FLASH", "");

const PropertyId PropertyIdWtc640::ARTICLE_NUMBER_CURRENT = PropertyId::createPropertyId("ARTICLE_NUMBER_CURRENT", "");
const PropertyId PropertyIdWtc640::SENSOR_TYPE_CURRENT = PropertyId::createPropertyId("SENSOR_TYPE_CURRENT", "");
const PropertyId PropertyIdWtc640::MAX_FRAMERATE_CURRENT = PropertyId::createPropertyId("MAX_FRAMERATE_CURRENT", "");
const PropertyId PropertyIdWtc640::CORE_TYPE_CURRENT = PropertyId::createPropertyId("CORE_TYPE_CURRENT", "");
const PropertyId PropertyIdWtc640::FOCUS_TYPE_CURRENT = PropertyId::createPropertyId("FOCUS_TYPE_CURRENT", "");
const PropertyId PropertyIdWtc640::DETECTOR_SENSITIVITY_CURRENT = PropertyId::createPropertyId("DETECTOR_SENSITIVITY_CURRENT", "");

const PropertyId PropertyIdWtc640::ARTICLE_NUMBER_IN_FLASH = PropertyId::createPropertyId("ARTICLE_NUMBER_IN_FLASH", "");
const PropertyId PropertyIdWtc640::SENSOR_TYPE_IN_FLASH = PropertyId::createPropertyId("SENSOR_TYPE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::MAX_FRAMERATE_IN_FLASH = PropertyId::createPropertyId("MAX_FRAMERATE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::CORE_TYPE_IN_FLASH = PropertyId::createPropertyId("CORE_TYPE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::FOCUS_TYPE_IN_FLASH = PropertyId::createPropertyId("FOCUS_TYPE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::DETECTOR_SENSITIVITY_IN_FLASH = PropertyId::createPropertyId("DETECTOR_SENSITIVITY_IN_FLASH", "");

const PropertyId PropertyIdWtc640::LED_R_BRIGHTNESS_CURRENT = PropertyId::createPropertyId("LED_R_BRIGHTNESS_CURRENT", "");
const PropertyId PropertyIdWtc640::LED_G_BRIGHTNESS_CURRENT = PropertyId::createPropertyId("LED_G_BRIGHTNESS_CURRENT", "");
const PropertyId PropertyIdWtc640::LED_B_BRIGHTNESS_CURRENT = PropertyId::createPropertyId("LED_B_BRIGHTNESS_CURRENT", "");
const PropertyId PropertyIdWtc640::LED_R_BRIGHTNESS_IN_FLASH = PropertyId::createPropertyId("LED_R_BRIGHTNESS_IN_FLASH", "");
const PropertyId PropertyIdWtc640::LED_G_BRIGHTNESS_IN_FLASH = PropertyId::createPropertyId("LED_G_BRIGHTNESS_IN_FLASH", "");
const PropertyId PropertyIdWtc640::LED_B_BRIGHTNESS_IN_FLASH = PropertyId::createPropertyId("LED_B_BRIGHTNESS_IN_FLASH", "");

const PropertyId PropertyIdWtc640::PALETTE_INDEX_CURRENT = PropertyId::createPropertyId("PALETTE_INDEX_CURRENT", "");
const PropertyId PropertyIdWtc640::PALETTE_INDEX_IN_FLASH = PropertyId::createPropertyId("PALETTE_INDEX_IN_FLASH", "");

const PropertyId PropertyIdWtc640::FRAMERATE_CURRENT = PropertyId::createPropertyId("FRAMERATE_CURRENT", "");
const PropertyId PropertyIdWtc640::FRAMERATE_IN_FLASH = PropertyId::createPropertyId("FRAMERATE_IN_FLASH", "");

const PropertyId PropertyIdWtc640::VIDEO_FORMAT_CURRENT = PropertyId::createPropertyId("VIDEO_FORMAT_CURRENT", "");
const PropertyId PropertyIdWtc640::VIDEO_FORMAT_IN_FLASH = PropertyId::createPropertyId("VIDEO_FORMAT_IN_FLASH", "");

const PropertyId PropertyIdWtc640::IMAGE_FLIP_CURRENT = PropertyId::createPropertyId("IMAGE_FLIP_CURRENT", "");
const PropertyId PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_CURRENT = PropertyId::createPropertyId("FLIP_IMAGE_VERTICALLY_CURRENT", "");
const PropertyId PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_CURRENT = PropertyId::createPropertyId("FLIP_IMAGE_HORIZONTALLY_CURRENT", "");

const PropertyId PropertyIdWtc640::IMAGE_FLIP_IN_FLASH = PropertyId::createPropertyId("IMAGE_FLIP_IN_FLASH", "");
const PropertyId PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_IN_FLASH = PropertyId::createPropertyId("FLIP_IMAGE_VERTICALLY_IN_FLASH", "");
const PropertyId PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_IN_FLASH = PropertyId::createPropertyId("FLIP_IMAGE_HORIZONTALLY_IN_FLASH", "");

const PropertyId PropertyIdWtc640::IMAGE_FREEZE = PropertyId::createPropertyId("IMAGE_FREEZE", "");
const PropertyId PropertyIdWtc640::TEST_PATTERN = PropertyId::createPropertyId("TEST_PATTERN", "");
const PropertyId PropertyIdWtc640::FPS_LOCK = PropertyId::createPropertyId("FPS_LOCK", "");

const PropertyId PropertyIdWtc640::GAMMA_CORRECTION_CURRENT = PropertyId::createPropertyId("GAMMA_CORRECTION_CURRENT", "");
const PropertyId PropertyIdWtc640::GAMMA_CORRECTION_IN_FLASH = PropertyId::createPropertyId("GAMMA_CORRECTION_IN_FLASH", "");
const PropertyId PropertyIdWtc640::MAX_AMPLIFICATION_CURRENT = PropertyId::createPropertyId("MAX_AMPLIFICATION_CURRENT", "");
const PropertyId PropertyIdWtc640::MAX_AMPLIFICATION_IN_FLASH = PropertyId::createPropertyId("MAX_AMPLIFICATION_IN_FLASH", "");
const PropertyId PropertyIdWtc640::PLATEAU_SMOOTHING_CURRENT = PropertyId::createPropertyId("PLATEAU_SMOOTHING_CURRENT", "");
const PropertyId PropertyIdWtc640::PLATEAU_SMOOTHING_IN_FLASH = PropertyId::createPropertyId("PLATEAU_SMOOTHING_IN_FLASH", "");

const PropertyId PropertyIdWtc640::TIME_FROM_LAST_NUC_OFFSET_UPDATE = PropertyId::createPropertyId("TIME_FROM_LAST_NUC_OFFSET_UPDATE", "");

const PropertyId PropertyIdWtc640::NUC_UPDATE_MODE_CURRENT = PropertyId::createPropertyId("NUC_UPDATE_MODE_CURRENT", "");
const PropertyId PropertyIdWtc640::NUC_UPDATE_MODE_IN_FLASH = PropertyId::createPropertyId("NUC_UPDATE_MODE_IN_FLASH", "");


const PropertyId PropertyIdWtc640::INTERNAL_SHUTTER_POSITION = PropertyId::createPropertyId("INTERNAL_SHUTTER_POSITION", "");

const PropertyId PropertyIdWtc640::NUC_MAX_PERIOD_CURRENT = PropertyId::createPropertyId("NUC_MAX_PERIOD_CURRENT", "");
const PropertyId PropertyIdWtc640::NUC_MAX_PERIOD_IN_FLASH = PropertyId::createPropertyId("NUC_MAX_PERIOD_IN_FLASH", "");

const PropertyId PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_CURRENT = PropertyId::createPropertyId("NUC_ADAPTIVE_THRESHOLD_CURRENT", "");
const PropertyId PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_IN_FLASH = PropertyId::createPropertyId("NUC_ADAPTIVE_THRESHOLD_IN_FLASH", "");

const PropertyId PropertyIdWtc640::UART_BAUDRATE_CURRENT = PropertyId::createPropertyId("UART_BAUDRATE_CURRENT", "");
const PropertyId PropertyIdWtc640::UART_BAUDRATE_IN_FLASH = PropertyId::createPropertyId("UART_BAUDRATE_IN_FLASH", "");

const PropertyId PropertyIdWtc640::TIME_DOMAIN_AVERAGE_CURRENT = PropertyId::createPropertyId("TIME_DOMAIN_AVERAGE_CURRENT", "");
const PropertyId PropertyIdWtc640::TIME_DOMAIN_AVERAGE_IN_FLASH = PropertyId::createPropertyId("TIME_DOMAIN_AVERAGE_IN_FLASH", "");

const PropertyId PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_CURRENT = PropertyId::createPropertyId("IMAGE_EQUALIZATION_TYPE_CURRENT", "");
const PropertyId PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_IN_FLASH = PropertyId::createPropertyId("IMAGE_EQUALIZATION_TYPE_IN_FLASH", "");

const PropertyId PropertyIdWtc640::LINEAR_GAIN_WEIGHT_CURRENT = PropertyId::createPropertyId("LINEAR_GAIN_WEIGHT_CURRENT", "");
const PropertyId PropertyIdWtc640::CLIP_LIMIT_CURRENT = PropertyId::createPropertyId("CLIP_LIMIT_CURRENT", "");
const PropertyId PropertyIdWtc640::PLATEAU_TAIL_REJECTION_CURRENT = PropertyId::createPropertyId("PLATEAU_TAIL_REJECTION_CURRENT", "");
const PropertyId PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT = PropertyId::createPropertyId("SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT", "");
const PropertyId PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_CURRENT = PropertyId::createPropertyId("SMART_MEDIAN_THRESHOLD_CURRENT", "");

const PropertyId PropertyIdWtc640::LINEAR_GAIN_WEIGHT_IN_FLASH = PropertyId::createPropertyId("LINEAR_GAIN_WEIGHT_IN_FLASH", "");
const PropertyId PropertyIdWtc640::CLIP_LIMIT_IN_FLASH = PropertyId::createPropertyId("CLIP_LIMIT_IN_FLASH", "");
const PropertyId PropertyIdWtc640::PLATEAU_TAIL_REJECTION_IN_FLASH = PropertyId::createPropertyId("PLATEAU_TAIL_REJECTION_IN_FLASH", "");
const PropertyId PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH = PropertyId::createPropertyId("SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH", "");
const PropertyId PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_IN_FLASH = PropertyId::createPropertyId("SMART_MEDIAN_THRESHOLD_IN_FLASH", "");

const PropertyId PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_CURRENT = PropertyId::createPropertyId("MGC_CONTRAST_BRIGHTNESS_CURRENT", "");
const PropertyId PropertyIdWtc640::MGC_CONTRAST_CURRENT = PropertyId::createPropertyId("MGC_CONTRAST_CURRENT", "");
const PropertyId PropertyIdWtc640::MGC_BRIGHTNESS_CURRENT = PropertyId::createPropertyId("MGC_BRIGHTNESS_CURRENT", "");
const PropertyId PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_IN_FLASH = PropertyId::createPropertyId("MGC_CONTRAST_BRIGHTNESS_IN_FLASH", "");
const PropertyId PropertyIdWtc640::MGC_CONTRAST_IN_FLASH = PropertyId::createPropertyId("MGC_CONTRAST_IN_FLASH", "");
const PropertyId PropertyIdWtc640::MGC_BRIGHTNESS_IN_FLASH = PropertyId::createPropertyId("MGC_BRIGHTNESS_IN_FLASH", "");

const PropertyId PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONBRIGHT = PropertyId::createPropertyId("FRAME_BLOCK_MEDIAN_CONBRIGHT", "");
const PropertyId PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONTRAST = PropertyId::createPropertyId("FRAME_BLOCK_MEDIAN_CONTRAST", "");
const PropertyId PropertyIdWtc640::FRAME_BLOCK_MEDIAN_BRIGHTNESS = PropertyId::createPropertyId("FRAME_BLOCK_MEDIAN_BRIGHTNESS", "");

const PropertyId PropertyIdWtc640::AGC_NH_SMOOTHING_CURRENT = PropertyId::createPropertyId("AGC_NH_SMOOTHING_CURRENT", "");
const PropertyId PropertyIdWtc640::AGC_NH_SMOOTHING_IN_FLASH = PropertyId::createPropertyId("AGC_NH_SMOOTHING_IN_FLASH", "");

const PropertyId PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT = PropertyId::createPropertyId("SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT", "");
const PropertyId PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH = PropertyId::createPropertyId("SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH", "");

const PropertyId PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_CURRENT = PropertyId::createPropertyId("DEAD_PIXELS_CORRECTION_ENABLED_CURRENT", "");
const PropertyId PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_IN_FLASH = PropertyId::createPropertyId("DEAD_PIXELS_CORRECTION_ENABLED_IN_FLASH", "");

const PropertyId PropertyIdWtc640::MOTOR_FOCUS_MODE = PropertyId::createPropertyId("MOTOR_FOCUS_MODE", "");

const PropertyId PropertyIdWtc640::CURRENT_MF_POSITION = PropertyId::createPropertyId("CURRENT_MF_POSITION", "");
const PropertyId PropertyIdWtc640::TARGET_MF_POSITION = PropertyId::createPropertyId("TARGET_MF_POSITION", "");
const PropertyId PropertyIdWtc640::MAXIMAL_MF_POSITION = PropertyId::createPropertyId("MAXIMAL_MF_POSITION", "");

const PropertyId PropertyIdWtc640::LENS_SERIAL_NUMBER = PropertyId::createPropertyId("LENS_SERIAL_NUMBER", "");
const PropertyId PropertyIdWtc640::LENS_ARTICLE_NUMBER = PropertyId::createPropertyId("LENS_ARTICLE_NUMBER", "");

const PropertyId PropertyIdWtc640::CURRENT_PRESET_INDEX = PropertyId::createPropertyId("CURRENT_PRESET_INDEX", "");
const PropertyId PropertyIdWtc640::SELECTED_PRESET_INDEX_CURRENT = PropertyId::createPropertyId("SELECTED_PRESET_INDEX_CURRENT", "");
const PropertyId PropertyIdWtc640::SELECTED_PRESET_INDEX_IN_FLASH = PropertyId::createPropertyId("SELECTED_PRESET_INDEX_IN_FLASH", "");

const PropertyId PropertyIdWtc640::ACTIVE_LENS_RANGE = PropertyId::createPropertyId("ACTIVE_LENS_RANGE", "");
const PropertyId PropertyIdWtc640::SELECTED_LENS_RANGE_CURRENT = PropertyId::createPropertyId("SELECTED_LENS_RANGE_CURRENT", "");
const PropertyId PropertyIdWtc640::SELECTED_LENS_RANGE_IN_FLASH = PropertyId::createPropertyId("SELECTED_LENS_RANGE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::ALL_VALID_LENS_RANGES = PropertyId::createPropertyId("ALL_VALID_LENS_RANGES", "");

const PropertyId PropertyIdWtc640::BOOT_TO_LOADER_IN_FLASH = PropertyId::createPropertyId("BOOT_TO_LOADER_IN_FLASH", "");

const std::vector<PropertyId> PropertyIdWtc640::PALETTES_FACTORY_CURRENT
{
    PropertyId::createPropertyId("PALETTE_FACTORY_1_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_2_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_3_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_4_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_5_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_6_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_7_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_8_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_9_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_10_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_11_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_12_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_13_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_14_CURRENT", ""),
};

const std::vector<PropertyId> PropertyIdWtc640::PALETTES_USER_CURRENT
{
    PropertyId::createPropertyId("PALETTE_USER_1_CURRENT", ""),
    PropertyId::createPropertyId("PALETTE_USER_2_CURRENT", ""),
};

const std::vector<PropertyId> PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH
{
    PropertyId::createPropertyId("PALETTE_FACTORY_1_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_2_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_3_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_4_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_5_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_6_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_7_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_8_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_9_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_10_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_11_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_12_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_13_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_FACTORY_14_IN_FLASH", ""),
};

const std::vector<PropertyId> PropertyIdWtc640::PALETTES_USER_IN_FLASH
{
    PropertyId::createPropertyId("PALETTE_USER_1_IN_FLASH", ""),
    PropertyId::createPropertyId("PALETTE_USER_2_IN_FLASH", ""),
};

unsigned int PropertyIdWtc640::getPalettesCount()
{
    return PALETTES_FACTORY_CURRENT.size() + PALETTES_USER_CURRENT.size();
}

PropertyId PropertyIdWtc640::getPaletteCurrentId(unsigned index)
{
    assert(index < getPalettesCount());

    if (index < PALETTES_FACTORY_CURRENT.size())
    {
        return PALETTES_FACTORY_CURRENT.at(index);
    }

    return PALETTES_USER_CURRENT.at(index - PALETTES_FACTORY_CURRENT.size());
}

PropertyId PropertyIdWtc640::getPaletteInFlashId(unsigned index)
{
    assert(index < getPalettesCount());

    if (index < PALETTES_FACTORY_IN_FLASH.size())
    {
        return PALETTES_FACTORY_IN_FLASH.at(index);
    }

    return PALETTES_USER_IN_FLASH.at(index - PALETTES_FACTORY_IN_FLASH.size());
}

const PropertyId PropertyIdWtc640::DEAD_PIXELS_CURRENT = PropertyId::createPropertyId("DEAD_PIXELS_CURRENT", "");
const PropertyId PropertyIdWtc640::DEAD_PIXELS_IN_FLASH = PropertyId::createPropertyId("DEAD_PIXELS_IN_FLASH", "");

const PropertyId PropertyIdWtc640::RETICLE_MODE_CURRENT = PropertyId::createPropertyId("RETICLE_MODE_CURRENT", "");
const PropertyId PropertyIdWtc640::RETICLE_MODE_IN_FLASH = PropertyId::createPropertyId("RETICLE_MODE_IN_FLASH", "");
const PropertyId PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_CURRENT = PropertyId::createPropertyId("RETICLE_SHIFT_X_AXIS_CURRENT", "");
const PropertyId PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_IN_FLASH = PropertyId::createPropertyId("RETICLE_SHIFT_X_AXIS_IN_FLASH", "");
const PropertyId PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_CURRENT = PropertyId::createPropertyId("RETICLE_SHIFT_Y_AXIS_CURRENT", "");
const PropertyId PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_IN_FLASH = PropertyId::createPropertyId("RETICLE_SHIFT_Y_AXIS_IN_FLASH", "");
} // namespace core
