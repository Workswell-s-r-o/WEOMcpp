#ifndef CORE_PROPERTYIDWTC640_H
#define CORE_PROPERTYIDWTC640_H

#include "core/properties/propertyid.h"

#include <array>


namespace core
{

class PropertyIdWtc640
{
    PropertyIdWtc640();

public:

    static const PropertyId STATUS;
        static const PropertyId LOGIN_ROLE;

    // General
    static const PropertyId MAIN_FIRMWARE_VERSION;
    static const PropertyId USB_PLUGIN_FIRMWARE_VERSION;
    static const PropertyId USB_PLUGIN_SERIAL_NUMBER;
    static const PropertyId PLUGIN_TYPE;
    static const PropertyId MAIN_FIRMWARE_TYPE;
    static const PropertyId LOADER_FIRMWARE_VERSION;

    static const PropertyId FPGA_BOARD_TEMPERATURE;
    static const PropertyId SHUTTER_TEMPERATURE;

    static const PropertyId SERIAL_NUMBER_CURRENT;
        static const PropertyId PRODUCTION_DATE;
    static const PropertyId SERIAL_NUMBER_IN_FLASH;

    static const PropertyId ARTICLE_NUMBER_CURRENT;
        static const PropertyId SENSOR_TYPE_CURRENT;
        static const PropertyId MAX_FRAMERATE_CURRENT;
        static const PropertyId CORE_TYPE_CURRENT;
        static const PropertyId FOCUS_TYPE_CURRENT;
        static const PropertyId DETECTOR_SENSITIVITY_CURRENT;

    static const PropertyId ARTICLE_NUMBER_IN_FLASH;
        static const PropertyId SENSOR_TYPE_IN_FLASH;
        static const PropertyId MAX_FRAMERATE_IN_FLASH;
        static const PropertyId CORE_TYPE_IN_FLASH;
        static const PropertyId FOCUS_TYPE_IN_FLASH;
        static const PropertyId DETECTOR_SENSITIVITY_IN_FLASH;

    static const PropertyId LED_R_BRIGHTNESS_CURRENT;
    static const PropertyId LED_G_BRIGHTNESS_CURRENT;
    static const PropertyId LED_B_BRIGHTNESS_CURRENT;
    static const PropertyId LED_R_BRIGHTNESS_IN_FLASH;
    static const PropertyId LED_G_BRIGHTNESS_IN_FLASH;
    static const PropertyId LED_B_BRIGHTNESS_IN_FLASH;

    // Video
    static const PropertyId PALETTE_INDEX_CURRENT;
    static const PropertyId PALETTE_INDEX_IN_FLASH;

    static const PropertyId FRAMERATE_CURRENT;
    static const PropertyId FRAMERATE_IN_FLASH;

    static const PropertyId VIDEO_FORMAT_CURRENT;
    static const PropertyId VIDEO_FORMAT_IN_FLASH;

    static const PropertyId IMAGE_FLIP_CURRENT;
        static const PropertyId FLIP_IMAGE_VERTICALLY_CURRENT;
        static const PropertyId FLIP_IMAGE_HORIZONTALLY_CURRENT;
    static const PropertyId IMAGE_FLIP_IN_FLASH;
        static const PropertyId FLIP_IMAGE_VERTICALLY_IN_FLASH;
        static const PropertyId FLIP_IMAGE_HORIZONTALLY_IN_FLASH;

    static const PropertyId IMAGE_FREEZE;
    static const PropertyId TEST_PATTERN;
    static const PropertyId FPS_LOCK;

    // NUC
    static const PropertyId TIME_FROM_LAST_NUC_OFFSET_UPDATE;

    static const PropertyId NUC_UPDATE_MODE_CURRENT;
    static const PropertyId NUC_UPDATE_MODE_IN_FLASH;


    static const PropertyId INTERNAL_SHUTTER_POSITION;

    static const PropertyId NUC_MAX_PERIOD_CURRENT;
    static const PropertyId NUC_MAX_PERIOD_IN_FLASH;

    static const PropertyId NUC_ADAPTIVE_THRESHOLD_CURRENT;
    static const PropertyId NUC_ADAPTIVE_THRESHOLD_IN_FLASH;

    // Connection
    static const PropertyId UART_BAUDRATE_CURRENT;
    static const PropertyId UART_BAUDRATE_IN_FLASH;

    // Filters
    static const PropertyId TIME_DOMAIN_AVERAGE_CURRENT;
    static const PropertyId TIME_DOMAIN_AVERAGE_IN_FLASH;

    static const PropertyId IMAGE_EQUALIZATION_TYPE_CURRENT;
    static const PropertyId IMAGE_EQUALIZATION_TYPE_IN_FLASH;

    static const PropertyId LINEAR_GAIN_WEIGHT_CURRENT;
    static const PropertyId CLIP_LIMIT_CURRENT;
    static const PropertyId PLATEAU_TAIL_REJECTION_CURRENT;
    static const PropertyId SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT;
    static const PropertyId SMART_MEDIAN_THRESHOLD_CURRENT;

    static const PropertyId LINEAR_GAIN_WEIGHT_IN_FLASH;
    static const PropertyId CLIP_LIMIT_IN_FLASH;
    static const PropertyId PLATEAU_TAIL_REJECTION_IN_FLASH;
    static const PropertyId SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH;
    static const PropertyId SMART_MEDIAN_THRESHOLD_IN_FLASH;

    static const PropertyId MGC_CONTRAST_BRIGHTNESS_CURRENT;
        static const PropertyId MGC_CONTRAST_CURRENT;
        static const PropertyId MGC_BRIGHTNESS_CURRENT;
    static const PropertyId MGC_CONTRAST_BRIGHTNESS_IN_FLASH;
        static const PropertyId MGC_CONTRAST_IN_FLASH;
        static const PropertyId MGC_BRIGHTNESS_IN_FLASH;

    static const PropertyId FRAME_BLOCK_MEDIAN_CONBRIGHT;
        static const PropertyId FRAME_BLOCK_MEDIAN_CONTRAST;
        static const PropertyId FRAME_BLOCK_MEDIAN_BRIGHTNESS;

    static const PropertyId AGC_NH_SMOOTHING_CURRENT;
    static const PropertyId AGC_NH_SMOOTHING_IN_FLASH;

    static const PropertyId SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT;
    static const PropertyId SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH;

    static const PropertyId GAMMA_CORRECTION_CURRENT;
    static const PropertyId GAMMA_CORRECTION_IN_FLASH;

    static const PropertyId MAX_AMPLIFICATION_CURRENT;
    static const PropertyId MAX_AMPLIFICATION_IN_FLASH;
    static const PropertyId PLATEAU_SMOOTHING_CURRENT;
    static const PropertyId PLATEAU_SMOOTHING_IN_FLASH;

    // DPR
    static const PropertyId DEAD_PIXELS_CORRECTION_ENABLED_CURRENT;
    static const PropertyId DEAD_PIXELS_CORRECTION_ENABLED_IN_FLASH;

    // Focus
    static const PropertyId MOTOR_FOCUS_MODE;

    static const PropertyId CURRENT_MF_POSITION;
    static const PropertyId TARGET_MF_POSITION;
    static const PropertyId MAXIMAL_MF_POSITION;

    static const PropertyId LENS_SERIAL_NUMBER;
    static const PropertyId LENS_ARTICLE_NUMBER;

    // Presets
    static const PropertyId CURRENT_PRESET_INDEX;
    static const PropertyId SELECTED_PRESET_INDEX_CURRENT;
    static const PropertyId SELECTED_PRESET_INDEX_IN_FLASH;

    static const PropertyId ACTIVE_LENS_RANGE;
    static const PropertyId SELECTED_LENS_RANGE_CURRENT;
    static const PropertyId SELECTED_LENS_RANGE_IN_FLASH;
    static const PropertyId ALL_VALID_LENS_RANGES;

    static const PropertyId BOOT_TO_LOADER_IN_FLASH;

    // Palettes data
    static const std::vector<PropertyId> PALETTES_FACTORY_CURRENT;
    static const std::vector<PropertyId> PALETTES_USER_CURRENT;
    static const std::vector<PropertyId> PALETTES_FACTORY_IN_FLASH;
    static const std::vector<PropertyId> PALETTES_USER_IN_FLASH;
    static unsigned getPalettesCount();
    static PropertyId getPaletteCurrentId(unsigned index);
    static PropertyId getPaletteInFlashId(unsigned index);

    // DPR data
    static const PropertyId DEAD_PIXELS_CURRENT;
    static const PropertyId DEAD_PIXELS_IN_FLASH;

    // Cross
    static const PropertyId RETICLE_MODE_CURRENT;
    static const PropertyId RETICLE_MODE_IN_FLASH;
    static const PropertyId RETICLE_SHIFT_X_AXIS_CURRENT;
    static const PropertyId RETICLE_SHIFT_X_AXIS_IN_FLASH;
    static const PropertyId RETICLE_SHIFT_Y_AXIS_CURRENT;
    static const PropertyId RETICLE_SHIFT_Y_AXIS_IN_FLASH;
};

} // namespace core

#endif // CORE_PROPERTYIDWTC640_H
