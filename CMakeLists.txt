cmake_minimum_required(VERSION 3.22)

project(NickToneprint VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(nick_toneprint_dsp
    src/dsp/ToneprintDSP.cpp
)
target_include_directories(nick_toneprint_dsp PUBLIC src)

add_executable(nick_toneprint_dsp_tests
    tests/ToneprintDSPTest.cpp
)
target_link_libraries(nick_toneprint_dsp_tests PRIVATE nick_toneprint_dsp)
enable_testing()
add_test(NAME nick_toneprint_dsp_tests COMMAND nick_toneprint_dsp_tests)

option(NICK_TONEPRINT_BUILD_PLUGIN "Build the JUCE AU/VST3/Standalone plugin targets" OFF)

if(NICK_TONEPRINT_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)

    add_subdirectory(plugin)
endif()
