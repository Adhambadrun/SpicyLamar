cmake_minimum_required(VERSION 3.20)
project(SpicyLamar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This CMakeLists lives in build/, so source paths are relative to ../ (repo root).
add_executable(BluetoothDevices WIN32
    ../src/main.cpp
    ../resources/app.rc
)

target_compile_definitions(BluetoothDevices PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SPICY_LAMAR_QUANTUM
)

target_compile_options(BluetoothDevices PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(BluetoothDevices PRIVATE
    kernel32
    user32
    gdi32
    comctl32
    shell32
    ole32
    oleaut32
    advapi32
    uxtheme
    winmm
    avrt
    dwmapi
    uiautomationcore
    oleacc
    tdh
    psapi
)

# Static linking
if(MSVC)
    set_target_properties(BluetoothDevices PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

add_executable(benchmark
    ../tests/benchmark.cpp
)

target_compile_definitions(benchmark PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SPICY_LAMAR_QUANTUM
    BENCHMARK
)

target_compile_options(benchmark PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(benchmark PRIVATE
    kernel32
    user32
    gdi32
    comctl32
    shell32
    ole32
    oleaut32
    advapi32
    uxtheme
    winmm
    avrt
    dwmapi
    uiautomationcore
    oleacc
    tdh
    psapi
)
