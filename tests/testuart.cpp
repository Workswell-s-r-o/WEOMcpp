#define BOOST_TEST_MODULE MyTest

#include "testfixture.h"
#include "commontestlogic.h"
#include "core/utils.h"


using UsbUartFixture = PropertiesFixture<core::Plugin::Item::USB, ConnectUartAuto>;
BOOST_FIXTURE_TEST_CASE(UartIntegrationTest, UsbUartFixture)
{
    runCommonIntegrationTest(this);

    BOOST_TEST_MESSAGE("Running UART-specific baudrate test...");
    {
        auto exclusiveTransaction = properties->createConnectionExclusiveTransactionWtc640(false);
        auto& transaction = exclusiveTransaction.getConnectionExclusiveTransaction().getPropertiesTransaction();

        auto availableBaudrates = transaction.getValueToUserNameMap<core::Baudrate::Item>(core::PropertyIdWtc640::UART_BAUDRATE_IN_FLASH);
        BOOST_REQUIRE_MESSAGE(!availableBaudrates.empty(), "Baudrate list is empty");

        auto lowestBaudrate = availableBaudrates.begin()->first;
        auto highestBaudrate = availableBaudrates.rbegin()->first;

        BOOST_TEST_MESSAGE(utils::format("Setting baudrate to highest available: {}", core::BaudrateWtc::ALL_ITEMS.at(highestBaudrate).pythonName));
        auto setResultHigh = exclusiveTransaction.setCoreBaudrate(highestBaudrate);
        BOOST_REQUIRE_MESSAGE(setResultHigh.isOk(), utils::format("Failed to set highest baudrate: {}", setResultHigh.toString()));

        BOOST_TEST_MESSAGE(utils::format("Setting baudrate to lowest available: {}", core::BaudrateWtc::ALL_ITEMS.at(lowestBaudrate).pythonName));
        auto setResultLow = exclusiveTransaction.setCoreBaudrate(lowestBaudrate);
        BOOST_REQUIRE_MESSAGE(setResultLow.isOk(), utils::format("Failed to set lowest baudrate: {}", setResultLow.toString()));
    }

    BOOST_TEST_MESSAGE("Running reset/reconnect stability test...");
    runResetReconnectStabilityTest(this, 10);
}
