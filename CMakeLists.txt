cmake_minimum_required(VERSION 3.22)

project(Audiosphere VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

# Link the C runtime statically on Windows so the plugin works without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------------------
# JUCE: pass -DJUCE_DIR=/path/to/JUCE to use a local checkout, otherwise it
# is downloaded automatically.
# ---------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(AUDIOSPHERE_COPY_PLUGIN "Copy the plugin to the system plugin folder after building" OFF)
option(AUDIOSPHERE_BUILD_RENDER_TOOL "Build the offline preset render/test tool" OFF)

set(AUDIOSPHERE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND AUDIOSPHERE_FORMATS AU)
endif()

juce_add_plugin(Audiosphere
    COMPANY_NAME "Audiosphere"
    COMPANY_WEBSITE "https://github.com/bobanes1/audiosphere_plugin"
    BUNDLE_ID "com.audiosphere.synth"
    PLUGIN_MANUFACTURER_CODE Adsp
    PLUGIN_CODE Asph
    PRODUCT_NAME "Audiosphere"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    DESCRIPTION "Wavetable synthesizer for hip hop production"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    NEEDS_CURL TRUE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD ${AUDIOSPHERE_COPY_PLUGIN}
    FORMATS ${AUDIOSPHERE_FORMATS})

set(AUDIOSPHERE_CORE_SOURCES
    Source/PluginProcessor.cpp
    Source/AppSettings.cpp
    Source/Parameters.cpp
    Source/DSP/Wavetables.cpp
    Source/DSP/SynthEngine.cpp
    Source/DSP/Effects.cpp
    Source/Presets/FactoryPresets.cpp
    Source/Presets/PresetManager.cpp)

target_sources(Audiosphere PRIVATE
    ${AUDIOSPHERE_CORE_SOURCES}
    Source/PluginEditor.cpp
    Source/GUI/LookAndFeel.cpp
    Source/GUI/Components.cpp
    Source/GUI/UpdateUI.cpp
    Source/Update/UpdateManager.cpp)

target_include_directories(Audiosphere PRIVATE Source)

target_compile_definitions(Audiosphere PUBLIC
    AUDIOSPHERE_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Audiosphere
    PRIVATE
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Offline render tool: renders every factory preset to WAV and prints levels.
# Used to sanity-check and level-match presets without a DAW.
# ---------------------------------------------------------------------------
if(AUDIOSPHERE_BUILD_RENDER_TOOL)
    juce_add_console_app(AudiosphereRender PRODUCT_NAME "AudiosphereRender")
    target_sources(AudiosphereRender PRIVATE tools/RenderPresets.cpp ${AUDIOSPHERE_CORE_SOURCES})
    target_include_directories(AudiosphereRender PRIVATE Source)
    target_compile_definitions(AudiosphereRender PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        AUDIOSPHERE_HEADLESS=1)
    target_link_libraries(AudiosphereRender
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
