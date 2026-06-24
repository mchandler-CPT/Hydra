cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# 1. PROJECT CONFIGURATION
# ==============================================================================
# Change "Boutique Template" to your new project name when cloning
project(Hydra VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
include(CTest)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal Binary support" FORCE)
endif()

# ==============================================================================
# 2. JUCE FRAMEWORK LOCATION
# ==============================================================================
# Keeps your exact local submodule path standard
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Juce" CACHE PATH "Path to JUCE")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Check submodules!")
endif()
add_subdirectory("${JUCE_PATH}" JUCE)

# Determine platform-specific build formats
set(BOUTIQUE_PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND BOUTIQUE_PLUGIN_FORMATS AU)
endif()

# ==============================================================================
# 3. JUCE PLUGIN DECLARATION
# ==============================================================================
juce_add_plugin(Hydra
    COMPANY_NAME "bdEnergy"
    PLUGIN_MANUFACTURER "bdEnergy"
    PLUGIN_MANUFACTURER_CODE "BDEN"
    BUNDLE_ID "com.synth.hydra"
    FORMATS ${BOUTIQUE_PLUGIN_FORMATS}
    
    # --------------------------------------------------------------------------
    # EFFECT VS SYNTH TOGGLES (FLIP THESE FOR AUDIO EFFECTS)
    # --------------------------------------------------------------------------
    # For a Synth:  IS_SYNTH TRUE  | NEEDS_MIDI_INPUT TRUE
    # For an Effect: IS_SYNTH FALSE | NEEDS_MIDI_INPUT FALSE (unless MIDI-controlled)
    IS_SYNTH TRUE                 
    NEEDS_MIDI_INPUT TRUE         
    # --------------------------------------------------------------------------
    
    PLUGIN_CODE "HYDR"             # Change this 4-char ID for every new project!
    PLUGIN_NAME "Hydra"
    PRODUCT_NAME "Hydra"
    DESCRIPTION "Hydra"
)

# Apple-specific plugin property mapping
if(APPLE)
    set_target_properties(Hydra PROPERTIES 
        JUCE_AU_MANUFACTURER_CODE "BDEN"
        JUCE_AU_PLUGIN_CODE "HYDR" # Must match PLUGIN_CODE above
        JUCE_PLUGIN_CODE "HYDR"
        JUCE_MANUFACTURER_CODE "BDEN"
    )
endif()

set_target_properties(Hydra PROPERTIES
    JUCE_PLUGIN_CODE "HYDR"
    JUCE_MANUFACTURER_CODE "BDEN"
    
    # --------------------------------------------------------------------------
    # AUDIO UNIT TYPE TOGGLE (FLIP THIS FOR AUDIO EFFECTS)
    # --------------------------------------------------------------------------
    # For a Synth:  kAudioUnitType_MusicDevice
    # For an Effect: kAudioUnitType_Effect
    JUCE_AU_MAIN_TYPE kAudioUnitType_MusicDevice
    # --------------------------------------------------------------------------
)

juce_generate_juce_header(Hydra)

juce_add_binary_data(HydraAssets
    SOURCES
        assets/logo.png
)

# ==============================================================================
# 4. SOURCE FILE COMPILATION
# ==============================================================================
target_sources(Hydra PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PresetManager.cpp
    Source/PresetManager.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/HydraPalette.h
    Source/UI/HydraPanelBackground.h
    Source/UI/HydraPanelBackground.cpp
    Source/UI/ProceduralDarkLookAndFeel.h
    Source/UI/ProceduralDarkLookAndFeel.cpp
    Source/UI/SnapToggleButton.h
    Source/UI/SnapToggleButton.cpp
    Source/UI/ChromeTextButton.h
    Source/UI/ChromeTextButton.cpp
    Source/UI/XyExplorer.h
    Source/UI/XyExplorer.cpp
    
    Source/DSP/HydraEngine.h
    Source/DSP/HydraEngine.cpp
    Source/DSP/HydraOscillator.h
    Source/DSP/HydraOscillator.cpp
    Source/DSP/HydraMacroMapper.h
    Source/DSP/HydraMacroMapper.cpp
    Source/DSP/HydraParallelSaturator.h
    Source/DSP/HydraParallelSaturator.cpp
    Source/DSP/HydraPhaseDisperser.h
    Source/DSP/HydraPhaseDisperser.cpp
    Source/DSP/ZdfLadderFilter.h
    Source/DSP/ZdfLadderFilter.cpp
) 

target_include_directories(Hydra PRIVATE Source Source/UI)

# Link your dependencies (Keeping juce_dsp active for instant access to filters/saturation)
target_link_libraries(Hydra PRIVATE
    HydraAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp           
    juce::juce_gui_basics
)

target_compile_definitions(Hydra PUBLIC 
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(Hydra PRIVATE cxx_std_20)

# ==============================================================================
# 5. TEST INFRASTRUCTURE (CATCH2)
# ==============================================================================
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(BoutiqueTests
    Tests/PluginTests.cpp
    Tests/HydraEngineTests.cpp
    Source/DSP/HydraEngine.cpp
    Source/DSP/HydraOscillator.cpp
    Source/DSP/HydraMacroMapper.cpp
    Source/DSP/HydraParallelSaturator.cpp
    Source/DSP/HydraPhaseDisperser.cpp
    Source/DSP/ZdfLadderFilter.cpp
)

target_include_directories(BoutiqueTests PRIVATE Source)

target_link_libraries(BoutiqueTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
)

target_compile_features(BoutiqueTests PRIVATE cxx_std_20)

if (BUILD_TESTING)
    add_test(NAME BoutiqueTests COMMAND BoutiqueTests)
endif()