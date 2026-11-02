cmake_minimum_required(VERSION 3.22)
project(MX400Controller VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core checks (no JUCE, no hardware): ctest, or sh tests/run.sh
enable_testing()
add_executable(mx400_test_core tests/test_core.cpp)
if(MSVC)
  target_compile_options(mx400_test_core PRIVATE /W4)
else()
  target_compile_options(mx400_test_core PRIVATE -Wall -Wextra)
endif()
add_test(NAME core COMMAND mx400_test_core)

# Plugin. Pass -DMX400_JUCE_DIR=/path/to/JUCE and -DFETCHCONTENT_SOURCE_DIR_CAT_UI=/path/to/cat-ui for local sources.
option(MX400_BUILD_PLUGIN "Build the VST3 plugin and standalone app" ON)
if(MX400_BUILD_PLUGIN)
  if(MX400_JUCE_DIR)
    add_subdirectory(${MX400_JUCE_DIR} juce)
  else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.16-security GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
  endif()
  include(FetchContent)
  FetchContent_Declare(cat_ui GIT_REPOSITORY https://github.com/SpeakingColors/cat-ui GIT_TAG main GIT_SHALLOW ON)
  FetchContent_MakeAvailable(cat_ui)

  juce_add_plugin(MX400Controller
    COMPANY_NAME "Studio 87"
    BUNDLE_ID "com.studio87.mx400controller"
    PLUGIN_MANUFACTURER_CODE Spkc
    PLUGIN_CODE MxCt
    FORMATS VST3 Standalone
    PRODUCT_NAME "MX400 Controller"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(MX400Controller PRIVATE src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
  target_include_directories(MX400Controller PRIVATE src)
  target_compile_definitions(MX400Controller PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(MX400Controller PRIVATE
    cat_ui juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  # Processor checks (parameters to MIDI, no send on load): console app, runs offline.
  option(MX400_BUILD_PLUGIN_TEST "Build the processor test" ON)
  if(MX400_BUILD_PLUGIN_TEST)
    juce_add_console_app(MX400PluginTest PRODUCT_NAME "MX400PluginTest")
    target_sources(MX400PluginTest PRIVATE tests/plugin_test.cpp src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
    target_include_directories(MX400PluginTest PRIVATE src)
    target_compile_definitions(MX400PluginTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
      JucePlugin_VersionString=\"${PROJECT_VERSION}\")
    target_link_libraries(MX400PluginTest PRIVATE cat_ui juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
  endif()
  # Screenshot tool: renders the real editor to PNG (docs and manual). Off by default.
  option(MX400_BUILD_TOOLS "Build the screenshot tool" OFF)
  if(MX400_BUILD_TOOLS)
    juce_add_console_app(MX400Screenshots PRODUCT_NAME "MX400Screenshots")
    target_sources(MX400Screenshots PRIVATE tools/screenshot.cpp src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
    target_include_directories(MX400Screenshots PRIVATE src)
    target_compile_definitions(MX400Screenshots PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
      JucePlugin_VersionString=\"${PROJECT_VERSION}\")
    target_link_libraries(MX400Screenshots PRIVATE cat_ui juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
  endif()
endif()
