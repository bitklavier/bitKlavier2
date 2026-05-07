setup_juce_lib(plugin_tests_lib
        juce::juce_audio_basics
        juce::juce_graphics
        sfzero::sfzero_juce
)

#setup_sfzero_lib( my_lib MODULES sfzero_juce)
add_subdirectory(sf_tests)