cmake_minimum_required(VERSION 3.24)

add_executable(Example example.cpp example.h)

target_link_libraries(Example PRIVATE
    ThermalCore::Core
    ThermalCore::WTC640
)

target_include_directories(Example PRIVATE
    ../
)
