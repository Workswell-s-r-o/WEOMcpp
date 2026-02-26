#define BOOST_TEST_MODULE EbusTest

#include "testfixture.h"
#include "commontestlogic.h"


using EbusFixture = PropertiesFixture<core::Plugin::Item::PLEORA, ConnectEbus>;
BOOST_FIXTURE_TEST_CASE(EbusIntegrationTest, EbusFixture)
{
    runCommonIntegrationTest(this);
}