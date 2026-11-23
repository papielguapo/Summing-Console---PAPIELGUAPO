cmake_minimum_required(VERSION 3.22)

project(JJ8NLS VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "C:/Users/anton/Desktop/FLResamplingQuality/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("C:/Users/anton/Desktop/FLResamplingQuality/build/_deps/juce-src"
                     "${CMAKE_BINARY_DIR}/juce-build" EXCLUDE_FROM_ALL)
elseif(EXISTS "C:/Users/anton/PitchCanvas/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("C:/Users/anton/PitchCanvas/build/_deps/juce-src"
                     "${CMAKE_BINARY_DIR}/juce-build" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(JJ8NLS
    COMPANY_NAME                "papielguapo"
    BUNDLE_ID                   "com.papielguapo.summingconsole"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Papi
    PLUGIN_CODE                 SmCn
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Summing Console - Papielguapo"
)

juce_generate_juce_header(JJ8NLS)

target_sources(JJ8NLS
    PRIVATE
        Source/DSP/JJ8NLSDSP.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_definitions(JJ8NLS
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(JJ8NLS
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
