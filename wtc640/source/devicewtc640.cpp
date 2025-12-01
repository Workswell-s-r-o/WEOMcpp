#include "core/wtc640/devicewtc640.h"
#include "core/utils.h"

#include <limits>

namespace core
{

const DeviceType DevicesWtc640::MAIN_USER = DeviceType::createDeviceType();
const DeviceType DevicesWtc640::LOADER = DeviceType::createDeviceType();

core::Size DevicesWtc640::getSizeInPixels(DeviceType deviceType)
{
    if (deviceType == MAIN_USER)
    {
        return core::Size(WIDTH, HEIGHT);
    }
    else
    {
        assert(deviceType == LOADER);

        return core::Size();
    }
}

const std::map<LoginRole::Item, EnumValueDescription> LoginRole::ALL_ITEMS
{
    {Item::NONE,   {"None",   "NONE"}},
    {Item::LOADER, {"Loader", "LOADER"}},
    {Item::USER,   {"User",   "USER"}},
};


bool LoginRole::isMainRole(Item loginRole)
{
    return loginRole == Item::USER;
}

StatusWtc640::StatusWtc640(uint32_t value) :
    m_value(value)
{
}

bool StatusWtc640::isNucActive() const
{
    return (m_value & (1 << 0)) != 0;
}

bool StatusWtc640::isCameraNotReady() const
{
    return (m_value & (1 << 1)) != 0;
}

std::optional<DeviceType> StatusWtc640::getDeviceType() const
{
    switch ((m_value >> 3) & 0b11)
    {
        case 0b01:
            return DevicesWtc640::MAIN_USER;

        case 0b11:
            return DevicesWtc640::LOADER;

        default:
            return std::nullopt;
    }
}

bool StatusWtc640::isMotorfocusBusy() const
{
    return (m_value & (1 << 5)) != 0;
}

bool StatusWtc640::isMotorfocusAvailable() const
{
    return (m_value & (1 << 6)) != 0;
}

StatusWtc640::BayonetState StatusWtc640::getBayonetState() const
{
    return static_cast<BayonetState>((m_value >> 7) & 0b11);
}

bool StatusWtc640::isMotorfocusRunning() const
{
    return (m_value & (1 << 9)) != 0;
}

bool StatusWtc640::isMotorfocusPositionReached() const
{
    return (m_value & (1 << 10)) != 0;
}

bool StatusWtc640::isAnyTriggerActive() const
{
    return (m_value & (1 << 11)) != 0;
}

bool StatusWtc640::nucRegistersChanged() const
{
    return (m_value & (1 << 27)) != 0;
}

bool StatusWtc640::bolometerRegistersChanged() const
{
    return (m_value & (1 << 28)) != 0;
}

bool StatusWtc640::focusRegistersChanged() const
{
    return (m_value & (1 << 30)) != 0;
}

bool StatusWtc640::presetsRegistersChanged() const
{
    return (m_value & (1 << 31)) != 0;
}

std::string StatusWtc640::toString() const
{
    return utils::format("isReady: {}", isCameraNotReady() ? "N" : "Y");
}

const std::map<Baudrate::Item, EnumValueDeviceDescription> BaudrateWtc::ALL_ITEMS
{
    {Item::B_115200,  {utils::format("{}", core::Baudrate::getBaudrateSpeed(Item::B_115200)),  "B_115200",  4}},
    {Item::B_921600,  {utils::format("{}", core::Baudrate::getBaudrateSpeed(Item::B_921600)),  "B_921600",  7}},
#if !defined(__APPLE__)
    {Item::B_3000000, {utils::format("{}", core::Baudrate::getBaudrateSpeed(Item::B_3000000)), "B_3000000", 9}},
#endif
};

const std::map<Sensor::Item, EnumValueDescription> Sensor::ALL_ITEMS
{
    {Item::PICO_640, {"WTC640", "WTC640"}},
};

const std::map<Core::Item, EnumValueDescription> Core::ALL_ITEMS
{
    {Item::RADIOMETRIC,     {"R (radiometric)",     "RADIOMETRIC"}},
    {Item::NON_RADIOMETRIC, {"N (non-radiometric)", "NON_RADIOMETRIC"}},
};

const std::map<DetectorSensitivity::Item, EnumValueDescription> DetectorSensitivity::ALL_ITEMS
{
    {Item::PERFORMANCE_NETD_50MK,  {"P (performance NETd 50mK)(HG + LG)",  "PERFORMANCE_NETD_50MK"}},
    {Item::SUPERIOR_NETD_30MK,     {"S (superior NETd 30mK)(LG + HG)",     "SUPERIOR_NETD_30MK"}},
    {Item::ULTIMATE_NETD_30MK,     {"U (ultimate NETd 30mK)(HG + SG)",     "ULTIMATE_NETD_30MK"}}
};

const std::map<Focus::Item, Focus::Description> Focus::ALL_ITEMS
{
    {Item::MANUAL_H25,               {"H25 (Non-motoric, manual focusable lens, 25mm screw)", "MANUAL_H25",               "H25"}},
    {Item::MANUAL_H34,               {"H34 (Non-motoric, manual focusable lens, 34mm screw)", "MANUAL_H34",               "H34"}},
    {Item::MOTORIC_E25,              {"E25 (Motor focus system, 25mm lens screw)",            "MOTORIC_E25",              "E25"}},
    {Item::MOTORIC_E34,              {"E34 (Motor focus system, 34mm lens screw)",            "MOTORIC_E34",              "E34"}},
    {Item::MOTORIC_WITH_BAYONET_B25, {"B25 (Motor focus system, bayonet lens interface M25)", "MOTORIC_WITH_BAYONET_B25", "B25"}},
    {Item::MOTORIC_WITH_BAYONET_B34, {"B34 (Motor focus system, bayonet lens interface M34)", "MOTORIC_WITH_BAYONET_B34", "B34"}},
};

bool Focus::isMotoric(Item item)
{
    switch (item)
    {
    case Item::MANUAL_H25:
    case Item::MANUAL_H34:
        return false;

    case Item::MOTORIC_E25:
    case Item::MOTORIC_E34:
    case Item::MOTORIC_WITH_BAYONET_B25:
    case Item::MOTORIC_WITH_BAYONET_B34:
        return true;
    }
    assert(false);
    return false;
}

bool Focus::isWithBayonet(Item item)
{
    switch (item)
    {
    case Item::MANUAL_H25:
    case Item::MANUAL_H34:
    case Item::MOTORIC_E25:
    case Item::MOTORIC_E34:
        return false;

    case Item::MOTORIC_WITH_BAYONET_B25:
    case Item::MOTORIC_WITH_BAYONET_B34:
        return true;
    }
    assert(false);
    return false;
}

const std::map<VideoFormat::Item, EnumValueDeviceDescription> VideoFormat::ALL_ITEMS
{
    {Item::PRE_IGC,                   {"Pre IGC",             "PRE_IGC",               0}},
    {Item::POST_IGC,                  {"Post IGC",            "POST_IGC",              1}},
    {Item::POST_COLORING,             {"Post Coloring"       ,"POST_COLORING",         2}},
};

const std::map<ImageGenerator::Item, EnumValueDeviceDescription> ImageGenerator::ALL_ITEMS
{
    {Item::SENSOR,           {"Infrared detector",             "SENSOR",               0b000}},
    {Item::ADC_1,            {"Static test pattern",           "ADC_1",                0b001}},
    {Item::INTERNAL_DYNAMIC, {"Dynamic test pattern",          "TEST_PATTERN_DYNAMIC", 0b011}},
};

const std::map<Plugin::Item, EnumValueDeviceDescription> Plugin::ALL_ITEMS
{
    {Item::CMOS,    {"CMOS",   "CMOS",      0b1111}},
    {Item::HDMI,    {"HDMI",   "HDMI",      0b0001}},
    {Item::ANALOG,  {"Analog", "ANALOG",    0b0011}},
    {Item::USB,     {"USB",    "USB",       0b1110}},
    {Item::PLEORA,  {"GigE",   "GIGE",      0b0111}},
    {Item::CVBS,    {"CVBS",   "CVBS",      0b1011}},
    {Item::ONVIF,   {"ONVIF",  "ONVIF",     0b0000}},
};

const std::map<FirmwareType::Item, EnumValueDeviceDescription> FirmwareType::ALL_ITEMS
{
    {Item::CMOS_PLEORA, {"CMOS/GigE",   "CMOS_GIGE",    0b1111}},
    {Item::HDMI,        {"HDMI",        "HDMI",         0b0001}},
    {Item::ANALOG,      {"Analog",      "ANALOG",       0b0011}},
    {Item::USB,         {"USB",         "USB",          0b1110}},
    {Item::ALL,         {"ALL",         "ALL",          0b0000}},
};

const std::map<Framerate::Item, EnumValueDeviceDescription> Framerate::ALL_ITEMS
{
    {Item::FPS_8_57, {"8.57 fps", "FPS_8_57",   0}},
    {Item::FPS_30,   {"30 fps",   "FPS_30",     1}},
    {Item::FPS_60,   {"60 fps",   "FPS_60",     2}},
};

double Framerate::getFramerateValue(Item type)
{
    switch (type)
    {
        case Item::FPS_8_57:
            return 8.57;

        case Item::FPS_30:
            return 30;

        case Item::FPS_60:
            return 60;

        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

const std::map<TimeDomainAveraging::Item, EnumValueDeviceDescription> TimeDomainAveraging::ALL_ITEMS
{
    {Item::OFF,      {"Off",                      "OFF",        0}},
    {Item::FRAMES_2, {"2x time domain averaging", "FRAMES_2",   1}},
    {Item::FRAMES_4, {"4x time domain averaging", "FRAMES_4",   2}},
};

const std::map<InternalShutterState::Item, EnumValueDeviceDescription> InternalShutterState::ALL_ITEMS
{
    {Item::OPEN,   {"Open",   "OPEN",   0}},
    {Item::CLOSED, {"Closed", "CLOSED", 1}},
};

const std::map<ShutterUpdateMode::Item, EnumValueDeviceDescription> ShutterUpdateMode::ALL_ITEMS
{
    {Item::PERIODIC, {"Periodic", "PERIODIC",   1}},
    {Item::ADAPTIVE, {"Adaptive", "ADAPTIVE",   2}},
};

const std::map<Range::Item, EnumValueDeviceDescription> Range::ALL_ITEMS
{
    {Item::NOT_DEFINED, {"Undefined",            "NOT_DEFINED",  0x0F}},
    {Item::R1,          {"-50 °C ... +160 °C",   "R1",           0x00}},
    {Item::R2,          {"-50 °C ... +600 °C",   "R2",           0x01}},
    {Item::R3,          {"+300 °C ... +1500 °C", "R3",           0x02}},
    {Item::HIGH_GAIN,   {"High gain",            "HIGH_GAIN",    0x07}},
    {Item::LOW_GAIN,    {"Low gain",             "LOW_GAIN",     0x08}},
    {Item::SUPER_GAIN,  {"Super gain",           "SUPER_GAIN",   0x09}},
};

uint32_t Range::getDeviceValue(const Item item)
{
    return ALL_ITEMS.at(item).deviceValue;
}

ValueResult<Range::Item> Range::getFromDeviceValue(const uint32_t deviceValue)
{
    auto maskedDeviceValue = deviceValue & Range::MASK;

    for (auto & description : ALL_ITEMS)
    {
        if (description.second.deviceValue == maskedDeviceValue)
        {
            return description.first;
        }
    }
    return ValueResult<Range::Item>::createError("Value out of range!", utils::format("value: {}", deviceValue));
}

bool Range::isRadiometric(Item item)
{
    switch (item)
    {
        case Item::R1:
        case Item::R2:
        case Item::R3:
            return true;

        default:
            return false;
    }
}

int Range::getLowerTemperature(Item item)
{
    switch (item)
    {
        case Item::R1:
            return -15;

        case Item::R2:
            return 0;

        case Item::R3:
            return 300;

        case Item::LOW_GAIN:
            return -50;

        case Item::HIGH_GAIN:
            return -50;

        case Item::SUPER_GAIN:
            return -50;

        default:
            assert(false);
            return 0;
    }
}

int Range::getUpperTemperature(Item item)
{
    switch (item)
    {
        case Item::R1:
            return 160;

        case Item::R2:
            return 650;

        case Item::R3:
            return 1500;

        case Item::LOW_GAIN:
            return 600;

        case Item::HIGH_GAIN:
            return 160;

        case Item::SUPER_GAIN:
            return 80;

        default:
            assert(false);
            return 0;
    }
}

const std::map<Lens::Item, Lens::Description> Lens::ALL_ITEMS
{
    {Item::NOT_DEFINED, {"Undefined",     "NOT_DEFINED",    0xF0, "Undefined"}},
    {Item::WTC_35,      {"35 mm f/1.10",  "WTC_35",         0x00, "L-WTC-35-{}-{}"}},
    {Item::WTC_25,      {"25 mm f/1.20",  "WTC_25",         0x10, "L-WTC-25-{}-{}"}},
    {Item::WTC_14,      {"14 mm f/1.20",  "WTC_14",         0x20, "L-WTC-14-{}-{}"}},
    {Item::WTC_7_5,     {"7.5 mm f/1.20", "WTC_7_5",        0x30, "L-WTC-7-{}-{}"}},
    {Item::WTC_50,      {"50 mm f/1.20",  "WTC_50",         0x40, "L-WTC-50-{}-{}"}},
    {Item::WTC_7,       {"7 mm f/1.00",   "WTC_7",          0x50, "L-WTC-7-{}-{}"}},
    {Item::USER_1,      {"USER 1",        "USER_1",         0x70, "USER 1"}},
    {Item::USER_2,      {"USER 2",        "USER_2",         0x80, "USER 2"}},
};

uint32_t Lens::getDeviceValue(const Item item)
{
    return ALL_ITEMS.at(item).deviceValue;
}

ValueResult<Lens::Item> Lens::getFromDeviceValue(const uint32_t deviceValue)
{
    auto maskedDeviceValue = deviceValue & Lens::MASK;

    for (auto & description : ALL_ITEMS)
    {
        if (description.second.deviceValue == maskedDeviceValue)
        {
            return description.first;
        }
    }
    return ValueResult<Lens::Item>::createError("Value out of range!", utils::format("value: {}", deviceValue));
}

bool Lens::isUserDefined(Item item)
{
    switch (item)
    {
        case Item::USER_1:
        case Item::USER_2:
            return true;

        default:
            return false;
    }
}

const std::map<LensVariant::Item, EnumValueDeviceDescription> LensVariant::ALL_ITEMS
{
    {Item::NOT_DEFINED, {"Undefined",  "Undefined",  0xFF000000}},
    {Item::A,           {"A",          "A",          0x00000000}},
    {Item::B,           {"B",          "B",          0x01000000}},
    {Item::C,           {"C",          "C",          0x02000000}},
};

uint32_t LensVariant::getDeviceValue(const Item item)
{
    return ALL_ITEMS.at(item).deviceValue << 24;
}

ValueResult<LensVariant::Item> LensVariant::getFromDeviceValue(const uint32_t deviceValue)
{
    auto maskedDeviceValue = deviceValue & LensVariant::MASK;

    for (auto & description : ALL_ITEMS)
    {
        if (description.second.deviceValue == maskedDeviceValue)
        {
            return description.first;
        }
    }
    return ValueResult<LensVariant::Item>::createError("Value out of range!", utils::format("value: {}", deviceValue));
}

const std::map<PresetVersion::Item, EnumValueDeviceDescription> PresetVersion::ALL_ITEMS
{
    {Item::PRESET_VERSION_NOT_DEFINED, {"PRESET_VERSION_NOT_DEFINED", "PRESET_VERSION_NOT_DEFINED", 0xF000}},
    {Item::PRESET_VERSION_WITH_ONUC,   {"ONUC",                       "PRESET_VERSION_WITH_ONUC",   0x0000}},
    {Item::PRESET_VERSION_WITH_SNUC,   {"SNUC",                       "PRESET_VERSION_WITH_SNUC",   0x1000}},
};

uint32_t PresetVersion::getDeviceValue(const Item item)
{
    return ALL_ITEMS.at(item).deviceValue;
}

ValueResult<PresetVersion::Item> PresetVersion::getFromDeviceValue(const uint32_t deviceValue)
{
    auto maskedDeviceValue = deviceValue & PresetVersion::MASK;

    for (auto & description : ALL_ITEMS)
    {
        if (description.second.deviceValue == maskedDeviceValue)
        {
            return description.first;
        }
    }
    return ValueResult<PresetVersion::Item>::createError("Value out of range!", utils::format("value: {}", deviceValue));
}

const std::map<SensorCint::Item, EnumValueDeviceDescription> SensorCint::ALL_ITEMS
{
    {Item::CINT_6_5_GAIN_1_00, {"6.5 pF (1.00x)", "CINT_6_5_GAIN_1_00", 0b101}},
    {Item::CINT_5_5_GAIN_1_18, {"5.5 pF (1.18x)", "CINT_5_5_GAIN_1_18", 0b100}},
    {Item::CINT_4_5_GAIN_1_44, {"4.5 pF (1.44x)", "CINT_4_5_GAIN_1_44", 0b011}},
    {Item::CINT_3_5_GAIN_1_86, {"3.5 pF (1.86x)", "CINT_3_5_GAIN_1_86", 0b010}},
    {Item::CINT_2_5_GAIN_2_60, {"2.5 pF (2.60x)", "CINT_2_5_GAIN_2_60", 0b001}},
    {Item::CINT_1_5_GAIN_4_30, {"1.5 pF (4.30x)", "CINT_1_5_GAIN_4_30", 0b000}},
};

double SensorCint::getRelativeGain(Item item)
{
    switch (item)
    {
        case Item::CINT_6_5_GAIN_1_00:
            return 1.0;

        case Item::CINT_5_5_GAIN_1_18:
            return 1.18;

        case Item::CINT_4_5_GAIN_1_44:
            return 1.44;

        case Item::CINT_3_5_GAIN_1_86:
            return 1.86;

        case Item::CINT_2_5_GAIN_2_60:
            return 2.6;

        case Item::CINT_1_5_GAIN_4_30:
            return 4.3;

        default:
            assert(false);
            return 1.0;
    }
}

double SensorCint::getCintValue(Item item)
{
    switch (item)
    {
        case Item::CINT_6_5_GAIN_1_00:
            return 6.5;

        case Item::CINT_5_5_GAIN_1_18:
            return 5.5;

        case Item::CINT_4_5_GAIN_1_44:
            return 4.5;

        case Item::CINT_3_5_GAIN_1_86:
            return 3.5;

        case Item::CINT_2_5_GAIN_2_60:
            return 2.5;

        case Item::CINT_1_5_GAIN_4_30:
            return 1.5;

        default:
            assert(false);
            return 6.5;
    }
}

const std::map<ImageEqualizationType::Item, EnumValueDeviceDescription> ImageEqualizationType::ALL_ITEMS
{
    {Item::AGC_NH, {"Automatic",      "AUTO_GAIN_CONTROL",      0}},
    {Item::MGC,    {"Manual",         "MANUAL_GAIN_CONTROL",    1}},
};

const std::map<MotorFocusMode::Item, EnumValueDeviceDescription> MotorFocusMode::ALL_ITEMS
{
    {Item::MANUAL_FOCUS,   {"Manual focus",    "MANUAL_FOCUS",  0b001}},
    {Item::REMOTE_FOCUS,   {"Remote focus",    "REMOTE_FOCUS",  0b010}},
    {Item::IFD,            {"IFD",             "IFD",           0b011}},
    {Item::NFD,            {"NFD",             "NFD",           0b100}},
    {Item::MFD,            {"MFD",             "MFD",           0b101}},
};

const std::map<ReticleMode::Item, EnumValueDeviceDescription> ReticleMode::ALL_ITEMS
{
    {Item::DISABLED,   {"Disabled",    "DISABLED",    0}},
    {Item::DARK,       {"Dark",        "DARK",        1}},
    {Item::BRIGHT,     {"Bright",      "BRIGHT",      2}},
    {Item::AUTO,       {"Auto",        "AUTO",        3}},
    {Item::INVERTED,   {"Inverted",    "INVERTED",    4}},
};
} // namespace core
