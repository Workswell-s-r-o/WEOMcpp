cmake_minimum_required(VERSION 3.24)

find_package(Pleora REQUIRED)

add_library(EbusPlugin SHARED
    datalinkebus.cpp datalinkebus.h
    ebusplugin.cpp ebusplugin.h
)

target_include_directories(EbusPlugin PUBLIC ${Pleora_INCLUDE_DIR})

target_link_libraries(EbusPlugin PRIVATE
    ${Pleora_LIBRARIES}
    ThermalCore::Core
)

# boost/dll export macro for creating the plugin
target_compile_definitions(EbusPlugin PRIVATE BOOST_DLL_EXPORT)

if(WIN_32)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND W_TCG_DEBUG_SYMBOLS_IN_RELEASE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O1 /Ob0 /Oy-")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi /O1 /Ob0 /Oy-")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
    endif()
endif()
