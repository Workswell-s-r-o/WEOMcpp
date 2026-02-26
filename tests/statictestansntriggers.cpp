#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <ostream>

#include "core/wtc640/propertieswtc640.h"

namespace core
{
    template<typename EnumType, typename MapType>
    std::ostream& writeEnumOrUnknown(std::ostream& os, const EnumType& item, const MapType& allItems)
    {
        const auto it = allItems.find(item);
        if (it != allItems.end())
        {
            return os << it->second.userName;
        }
        return os << "UNKNOWN(" << static_cast<int>(item) << ")";
    }

    std::ostream& operator<<(std::ostream& os, const core::CommonTrigger::Item& item)
    {
        return writeEnumOrUnknown(os, item, core::CommonTrigger::ALL_ITEMS);
    }

    std::ostream& operator<<(std::ostream& os, const core::ResetTrigger::Item& item)
    {
        return writeEnumOrUnknown(os, item, core::ResetTrigger::ALL_ITEMS);
    }

    std::ostream& operator<<(std::ostream& os, const core::VideoFormat::Item& item)
    {
        return writeEnumOrUnknown(os, item, core::VideoFormat::ALL_ITEMS);
    }

    std::ostream& operator<<(std::ostream& os, const core::Plugin::Item& item)
    {
        return writeEnumOrUnknown(os, item, core::Plugin::ALL_ITEMS);
    }
}

namespace std
{
    template<typename T, typename U>
    ostream& operator<<(ostream& os, const pair<T, U>& p)
    {
        return os << "[" << p.first << ", " << p.second << "]";
    }
}

BOOST_AUTO_TEST_SUITE(PropertiesWtc640TestSuite)

namespace data
{
    const std::vector<std::string>& validArticleNumbers()
    {
        static const std::vector<std::string> data =
        {
            "WTC640-N-P-H25-9",
            "WTC640-R-S-B34-60",
        };
        return data;
    }

    const std::vector<std::string>& invalidArticleNumbers()
    {
        static const std::vector<std::string> data =
        {
            "WTC640-N-P-H25-9-",
            "WTC640-N-P-H25",
            "WTC640-N-P-H25-",
            "WTC640-N-P-H25-9-",
            "WTC640-N-P-H25-9-EXTRA",
            "INVALID-ARTICLE-NUMBER",
        };
        return data;
    }

    const std::map<std::string, boost::posix_time::ptime>& validSerialNumbers()
    {
        static const std::map<std::string, boost::posix_time::ptime> data =
        {
            {"2101W001", {boost::gregorian::date(2021, 1, 4),   boost::posix_time::time_duration(0,0,0)}},
            {"2152W001", {boost::gregorian::date(2021, 12, 27), boost::posix_time::time_duration(0,0,0)}},
            {"9952W999", {boost::gregorian::date(2099, 12, 27), boost::posix_time::time_duration(0,0,0)}},
        };
        return data;
    }

    const std::vector<std::string>& invalidSerialNumbers()
    {
        static const std::vector<std::string> data =
        {
            "2100W001", // week 0
            "2153W001", // week 53
            "2101M001", // invalid separator
            "2101W000", // invalid serial
            "2101W1000",// invalid serial
        };
        return data;
    }

    const std::map<core::CommonTrigger::Item, uint32_t>& commonTriggerMasks()
    {
        static const std::map<core::CommonTrigger::Item, uint32_t> data =
        {
            {core::CommonTrigger::Item::NUC_OFFSET_UPDATE,      1 << 2},
            {core::CommonTrigger::Item::CLEAN_USER_DP,          1 << 3},
            {core::CommonTrigger::Item::SET_SELECTED_PRESET,    1 << 4},
            {core::CommonTrigger::Item::MOTORFOCUS_CALIBRATION, 1 << 5},
            {core::CommonTrigger::Item::FRAME_CAPTURE_START,    1 << 6},
        };
        return data;
    }

    enum class TestDeviceType { MAIN, LOADER };

    std::ostream& operator<<(std::ostream& os, const TestDeviceType& type)
    {
        switch(type)
        {
            case TestDeviceType::MAIN:
                os << "MAIN";
                break;
            case TestDeviceType::LOADER:
                os << "LOADER";
                break;
        }
        return os;
    }

    const std::vector<std::tuple<core::ResetTrigger::Item, TestDeviceType, uint32_t>>& resetTriggerMasks()
    {
        static const std::vector<std::tuple<core::ResetTrigger::Item, TestDeviceType, uint32_t>> data =
        {
            {core::ResetTrigger::Item::RESET_FROM_LOADER,        TestDeviceType::LOADER,    1 << 0},
            {core::ResetTrigger::Item::STAY_IN_LOADER,           TestDeviceType::LOADER,    1 << 1},
            {core::ResetTrigger::Item::SOFTWARE_RESET,           TestDeviceType::MAIN,      1 << 0},
            {core::ResetTrigger::Item::RESET_TO_LOADER,          TestDeviceType::MAIN,      1 << 1},
            {core::ResetTrigger::Item::RESET_TO_FACTORY_DEFAULT, TestDeviceType::MAIN,      1 << 7},
            {core::ResetTrigger::Item::RESET_TO_LOADER,          TestDeviceType::LOADER,    1 << 1},
        };
        return data;
    }

    const std::vector<std::tuple<core::Plugin::Item, core::VideoFormat::Item, bool>>& videoFormatValidity()
    {
        static const std::vector<std::tuple<core::Plugin::Item, core::VideoFormat::Item, bool>> data =
        {
            {core::Plugin::Item::USB,    core::VideoFormat::Item::PRE_IGC,        true},
            {core::Plugin::Item::USB,    core::VideoFormat::Item::POST_IGC,       false},
            {core::Plugin::Item::USB,    core::VideoFormat::Item::POST_COLORING,  true},
            {core::Plugin::Item::PLEORA, core::VideoFormat::Item::PRE_IGC,        true},
            {core::Plugin::Item::PLEORA, core::VideoFormat::Item::POST_IGC,       true},
            {core::Plugin::Item::PLEORA, core::VideoFormat::Item::POST_COLORING,  false},
            {core::Plugin::Item::CMOS,   core::VideoFormat::Item::PRE_IGC,        true},
            {core::Plugin::Item::CMOS,   core::VideoFormat::Item::POST_IGC,       true},
            {core::Plugin::Item::CMOS,   core::VideoFormat::Item::POST_COLORING,  false},
            {core::Plugin::Item::CVBS,   core::VideoFormat::Item::PRE_IGC,        false},
            {core::Plugin::Item::CVBS,   core::VideoFormat::Item::POST_IGC,       false},
            {core::Plugin::Item::CVBS,   core::VideoFormat::Item::POST_COLORING,  true},
            {core::Plugin::Item::HDMI,   core::VideoFormat::Item::PRE_IGC,        false},
            {core::Plugin::Item::HDMI,   core::VideoFormat::Item::POST_IGC,       false},
            {core::Plugin::Item::HDMI,   core::VideoFormat::Item::POST_COLORING,  true},
        };
        return data;
    }
}

BOOST_DATA_TEST_CASE(ArticleNumberCreateFromStringValid, boost::unit_test::data::make(data::validArticleNumbers()), articleNumberString)
{
    auto articleNumberResult = core::PropertiesWtc640::ArticleNumber::createFromString(articleNumberString);
    BOOST_TEST(articleNumberResult.isOk());
    BOOST_TEST(articleNumberResult.getValue().isAllComponentsOk());
    auto toStringResult = articleNumberResult.getValue().toString();
    BOOST_TEST(toStringResult.isOk());
    BOOST_TEST(toStringResult.getValue() == articleNumberString);
}

BOOST_DATA_TEST_CASE(ArticleNumberCreateFromStringInvalid, boost::unit_test::data::make(data::invalidArticleNumbers()), articleNumberString)
{
    auto articleNumberResult = core::PropertiesWtc640::ArticleNumber::createFromString(articleNumberString);
    BOOST_TEST(!articleNumberResult.isOk());
}

BOOST_DATA_TEST_CASE(CommonTriggerGetAddressRange, boost::unit_test::data::make(data::commonTriggerMasks()), data_set)
{
    auto rangeResult = core::CommonTrigger::getAddressRange(data_set.first, core::DevicesWtc640::MAIN_USER);
    BOOST_TEST(rangeResult.isOk());
    BOOST_TEST(rangeResult.getValue().getSize() == 4);
    BOOST_TEST(core::CommonTrigger::getMask(data_set.first) == data_set.second);
}

BOOST_DATA_TEST_CASE(ResetTriggerGetAddressRange, boost::unit_test::data::make(data::resetTriggerMasks()), trigger, testDeviceType, mask)
{
    core::DeviceType deviceType = (testDeviceType == data::TestDeviceType::MAIN) ? core::DevicesWtc640::MAIN_USER : core::DevicesWtc640::LOADER;
    auto rangeResult = core::ResetTrigger::getAddressRange(trigger, deviceType);
    BOOST_TEST(rangeResult.isOk());
    BOOST_TEST(rangeResult.getValue().getSize() == 4);
    BOOST_TEST(core::ResetTrigger::getMask(trigger) == mask);
}

BOOST_DATA_TEST_CASE(IsValidVideoFormat, boost::unit_test::data::make(data::videoFormatValidity()), plugin, format, isValid)
{
    BOOST_TEST(core::PropertiesWtc640::isValidVideoFormat(plugin, format) == isValid);
}

BOOST_AUTO_TEST_CASE(AllCommonTriggersAreTested)
{
    for (const auto& item : core::CommonTrigger::ALL_ITEMS)
    {
        BOOST_TEST(data::commonTriggerMasks().count(item.first) == 1, "Common trigger " << item.first << " is not tested");
    }
}

BOOST_AUTO_TEST_CASE(AllResetTriggersAreTested)
{
    for (const auto& item : core::ResetTrigger::ALL_ITEMS)
    {
        bool found = false;
        for (const auto& testData : data::resetTriggerMasks())
        {
            if (std::get<0>(testData) == item.first)
            {
                found = true;
                break;
            }
        }
        BOOST_TEST(found, "Reset trigger " << item.first << " is not tested");
    }
}

BOOST_AUTO_TEST_SUITE_END()
