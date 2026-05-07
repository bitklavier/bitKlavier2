include(SubprojectVersion)

#include(SetupJuceTest)
include(SetupCatchTest)
include(SetupSFZero)

message(STATUS "Configuring tests for chowdsp_utils")

# set up JUCE
option(CHOWDSP_BUILD_LIVE_GUI_TEST "Build live GUI test app" OFF)
if(NOT CHOWDSP_BUILD_LIVE_GUI_TEST)
    message(STATUS "Live GUI test is not being built... skipping juceaide!")
    set(JUCE_MODULES_ONLY ON CACHE INTERNAL "Only configure the JUCE modules")
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/../JUCE JUCE)
subproject_version(JUCE juce_version)
message(STATUS "VERSION for JUCE: ${juce_version}")

# Set up JUCE modules
juce_add_module(${PROJECT_SOURCE_DIR}/sfzero_juce ALIAS_NAMESPACE sfzero)


# include CPM
include(CPM)
CPMAddPackage("gh:catchorg/Catch2@3.2.1")
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
add_subdirectory(sfzero_tests)

