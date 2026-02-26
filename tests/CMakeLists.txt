cmake_minimum_required(VERSION 3.24)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

enable_testing()

set(COMMON_TEST_HEADERS
  testfixture.h
  commontestlogic.h
)

set(COMMON_TEST_SOURCES
  statictestansntriggers.cpp
  propertyidwtc640coverage.cpp
)

add_executable(
  testuart
  testuart.cpp
  ${COMMON_TEST_SOURCES}
  ${COMMON_TEST_HEADERS}
)

target_link_libraries(testuart PRIVATE
  ThermalCore::Core
  ThermalCore::WTC640
  Boost::unit_test_framework
)
add_test(NAME testuart COMMAND testuart)

if(W_CORE_BUILD_WITH_EBUS)
    add_executable(
    testebus
    testebus.cpp
    ${COMMON_TEST_SOURCES}
    ${COMMON_TEST_HEADERS}
    )
    target_link_libraries(testebus PRIVATE
    ThermalCore::Core
    EbusPlugin
    Boost::unit_test_framework
    )
    add_test(NAME testebus COMMAND testebus)
endif()
