#pragma once

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include "core/properties/properties.inl"
#include "core/wtc640/propertieswtc640.h"
#include "core/logging.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/connection/serialportinfo.h"
#include "core/connection/iebusplugin.h"
#include "core/misc/imainthreadindicator.h"

class MainThreadIndicatorMock : public core::IMainThreadIndicator
{
public:
    [[nodiscard]] bool isInGuiThread() override
    {
        return true;
    }
};

struct ConnectUartAuto
{
    static auto connect(core::PropertiesWtc640::ConnectionStateTransaction& transaction, std::shared_ptr<core::PropertiesWtc640> properties)
    {
        std::vector<core::connection::SerialPortInfo> ports;
        auto& masterTestSuite = boost::unit_test::framework::master_test_suite();
        if (masterTestSuite.argc == 3)
        {
            core::connection::SerialPortInfo portInfo;
            portInfo.serialNumber = masterTestSuite.argv[1];
            portInfo.systemLocation = masterTestSuite.argv[2];
            ports.push_back(portInfo);
        }
        else
        {
            BOOST_TEST_ERROR("Usage: " << masterTestSuite.argv[0] << " <serial_number> <system_location>");
        }

        core::ProgressController progressController;
        return transaction.connectUartAuto(ports, progressController);
    }
};

struct ConnectEbus
{
    static auto connect(core::PropertiesWtc640::ConnectionStateTransaction& transaction, std::shared_ptr<core::PropertiesWtc640> properties)
    {
        BOOST_TEST_CHECK(properties->getEbusPlugin());
        core::connection::EbusDevice device;
        return transaction.connectEbus(device);
    }
};

template <core::Plugin::Item PluginType, typename ConnectionPolicy>
struct PropertiesFixture
{
    std::shared_ptr<core::PropertiesWtc640> properties;

    PropertiesFixture()
    {
        {
            auto logLevel = logging::ChannelFilters();
            logging::initLogging(&logLevel);
            logLevel.set(logging::CORE_CONNECTION_CHANNEL_NAME, logging::severityLevel::critical);
            logLevel.set(logging::CORE_PROPERTIES_CHANNEL_NAME, logging::severityLevel::critical);

            auto mainThreadIndicator = std::make_shared<MainThreadIndicatorMock>();
            properties = core::PropertiesWtc640::createInstance(core::Properties::Mode::SYNC_DIRECT, mainThreadIndicator, nullptr);

            auto connectionStateTransaction = properties->createConnectionStateTransaction();

            auto result = ConnectionPolicy::connect(connectionStateTransaction, properties);

            if (!result.isOk())
            {
                BOOST_TEST_MESSAGE("Connection failed: " << result.toString());
            }
        }

        const auto transaction = properties->createConnectionExclusiveTransactionWtc640(false);

        auto pluginTypeResult = transaction.getConnectionExclusiveTransaction().getPropertiesTransaction().getValue<core::Plugin::Item>(core::PropertyIdWtc640::PLUGIN_TYPE);

        if (pluginTypeResult.getResult().isOk())
        {
            BOOST_CHECK_EQUAL(static_cast<int>(pluginTypeResult.getValue()), static_cast<int>(PluginType));
        }
    }

    ~PropertiesFixture()
    {
        properties->createConnectionStateTransaction().disconnectCore();
    }
};
