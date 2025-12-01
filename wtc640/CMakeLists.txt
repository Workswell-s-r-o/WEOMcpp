cmake_minimum_required(VERSION 3.24)

set(HEADERS
    include/core/wtc640/deadpixels.h
    include/core/wtc640/devicewtc640.h
    include/core/wtc640/deviceinterfacewtc640.h
    include/core/wtc640/enumvaluedescription.h
    include/core/wtc640/hungariandeadpixels.h
    include/core/wtc640/firmwarewtc640.h
    include/core/wtc640/memoryspacewtc640.h
    include/core/wtc640/propertieswtc640.h
    include/core/wtc640/propertyadapterlensrange.h
    include/core/wtc640/propertyidwtc640.h
    include/core/wtc640/videoformatadapter.h
    include/core/wtc640/palettesmanager.h
)

set(SOURCES
    source/deadpixels.cpp
    source/devicewtc640.cpp
    source/deviceinterfacewtc640.cpp
    source/hungariandeadpixels.cpp
    source/firmwarewtc640.cpp
    source/memoryspacewtc640.cpp
    source/propertieswtc640.cpp
    source/propertyadapterlensrange.cpp
    source/propertyidwtc640.cpp
    source/videoformatadapter.cpp
    source/palettesmanager.cpp
    
)



add_library(WTC640 STATIC
    ${HEADERS}
    ${SOURCES}
)

target_compile_features(WTC640 PUBLIC cxx_std_20)
set_property(TARGET WTC640 PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package(OpenSSL REQUIRED)

target_link_libraries(WTC640 PRIVATE
    ThermalCore::Core
)


if (W_BUILD_DEPENDENCIES_WITH_CONAN)
    find_package(libzip CONFIG REQUIRED)
    target_link_libraries(WTC640 PRIVATE
        libzip::zip
    )
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)
    target_link_libraries(Core PUBLIC PkgConfig::libzip)
endif()


if (W_BUILD_DEPENDENCIES_WITH_CONAN)
    target_link_libraries(WTC640 PRIVATE openssl::openssl)
else ()
    target_link_libraries(WTC640 PRIVATE OpenSSL::SSL)
endif ()


target_include_directories(WTC640 PUBLIC include)

add_library(ThermalCore::WTC640 ALIAS WTC640)
