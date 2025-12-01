cmake_minimum_required(VERSION 3.24)

option(W_CORE_RESULT_STRING_WITH_DETAIL "Result::toString() returns full error info including detailErrorMessage" OFF)

set(HEADERS
    include/core/connection/addressrange.h
    include/core/connection/asiodatalinkwithbaudrateandstreamsource.h
    include/core/connection/datalinkuart.h
    include/core/connection/deviceutils.h
    include/core/connection/idatalinkinterface.h
    include/core/connection/ideviceinterface.h
    include/core/connection/iprotocolinterface.h
    include/core/connection/protocolinterfacetcsi.h
    include/core/connection/resultdeviceinfo.h
    include/core/connection/stats.h
    include/core/connection/status.h
    include/core/connection/tcsipacket.h

    include/core/misc/buffereddatareader.h
    include/core/misc/conversions.h
    include/core/misc/deadlockdetectionmutex.h
    include/core/misc/elapsedtimer.h
    include/core/misc/futurewatcher.h

    include/core/misc/progresscontroller.h
    include/core/misc/result.h
    include/core/misc/resultmacros.h
    include/core/misc/verify.h

    include/core/properties/itaskmanager.h
    include/core/properties/properties.h
    include/core/properties/properties.inl
    include/core/properties/propertyadapterbase.h
    include/core/properties/propertyadaptervalue.h
    include/core/properties/valueadapterusbstringdescriptor.h
    include/core/properties/propertyadaptervaluecomponent.h
    include/core/properties/propertyadaptervaluederived.h
    include/core/properties/propertyadaptervaluederivedfrom1.h
    include/core/properties/propertyadaptervaluederivedfrom2.h
    include/core/properties/propertyadaptervaluederivedfrom3.h
    include/core/properties/propertyadaptervaluedevice.h
    include/core/properties/propertyadaptervaluedevicecomposite.h
    include/core/properties/propertyadaptervaluedeviceprogress.h
    include/core/properties/propertyadaptervaluedevicesimple.h
    include/core/properties/propertydependencyvalidator.h
    include/core/properties/propertyid.h
    include/core/properties/propertydependencyvalidatorfor2.h
    include/core/properties/propertyvalue.h
    include/core/properties/propertyvaluearithmetic.h
    include/core/properties/propertyvaluebase.h
    include/core/properties/propertyvalueconversion.h
    include/core/properties/propertyvalueenum.h
    include/core/properties/propertyvalues.h
    include/core/properties/rankedvalidationresult.h
    include/core/properties/taskmanagerdirect.h
    include/core/properties/taskmanagerqueued.h

    include/core/device.h
    include/core/execution.h
    include/core/utils.h
    include/core/logging.h
    include/core/prtutils.h
)

set(SOURCES
    source/connection/addressrange.cpp
    source/connection/asiodatalinkwithbaudrateandstreamsource.cpp
    source/connection/datalinkuart.cpp
    source/connection/deviceutils.cpp
    source/connection/ideviceinterface.cpp
    source/connection/protocolinterfacetcsi.cpp
    source/connection/stats.cpp
    source/connection/status.cpp
    source/connection/tcsipacket.cpp

    source/misc/buffereddatareader.cpp
    source/misc/conversions.cpp
    source/misc/deadlockdetectionmutex.cpp
    source/misc/elapsedtimer.cpp

    source/misc/progresscontroller.cpp
    source/misc/result.cpp

    source/properties/itaskmanager.cpp
    source/properties/properties.cpp
    source/properties/propertyadapterbase.cpp
    source/properties/propertydependencyvalidator.cpp
    source/properties/propertyid.cpp
    source/properties/propertyvaluebase.cpp
    source/properties/propertyvalues.cpp
    source/properties/valueadapterusbstringdescriptor.cpp
    source/properties/rankedvalidationresult.cpp
    source/properties/taskmanagerdirect.cpp
    source/properties/taskmanagerqueued.cpp

    source/device.cpp
    source/logging.cpp

)
if(APPLE)
    list(APPEND SOURCES source/connection/deviceutils.mm)
endif()

if(WIN32)
    list(APPEND SOURCES source/properties/valueadapterusbstringdescriptorwin.cpp)
elseif(UNIX OR APPLE)
    list(APPEND SOURCES source/properties/valueadapterusbstringdescriptorunix.cpp)
else()
    list(APPEND SOURCES source/properties/valueadapterusbstringdescriptordummy.cpp)
endif()


add_library(Core STATIC
    ${HEADERS}
    ${SOURCES}
    include/core/connection/idatalinkwithbaudrate.h

    include/core/misc/lifetimechecker.h source/misc/lifetimechecker.cpp

    include/core/properties/transactionchanges.h source/properties/transactionchanges.cpp
    include/core/properties/transactionsummary.h source/properties/transactionsummary.cpp

    include/core/stream/idatalinkwithbaudrateandstreamsource.h
    include/core/stream/imagedata.h
    include/core/stream/istream.h
    include/core/stream/istreamsource.h

    include/core/connection/iebusplugin.h
    include/core/misc/palette.h source/misc/palette.cpp
    include/core/misc/imagecolorization.h source/misc/imagecolorization.cpp
    include/core/connection/serialportinfo.h
    include/core/misc/imainthreadindicator.h
)

target_compile_features(Core PUBLIC cxx_std_20)
set_property(TARGET Core PROPERTY POSITION_INDEPENDENT_CODE ON)

if(W_CORE_RESULT_STRING_WITH_DETAIL)
    target_compile_definitions(Core PRIVATE RESULT_STRING_WITH_DETAIL)
endif()

if(MSVC)
    target_compile_options(Core PUBLIC /bigobj)
    target_compile_definitions(Core PRIVATE NOMINMAX)
elseif(MINGW)
    target_compile_options(Core PUBLIC -Wa,-mbig-obj)

    if(WIN32)
        target_link_libraries(Core PUBLIC ws2_32)
    endif()
endif()


find_package(Boost REQUIRED COMPONENTS log log_setup thread filesystem system json)

if (W_BUILD_DEPENDENCIES_WITH_CONAN)
    find_package(ffmpeg REQUIRED)
    target_link_libraries(Core PUBLIC ffmpeg::ffmpeg boost::boost)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libav REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
    )
    target_link_libraries(Core PUBLIC PkgConfig::libav ${Boost_LIBRARIES})
endif()



if(WIN32)
    target_link_libraries(Core PRIVATE Setupapi Winusb)
elseif(UNIX OR APPLE)
    if(NOT CMAKE_CROSSCOMPILING)
        if (W_BUILD_DEPENDENCIES_WITH_CONAN)
            find_package(libusb REQUIRED)
            target_link_libraries(Core PRIVATE libusb::libusb)
        else()
            pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
            target_link_libraries(Core PRIVATE ${LIBUSB_LIBRARIES})
        endif()
    endif()
endif()

target_include_directories(Core PUBLIC include)

add_library(ThermalCore::Core ALIAS Core)
