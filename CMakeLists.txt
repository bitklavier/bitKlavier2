cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)
add_compile_options(-w)
# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "bitKlavier2")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala    JUCE_PLUGINHOST_VST3=1:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugin    JUCE_PLUGINHOST_VST3=
# allowing you to change parameters and behavior without breaking existing user projects
set(PROJECT_NAME "bitKlavier${MAJOR_VERSION}")
set(PRODUCT_NAME "bitKlavier${MAJOR_VERSION}")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "Many Arrows Music")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.manyarrowsmusic.bitklavier")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 AUv3)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)
# Link to any other modules you added (with juce_add_module) here!
message("-- Adding juce modules")
juce_add_module(modules/melatonin_audio_sparklines)

# Add any other modules you want modules here, before the juce_add_plugin call
message("-- Adding external libraries" )
add_subdirectory(third_party/chowdsp_utils)

# Add CLAP format

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    # Icons for the standalone app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    # Change me!
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Change me!
    # A four-character plugin id
    # First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE Bitk 

    # Change me!
    # A unique four-character plugin id
    # Note: this must have at least one upper-case character
    PLUGIN_CODE B202
    FORMATS "${FORMATS}"

    # The name of your final executable
    # This is how it's listed in the DAW
    # This can be different from PROJECT_NAME and can have spaces!
    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE
        source/interface/components/peak_meter_viewer.h
)
# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)
target_include_directories(SharedCode
    INTERFACE
        source/common

        #synthesis includes
        source/synthesis/sound_engine
        source/synthesis
        source/synthesis/framework
        source/synthesis/framework/Processors
        source/synthesis/framework/ParamHolders
        source/synthesis/framework/Synthesiser
        #interface includes
        source/interface
        source/interface/components
        source/interface/components/BKitems
        source/interface/components/Cable
        source/interface/components/opengl
        source/interface/sections
        source/interface/Preparations
        source/interface/ParameterView
        source/interface/look_and_feel
        source/modulation
)

#we were previously linking against the entire tracktion_engine and including
# via git submodules. this was a source of many headaches and we only use a small
# portion of the library so i moved the useful files here and removed the submodule
target_include_directories(SharedCode
        INTERFACE
        third_party/tracktion_engine)


# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE
        #JUCE_USE_CUSTOM_PLUGIN_STANDALONE_ENTRYPOINT=1

        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_OPENGL3=1
        JUCE_PLUGINHOST_VST3=1
    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0
        #JUCE_ENABLE_REPAINT_DEBUGGING=1
    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
        JUCE_MODULE_AVAILABLE_chowdsp_plugin_state=1
    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="bitKlavier2"
)


# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with bitKlavier2, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
set(CHOWDSP_DEPENDENCIES
        chowdsp::chowdsp_plugin_base
        chowdsp::chowdsp_plugin_state
        chowdsp::chowdsp_sources
        chowdsp::chowdsp_plugin_utils
        chowdsp::chowdsp_gui
        chowdsp::chowdsp_dsp_utils
        chowdsp::chowdsp_data_structures
)
set(JUCE_DEPENDENCIES
        juce_audio_utils
        juce_audio_processors
        juce_dsp
        juce_gui_basics
        juce_gui_extra
        juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags

)
target_link_libraries(SharedCode
    INTERFACE
    Assets

    ${CHOWDSP_DEPENDENCIES}
    ${JUCE_DEPENDENCIES}
    melatonin_audio_sparklines
)



include (FetchContent)

#FetchContent_Declare (melatonin_perfetto
#        GIT_REPOSITORY https://github.com/sudara/melatonin_perfetto.git
#        GIT_TAG origin/main)
#
#FetchContent_MakeAvailable (melatonin_perfetto)
# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE
        SharedCode

        #Melatonin::Perfetto
)

#add_custom_target(graphviz ALL
#        COMMAND ${CMAKE_COMMAND} "--graphviz=foo.dot" .
#        COMMAND dot -Tpng foo.dot -o foo.png
#        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
#)
# IPP support, comment out to disable
include(PamplejuceIPP)

## Everything related to the tests target
include(Tests)
#
# A separate target keeps the Tests target fast!
include(Benchmarks)
#
# Pass some config to GA (like our PRODUCT_NAME)
include(GitHubENV)
