cmake_minimum_required(VERSION 3.24)

add_executable(PaletteLoopExample paletteloopexample.cpp paletteloopexample.h portarghelper.h)
add_executable(CaptureFlowExample captureflowexample.cpp captureflowexample.h portarghelper.h)

target_link_libraries(PaletteLoopExample PRIVATE
    ThermalCore::Core
    ThermalCore::WTC640
)
target_link_libraries(CaptureFlowExample PRIVATE
    ThermalCore::Core
    ThermalCore::WTC640
)

target_include_directories(PaletteLoopExample PRIVATE
    ../
)
target_include_directories(CaptureFlowExample PRIVATE
    ../
)

add_custom_target(ExamplesAll ALL
    DEPENDS
        PaletteLoopExample
        CaptureFlowExample
)
