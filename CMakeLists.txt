cmake_minimum_required(VERSION 3.24)
project(wshdbg VERSION 0.1.0 LANGUAGES CXX)

option(WSHDBG_BUILD_TESTS "Build wshdbg tests" ON)
option(WSHDBG_BUILD_WINDOWS_BACKEND "Build Windows Active Scripting backend" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(wshdbg_core
    src/core/breakpoint_store.cpp
    src/core/session.cpp
)
target_include_directories(wshdbg_core PUBLIC include)
target_compile_features(wshdbg_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(wshdbg_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(wshdbg_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND WSHDBG_BUILD_WINDOWS_BACKEND)
    add_library(wshdbg_windows
        src/windows/active_script_host.cpp
        src/windows/com_runtime.cpp
        src/windows/debug_site.cpp
    )
    target_include_directories(wshdbg_windows PUBLIC include PRIVATE src/windows)
    target_link_libraries(wshdbg_windows PUBLIC wshdbg_core ole32 oleaut32 uuid)
    target_compile_definitions(wshdbg_windows PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    if(MSVC)
        target_compile_options(wshdbg_windows PRIVATE /W4 /permissive- /EHsc)
    endif()
endif()

add_executable(wshdbg src/cli/main.cpp)
target_link_libraries(wshdbg PRIVATE wshdbg_core)
if(WIN32 AND WSHDBG_BUILD_WINDOWS_BACKEND)
    target_link_libraries(wshdbg PRIVATE wshdbg_windows)
    target_compile_definitions(wshdbg PRIVATE WSHDBG_HAS_WINDOWS_BACKEND=1 UNICODE _UNICODE)
endif()

if(WSHDBG_BUILD_TESTS)
    enable_testing()
    add_executable(wshdbg_tests
        tests/test_main.cpp
        tests/breakpoint_store_tests.cpp
        tests/session_tests.cpp
    )
    target_link_libraries(wshdbg_tests PRIVATE wshdbg_core)
    add_test(NAME wshdbg_tests COMMAND wshdbg_tests)
endif()
