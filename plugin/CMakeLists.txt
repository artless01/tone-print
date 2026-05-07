juce_add_plugin(NickToneprint
    COMPANY_NAME "NickToneprint"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nktp
    PLUGIN_CODE Ntpr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Nick Toneprint"
)

juce_generate_juce_header(NickToneprint)

target_sources(NickToneprint PRIVATE
    JucePlugin.cpp
)

target_link_libraries(NickToneprint PRIVATE
    nick_toneprint_dsp
    juce::juce_audio_processors
    juce::juce_audio_utils
)

target_compile_definitions(NickToneprint PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
