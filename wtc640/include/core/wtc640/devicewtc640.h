#ifndef CORE_DEVICEWTC640_H
#define CORE_DEVICEWTC640_H

#include "core/device.h"
#include "core/stream/imagedata.h"
#include "core/wtc640/enumvaluedescription.h"


#include <vector>
#include <string>
#include <map>
#include <set>
#include <array>
#include <optional>


namespace core
{

class DevicesWtc640
{
    DevicesWtc640() = delete;

public:
    static const DeviceType MAIN_USER;
    static const DeviceType LOADER;

    static constexpr unsigned WIDTH = 640;
    static constexpr unsigned HEIGHT = 480;

    static core::Size getSizeInPixels(DeviceType deviceType);
};

class LoginRole
{
public:
    enum class Item
    {
        NONE,
        LOADER,
        USER
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;

    static bool isMainRole(LoginRole::Item loginRole);
};

class StatusWtc640
{
public:
    explicit StatusWtc640() = default;
    explicit StatusWtc640(uint32_t value);

    enum class BayonetState
    {
        UNKNOWN_STATE     = 0b00,
        DISCONNECTED      = 0b01,
        CONNECTED_UNKNOWN = 0b10,
        CONNECTED_KNOWN   = 0b11,
    };

    bool isNucActive() const;
    bool isCameraNotReady() const;
    std::optional<DeviceType> getDeviceType() const;
    bool isMotorfocusBusy() const;
    bool isMotorfocusAvailable() const;
    BayonetState getBayonetState() const;
    bool isMotorfocusRunning() const;
    bool isMotorfocusPositionReached() const;
    bool isAnyTriggerActive() const;

    bool nucRegistersChanged() const;
    bool bolometerRegistersChanged() const;
    bool focusRegistersChanged() const;
    bool presetsRegistersChanged() const;

    std::string toString() const;

    bool operator==(const StatusWtc640& other) const = default;

private:
    uint32_t m_value {0};
};


class BaudrateWtc
{
public:
    using Item = Baudrate::Item;

    static constexpr uint32_t MASK = 0b1111;
    static const std::map<Baudrate::Item, EnumValueDeviceDescription> ALL_ITEMS;
};


class Sensor
{
public:
    enum class Item
    {
        PICO_640,
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;
};


class Core
{
public:
    enum class Item
    {
        RADIOMETRIC,
        NON_RADIOMETRIC
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;
};


class DetectorSensitivity
{
public:
    enum class Item //movitherm.com/knowledgebase/netd-thermal-camera/
    {
        PERFORMANCE_NETD_50MK,
        SUPERIOR_NETD_30MK,
        ULTIMATE_NETD_30MK,
    };

    static const std::map<Item, EnumValueDescription> ALL_ITEMS;
};


class Focus
{
public:
    enum class Item
    {
        MANUAL_H25,
        MANUAL_H34,
        MOTORIC_E25,
        MOTORIC_E34,
        MOTORIC_WITH_BAYONET_B25,
        MOTORIC_WITH_BAYONET_B34
    };

    struct Description final : EnumValueDescription
    {
        std::string shortName;
    };

    static const std::map<Item, Description> ALL_ITEMS;

    static bool isMotoric(Item item);
    static bool isWithBayonet(Item item);
};

class VideoFormat
{
public:
    enum class Item : int
    {
        PRE_IGC,
        POST_IGC,
        POST_COLORING,
    };

    static constexpr uint32_t MASK = 0b11;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class ImageGenerator
{
public:
    enum class Item
    {
        SENSOR,
        ADC_1,
        INTERNAL_DYNAMIC,
    };
    static constexpr uint32_t MASK = 0b111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class Plugin
{
public:
    enum class Item : int
    {
        CMOS,
        HDMI,
        ANALOG,
        USB,
        PLEORA,
        CVBS,
        ONVIF
    };
    static constexpr uint32_t MASK = 0b1111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class FirmwareType
{
public:
    enum class Item : int
    {
        // this enum is persisted
        CMOS_PLEORA = 0,
        HDMI = 1,
        ANALOG = 2,
        USB = 3,
        ALL = 4,
    };
    static constexpr uint32_t MASK = 0b1111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class Framerate
{
public:
    enum class Item
    {
        FPS_8_57,
        FPS_30,
        FPS_60,
    };
    static constexpr uint32_t MASK = 0b11;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;

    static double getFramerateValue(Item item);
};

class TimeDomainAveraging
{
public:
    enum class Item
    {
        OFF,
        FRAMES_2,
        FRAMES_4,
    };
    static constexpr uint32_t MASK = 0b11;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class InternalShutterState
{
public:
    enum class Item
    {
        OPEN,
        CLOSED,
    };
    static constexpr uint32_t MASK = 0b1;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class ShutterUpdateMode
{
public:
    enum class Item
    {
        PERIODIC,
        ADAPTIVE,
    };
    static constexpr uint32_t MASK = 0b11;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class Range final
{
    Range() = default;
public:
    enum class Item
    {
        NOT_DEFINED,
        R1,
        R2,
        R3,
        HIGH_GAIN,
        LOW_GAIN,
        SUPER_GAIN,
    };
    static constexpr uint32_t MASK = 0b1111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;

    static uint32_t getDeviceValue(const Item item);
    static ValueResult<Item> getFromDeviceValue(const uint32_t deviceValue);

    static bool isRadiometric(Item item);
    static int getLowerTemperature(Item item);
    static int getUpperTemperature(Item item);
};

class Lens final
{
    Lens() = default;
public:
    enum class Item
    {
        NOT_DEFINED,
        WTC_35,
        WTC_25,
        WTC_14,
        WTC_7_5,
        WTC_50,
        WTC_7,
        USER_1,
        USER_2,
    };

    struct Description final : EnumValueDeviceDescription
    {
        std::string articleNumberTemplate;
    };

    static constexpr uint32_t MASK = 0b1111'0000;
    static const std::map<Item, Description> ALL_ITEMS;

    static uint32_t getDeviceValue(const Item item);
    static ValueResult<Item> getFromDeviceValue(const uint32_t deviceValue);

    static bool isUserDefined(Item item);
};

class LensVariant final
{
    LensVariant() = default;
public:
    enum class Item
    {
        NOT_DEFINED,
        A,
        B,
        C,
    };

    static constexpr uint32_t MASK = 0xFF000000;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;

    static uint32_t getDeviceValue(const Item item);
    static ValueResult<Item> getFromDeviceValue(const uint32_t deviceValue);
};

class PresetVersion final
{
    PresetVersion() = default;
public:
    enum class Item
    {
        PRESET_VERSION_NOT_DEFINED,
        PRESET_VERSION_WITH_SNUC,
        PRESET_VERSION_WITH_ONUC,
    };

    static constexpr uint32_t MASK = 0xF000;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;

    static uint32_t getDeviceValue(const Item item);
    static ValueResult<Item> getFromDeviceValue(const uint32_t deviceValue);
};

class SensorCint
{
public:
    enum class Item : uint8_t
    {
        CINT_6_5_GAIN_1_00,
        CINT_5_5_GAIN_1_18,
        CINT_4_5_GAIN_1_44,
        CINT_3_5_GAIN_1_86,
        CINT_2_5_GAIN_2_60,
        CINT_1_5_GAIN_4_30
    };

    static constexpr uint32_t MASK = 0b111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;

    static double getRelativeGain(Item item);
    static double getCintValue(Item item);
};

class ImageEqualizationType
{
public:
    enum class Item : uint8_t
    {
        AGC_NH,
        MGC,
    };

    static constexpr uint32_t MASK = 0b1;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class MotorFocusMode
{
public:
    enum class Item : int
    {
        MANUAL_FOCUS,
        REMOTE_FOCUS,
        IFD,
        NFD,
        MFD,
    };

    static constexpr uint32_t MASK = 0b111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};

class ReticleMode final
{
public:
    enum class Item : int
    {
        DISABLED,
        DARK,
        BRIGHT,
        AUTO,
        INVERTED,
    };

    static constexpr uint32_t MASK = 0b111;
    static const std::map<Item, EnumValueDeviceDescription> ALL_ITEMS;
};
} // namespace core

#endif // CORE_DEVICEWTC640_H
