#include <boost/test/unit_test.hpp>

#include "testfixture.h"

#include <array>
#include <set>
#include <vector>

namespace
{

std::vector<core::PropertyId> getAllDeclaredPropertyIds()
{
    const std::array<core::PropertyId, 121> listPropertyIds =
    {
        core::PropertyIdWtc640::STATUS,
        core::PropertyIdWtc640::LOGIN_ROLE,
        core::PropertyIdWtc640::MAIN_FIRMWARE_VERSION,
        core::PropertyIdWtc640::USB_PLUGIN_FIRMWARE_VERSION,
        core::PropertyIdWtc640::USB_PLUGIN_SERIAL_NUMBER,
        core::PropertyIdWtc640::PLUGIN_TYPE,
        core::PropertyIdWtc640::MAIN_FIRMWARE_TYPE,
        core::PropertyIdWtc640::LOADER_FIRMWARE_VERSION,
        core::PropertyIdWtc640::FPGA_BOARD_TEMPERATURE,
        core::PropertyIdWtc640::SHUTTER_TEMPERATURE,
        core::PropertyIdWtc640::SERIAL_NUMBER_CURRENT,
        core::PropertyIdWtc640::PRODUCTION_DATE,
        core::PropertyIdWtc640::SERIAL_NUMBER_IN_FLASH,
        core::PropertyIdWtc640::ARTICLE_NUMBER_CURRENT,
        core::PropertyIdWtc640::SENSOR_TYPE_CURRENT,
        core::PropertyIdWtc640::MAX_FRAMERATE_CURRENT,
        core::PropertyIdWtc640::CORE_TYPE_CURRENT,
        core::PropertyIdWtc640::FOCUS_TYPE_CURRENT,
        core::PropertyIdWtc640::DETECTOR_SENSITIVITY_CURRENT,
        core::PropertyIdWtc640::ARTICLE_NUMBER_IN_FLASH,
        core::PropertyIdWtc640::SENSOR_TYPE_IN_FLASH,
        core::PropertyIdWtc640::MAX_FRAMERATE_IN_FLASH,
        core::PropertyIdWtc640::CORE_TYPE_IN_FLASH,
        core::PropertyIdWtc640::FOCUS_TYPE_IN_FLASH,
        core::PropertyIdWtc640::DETECTOR_SENSITIVITY_IN_FLASH,
        core::PropertyIdWtc640::LED_R_BRIGHTNESS_CURRENT,
        core::PropertyIdWtc640::LED_G_BRIGHTNESS_CURRENT,
        core::PropertyIdWtc640::LED_B_BRIGHTNESS_CURRENT,
        core::PropertyIdWtc640::LED_R_BRIGHTNESS_IN_FLASH,
        core::PropertyIdWtc640::LED_G_BRIGHTNESS_IN_FLASH,
        core::PropertyIdWtc640::LED_B_BRIGHTNESS_IN_FLASH,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_MODE_CURRENT,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_0_CURRENT,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_1_CURRENT,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_2_CURRENT,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_0_IN_FLASH,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_1_IN_FLASH,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_PIN_2_IN_FLASH,
        core::PropertyIdWtc640::AUXILARY_TRIGGER_MODE_IN_FLASH,
        core::PropertyIdWtc640::PALETTE_INDEX_CURRENT,
        core::PropertyIdWtc640::PALETTE_INDEX_IN_FLASH,
        core::PropertyIdWtc640::FRAMERATE_CURRENT,
        core::PropertyIdWtc640::FRAMERATE_IN_FLASH,
        core::PropertyIdWtc640::VIDEO_FORMAT_CURRENT,
        core::PropertyIdWtc640::VIDEO_FORMAT_IN_FLASH,
        core::PropertyIdWtc640::IMAGE_FLIP_CURRENT,
        core::PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_CURRENT,
        core::PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_CURRENT,
        core::PropertyIdWtc640::IMAGE_FLIP_IN_FLASH,
        core::PropertyIdWtc640::FLIP_IMAGE_VERTICALLY_IN_FLASH,
        core::PropertyIdWtc640::FLIP_IMAGE_HORIZONTALLY_IN_FLASH,
        core::PropertyIdWtc640::IMAGE_FREEZE,
        core::PropertyIdWtc640::TEST_PATTERN,
        core::PropertyIdWtc640::FPS_LOCK,
        core::PropertyIdWtc640::TIME_FROM_LAST_NUC_OFFSET_UPDATE,
        core::PropertyIdWtc640::NUC_UPDATE_MODE_CURRENT,
        core::PropertyIdWtc640::NUC_UPDATE_MODE_IN_FLASH,
        core::PropertyIdWtc640::INTERNAL_SHUTTER_POSITION,
        core::PropertyIdWtc640::NUC_MAX_PERIOD_CURRENT,
        core::PropertyIdWtc640::NUC_MAX_PERIOD_IN_FLASH,
        core::PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_CURRENT,
        core::PropertyIdWtc640::NUC_ADAPTIVE_THRESHOLD_IN_FLASH,
        core::PropertyIdWtc640::UART_BAUDRATE_CURRENT,
        core::PropertyIdWtc640::UART_BAUDRATE_IN_FLASH,
        core::PropertyIdWtc640::TIME_DOMAIN_AVERAGE_CURRENT,
        core::PropertyIdWtc640::TIME_DOMAIN_AVERAGE_IN_FLASH,
        core::PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_CURRENT,
        core::PropertyIdWtc640::IMAGE_EQUALIZATION_TYPE_IN_FLASH,
        core::PropertyIdWtc640::LINEAR_GAIN_WEIGHT_CURRENT,
        core::PropertyIdWtc640::CLIP_LIMIT_CURRENT,
        core::PropertyIdWtc640::PLATEAU_TAIL_REJECTION_CURRENT,
        core::PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_CURRENT,
        core::PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_CURRENT,
        core::PropertyIdWtc640::LINEAR_GAIN_WEIGHT_IN_FLASH,
        core::PropertyIdWtc640::CLIP_LIMIT_IN_FLASH,
        core::PropertyIdWtc640::PLATEAU_TAIL_REJECTION_IN_FLASH,
        core::PropertyIdWtc640::SMART_TIME_DOMAIN_AVERAGE_THRESHOLD_IN_FLASH,
        core::PropertyIdWtc640::SMART_MEDIAN_THRESHOLD_IN_FLASH,
        core::PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_CURRENT,
        core::PropertyIdWtc640::MGC_CONTRAST_CURRENT,
        core::PropertyIdWtc640::MGC_BRIGHTNESS_CURRENT,
        core::PropertyIdWtc640::MGC_CONTRAST_BRIGHTNESS_IN_FLASH,
        core::PropertyIdWtc640::MGC_CONTRAST_IN_FLASH,
        core::PropertyIdWtc640::MGC_BRIGHTNESS_IN_FLASH,
        core::PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONBRIGHT,
        core::PropertyIdWtc640::FRAME_BLOCK_MEDIAN_CONTRAST,
        core::PropertyIdWtc640::FRAME_BLOCK_MEDIAN_BRIGHTNESS,
        core::PropertyIdWtc640::AGC_NH_SMOOTHING_CURRENT,
        core::PropertyIdWtc640::AGC_NH_SMOOTHING_IN_FLASH,
        core::PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_CURRENT,
        core::PropertyIdWtc640::SPATIAL_MEDIAN_FILTER_ENABLE_IN_FLASH,
        core::PropertyIdWtc640::GAMMA_CORRECTION_CURRENT,
        core::PropertyIdWtc640::GAMMA_CORRECTION_IN_FLASH,
        core::PropertyIdWtc640::MAX_AMPLIFICATION_CURRENT,
        core::PropertyIdWtc640::MAX_AMPLIFICATION_IN_FLASH,
        core::PropertyIdWtc640::PLATEAU_SMOOTHING_CURRENT,
        core::PropertyIdWtc640::PLATEAU_SMOOTHING_IN_FLASH,
        core::PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_CURRENT,
        core::PropertyIdWtc640::DEAD_PIXELS_CORRECTION_ENABLED_IN_FLASH,
        core::PropertyIdWtc640::MOTOR_FOCUS_MODE,
        core::PropertyIdWtc640::CURRENT_MF_POSITION,
        core::PropertyIdWtc640::TARGET_MF_POSITION,
        core::PropertyIdWtc640::MAXIMAL_MF_POSITION,
        core::PropertyIdWtc640::LENS_SERIAL_NUMBER,
        core::PropertyIdWtc640::LENS_ARTICLE_NUMBER,
        core::PropertyIdWtc640::CURRENT_PRESET_INDEX,
        core::PropertyIdWtc640::SELECTED_PRESET_INDEX_CURRENT,
        core::PropertyIdWtc640::SELECTED_PRESET_INDEX_IN_FLASH,
        core::PropertyIdWtc640::ACTIVE_LENS_RANGE,
        core::PropertyIdWtc640::SELECTED_LENS_RANGE_CURRENT,
        core::PropertyIdWtc640::SELECTED_LENS_RANGE_IN_FLASH,
        core::PropertyIdWtc640::ALL_VALID_LENS_RANGES,
        core::PropertyIdWtc640::BOOT_TO_LOADER_IN_FLASH,
        core::PropertyIdWtc640::DEAD_PIXELS_CURRENT,
        core::PropertyIdWtc640::DEAD_PIXELS_IN_FLASH,
        core::PropertyIdWtc640::RETICLE_MODE_CURRENT,
        core::PropertyIdWtc640::RETICLE_MODE_IN_FLASH,
        core::PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_CURRENT,
        core::PropertyIdWtc640::RETICLE_SHIFT_X_AXIS_IN_FLASH,
        core::PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_CURRENT,
        core::PropertyIdWtc640::RETICLE_SHIFT_Y_AXIS_IN_FLASH,
    };

    std::vector<core::PropertyId> allPropertyIds;
    allPropertyIds.reserve(listPropertyIds.size()
                           + core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.size()
                           + core::PropertyIdWtc640::PALETTES_USER_CURRENT.size()
                           + core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.size()
                           + core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.size());

    allPropertyIds.insert(allPropertyIds.end(), listPropertyIds.begin(), listPropertyIds.end());
    allPropertyIds.insert(allPropertyIds.end(), core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.begin(), core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.end());
    allPropertyIds.insert(allPropertyIds.end(), core::PropertyIdWtc640::PALETTES_USER_CURRENT.begin(), core::PropertyIdWtc640::PALETTES_USER_CURRENT.end());
    allPropertyIds.insert(allPropertyIds.end(), core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.begin(), core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.end());
    allPropertyIds.insert(allPropertyIds.end(), core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.begin(), core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.end());

    return allPropertyIds;
}

std::set<core::PropertyId> asPropertySet(const std::vector<core::PropertyId>& ids)
{
    return std::set<core::PropertyId>(ids.begin(), ids.end());
}

} // namespace

BOOST_AUTO_TEST_SUITE(PropertyIdWtc640CoverageSuite)

BOOST_AUTO_TEST_CASE(AllDeclaredPropertyIdsAreUnique)
{
    const auto allPropertyIds = getAllDeclaredPropertyIds();
    const auto allPropertyIdsSet = asPropertySet(allPropertyIds);
    BOOST_TEST(allPropertyIdsSet.size() == allPropertyIds.size());
}

BOOST_AUTO_TEST_CASE(PaletteCollectionsAndAccessorsAreConsistent)
{
    const auto expectedPalettesCount = core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.size() + core::PropertyIdWtc640::PALETTES_USER_CURRENT.size();
    BOOST_TEST(core::PropertyIdWtc640::getPalettesCount() == expectedPalettesCount);
    BOOST_TEST(core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.size() == core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.size());
    BOOST_TEST(core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.size() == core::PropertyIdWtc640::PALETTES_USER_CURRENT.size());

    std::vector<core::PropertyId> expectedCurrentPalettes;
    expectedCurrentPalettes.insert(expectedCurrentPalettes.end(), core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.begin(), core::PropertyIdWtc640::PALETTES_FACTORY_CURRENT.end());
    expectedCurrentPalettes.insert(expectedCurrentPalettes.end(), core::PropertyIdWtc640::PALETTES_USER_CURRENT.begin(), core::PropertyIdWtc640::PALETTES_USER_CURRENT.end());

    std::vector<core::PropertyId> expectedInFlashPalettes;
    expectedInFlashPalettes.insert(expectedInFlashPalettes.end(), core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.begin(), core::PropertyIdWtc640::PALETTES_FACTORY_IN_FLASH.end());
    expectedInFlashPalettes.insert(expectedInFlashPalettes.end(), core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.begin(), core::PropertyIdWtc640::PALETTES_USER_IN_FLASH.end());

    for (size_t i = 0; i < core::PropertyIdWtc640::getPalettesCount(); ++i)
    {
        BOOST_CHECK_EQUAL(core::PropertyIdWtc640::getPaletteCurrentId(static_cast<unsigned>(i)).getIdString(),
                          expectedCurrentPalettes.at(i).getIdString());
        BOOST_CHECK_EQUAL(core::PropertyIdWtc640::getPaletteInFlashId(static_cast<unsigned>(i)).getIdString(),
                          expectedInFlashPalettes.at(i).getIdString());
    }
}

BOOST_AUTO_TEST_CASE(PropertiesExposeAdaptersForAllDeclaredPropertyIds)
{
    auto indicator = std::make_shared<MainThreadIndicatorMock>();
    auto properties = core::PropertiesWtc640::createInstance(core::Properties::Mode::SYNC_DIRECT, indicator, nullptr);
    BOOST_REQUIRE(properties != nullptr);

    // No active connection still has to expose the full static property set.
    properties->createConnectionStateTransaction().disconnectCore();

    auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(false);
    const auto registeredProperties = exclusiveTransaction.getConnectionExclusiveTransaction().getPropertiesTransaction().getAllProperyIds();
    const auto expectedProperties = asPropertySet(getAllDeclaredPropertyIds());

    BOOST_TEST(registeredProperties.size() == expectedProperties.size());

    for (const auto& expectedPropertyId : expectedProperties)
    {
        BOOST_CHECK_MESSAGE(registeredProperties.contains(expectedPropertyId),
                            "Missing adapter for property: " << expectedPropertyId.getIdString());
    }

    for (const auto& registeredPropertyId : registeredProperties)
    {
        BOOST_CHECK_MESSAGE(expectedProperties.contains(registeredPropertyId),
                            "Unexpected property adapter registered: " << registeredPropertyId.getIdString());
    }
}

BOOST_AUTO_TEST_SUITE_END()
