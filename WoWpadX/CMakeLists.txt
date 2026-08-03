cmake_minimum_required(VERSION 3.24)

project(WoWpadX VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

set(SDL3_ROOT "" CACHE PATH "Path to an SDL3 Visual C++ development package")
find_path(
    SDL3_INCLUDE_DIR
    NAMES SDL3/SDL.h
    HINTS "${SDL3_ROOT}/include"
    REQUIRED
)
find_library(
    SDL3_LIBRARY
    NAMES SDL3
    HINTS "${SDL3_ROOT}/lib/x64"
    REQUIRED
)

add_library(SDL3::SDL3 SHARED IMPORTED)
set_target_properties(
    SDL3::SDL3
    PROPERTIES
        IMPORTED_IMPLIB "${SDL3_LIBRARY}"
        IMPORTED_LOCATION "${SDL3_ROOT}/lib/x64/SDL3.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
)

qt_add_executable(
    WoWpadX
    WIN32
    AppSettings.cpp
    AppSettings.h
    BindManager.cpp
    BindManager.h
    ControllerManager.cpp
    ControllerManager.h
    DevicesController.cpp
    DevicesController.h
    InputDiagnostics.cpp
    InputDiagnostics.h
    InputMapper.cpp
    InputMapper.h
    InputSource.h
    Log.cpp
    Log.h
    main.cpp
    MainPageController.cpp
    MainPageController.h
    MemoryReadingController.cpp
    MemoryReadingController.h
    NativeHidProbe.cpp
    OverlayClient.cpp
    OverlayClient.h
    OverlayLoader.cpp
    OverlayLoader.h
    ProcessManager.cpp
    ProcessManager.h
    RawInputGamepad.cpp
    RawInputGamepad.h
    RawInputHidIncludes.h
    TrayManager.cpp
    TrayManager.h
    WoWReader.cpp
    WoWReader.h
    Gamepad/gamepad.h
    KeybindDefaults.h
    resource.h
    resources.rc
    qml.qrc
)

target_compile_definitions(
    WoWpadX
    PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

target_compile_options(
    WoWpadX
    PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/FI${CMAKE_CURRENT_SOURCE_DIR}/RawInputHidIncludes.h>"
)

target_link_libraries(
    WoWpadX
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        SDL3::SDL3
        hid
        setupapi
)

qt_finalize_executable(WoWpadX)
