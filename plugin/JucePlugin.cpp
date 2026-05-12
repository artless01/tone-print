#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "dsp/ToneprintDSP.h"

namespace
{
struct PresetValue
{
    const char* id;
    float value;
};

struct Preset
{
    const char* name;
    const char* description;
    std::array<PresetValue, 20> values;
};

const std::array<Preset, 6>& getPresets()
{
    static constexpr std::array<Preset, 6> presets {{
        {
            "Tape Ghost",
            "Tight slap, mild drive, and low drift for always-on guitar or vocal thickening.",
            {{
                { "drive", 7.5f }, { "tone", 0.54f }, { "delay", 74.0f }, { "feedback", 0.16f },
                { "modDepth", 1.8f }, { "modRate", 0.24f }, { "width", 0.92f }, { "mix", 0.28f },
                { "verbMix", 0.10f }, { "verbDecay", 0.48f }, { "verbSize", 0.52f }, { "verbDamping", 0.58f },
                { "preDelay", 18.0f }, { "shimmer", 0.0f }, { "shimmerTone", 0.50f }, { "driftSend", 0.35f },
                { "afterimage", 0.0f }, { "capture", 0.30f }, { "warp", 0.24f },
                { "output", -1.5f },
            }},
        },
        {
            "Oscillator Slap",
            "The first magic zone: slow slap, around 30% feedback, and long drift movement.",
            {{
                { "drive", 10.0f }, { "tone", 0.60f }, { "delay", 118.0f }, { "feedback", 0.31f },
                { "modDepth", 8.5f }, { "modRate", 0.08f }, { "width", 1.18f }, { "mix", 0.42f },
                { "verbMix", 0.16f }, { "verbDecay", 0.62f }, { "verbSize", 0.74f }, { "verbDamping", 0.42f },
                { "preDelay", 28.0f }, { "shimmer", 0.10f }, { "shimmerTone", 0.58f }, { "driftSend", 0.72f },
                { "afterimage", 0.24f }, { "capture", 0.52f }, { "warp", 0.46f },
                { "output", -2.0f },
            }},
        },
        {
            "Blue Drift",
            "Wide drift feeding a long, soft shimmer wash for chords, swells, and ambient beds.",
            {{
                { "drive", 5.0f }, { "tone", 0.48f }, { "delay", 96.0f }, { "feedback", 0.22f },
                { "modDepth", 6.8f }, { "modRate", 0.15f }, { "width", 1.28f }, { "mix", 0.50f },
                { "verbMix", 0.58f }, { "verbDecay", 0.88f }, { "verbSize", 1.06f }, { "verbDamping", 0.55f },
                { "preDelay", 42.0f }, { "shimmer", 0.42f }, { "shimmerTone", 0.70f }, { "driftSend", 0.86f },
                { "afterimage", 0.48f }, { "capture", 0.50f }, { "warp", 0.42f },
                { "output", -4.0f },
            }},
        },
        {
            "Vocal Mirage",
            "Lower drive, wider modulation, and controlled bloom for a vocal double/dream smear.",
            {{
                { "drive", 4.0f }, { "tone", 0.57f }, { "delay", 64.0f }, { "feedback", 0.12f },
                { "modDepth", 4.2f }, { "modRate", 0.31f }, { "width", 1.35f }, { "mix", 0.32f },
                { "verbMix", 0.28f }, { "verbDecay", 0.72f }, { "verbSize", 0.82f }, { "verbDamping", 0.62f },
                { "preDelay", 34.0f }, { "shimmer", 0.16f }, { "shimmerTone", 0.56f }, { "driftSend", 0.56f },
                { "afterimage", 0.34f }, { "capture", 0.40f }, { "warp", 0.31f },
                { "output", -2.0f },
            }},
        },
        {
            "DI Fever Dream",
            "Hotter drive and unstable drift for leads, noise parts, and synth-like guitar lines.",
            {{
                { "drive", 17.0f }, { "tone", 0.68f }, { "delay", 142.0f }, { "feedback", 0.38f },
                { "modDepth", 10.5f }, { "modRate", 0.21f }, { "width", 1.42f }, { "mix", 0.56f },
                { "verbMix", 0.34f }, { "verbDecay", 0.78f }, { "verbSize", 0.92f }, { "verbDamping", 0.36f },
                { "preDelay", 22.0f }, { "shimmer", 0.26f }, { "shimmerTone", 0.76f }, { "driftSend", 0.90f },
                { "afterimage", 0.56f }, { "capture", 0.72f }, { "warp", 0.67f },
                { "output", -5.0f },
            }},
        },
        {
            "Cloud Machine",
            "Maximum bloom: big verb, clear shimmer, and afterimage memory turning notes into atmosphere.",
            {{
                { "drive", 6.0f }, { "tone", 0.46f }, { "delay", 132.0f }, { "feedback", 0.26f },
                { "modDepth", 7.2f }, { "modRate", 0.11f }, { "width", 1.50f }, { "mix", 0.64f },
                { "verbMix", 0.82f }, { "verbDecay", 0.94f }, { "verbSize", 1.22f }, { "verbDamping", 0.50f },
                { "preDelay", 58.0f }, { "shimmer", 0.68f }, { "shimmerTone", 0.82f }, { "driftSend", 1.0f },
                { "afterimage", 0.72f }, { "capture", 0.58f }, { "warp", 0.55f },
                { "output", -6.0f },
            }},
        },
    }};

    return presets;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto db = juce::AudioParameterFloatAttributes().withLabel("dB");
    const auto ms = juce::AudioParameterFloatAttributes().withLabel("ms");
    const auto hz = juce::AudioParameterFloatAttributes().withLabel("Hz");

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive", juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f), 9.0f, db));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tone", "Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.62f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delay", "Slap", juce::NormalisableRange<float>(20.0f, 220.0f, 0.01f), 82.0f, ms));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 0.75f, 0.001f), 0.18f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "modDepth", "Drift", juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 2.5f, ms));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "modRate", "Drift Rate", juce::NormalisableRange<float>(0.03f, 2.0f, 0.001f), 0.33f, hz));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "width", "Width", juce::NormalisableRange<float>(0.0f, 1.5f, 0.001f), 1.08f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.34f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "verbMix", "Verb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "verbDecay", "Decay", juce::NormalisableRange<float>(0.0f, 0.98f, 0.001f), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "verbSize", "Size", juce::NormalisableRange<float>(0.25f, 1.35f, 0.001f), 0.68f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "verbDamping", "Damping", juce::NormalisableRange<float>(0.0f, 0.98f, 0.001f), 0.46f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "preDelay", "PreDelay", juce::NormalisableRange<float>(0.0f, 220.0f, 0.01f), 24.0f, ms));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shimmer", "Shimmer", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shimmerTone", "Shimmer Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.64f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "afterimage", "Afterimage", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "capture", "Capture", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "warp", "Warp", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.38f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "driftSend", "Drift Send", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), -1.5f, db));

    return { params.begin(), params.end() };
}

class NickToneprintAudioProcessor final : public juce::AudioProcessor
{
public:
    NickToneprintAudioProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        setCurrentProgram(0);
    }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return static_cast<int>(getPresets().size()); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override
    {
        const auto& presets = getPresets();
        if (index < 0 || index >= static_cast<int>(presets.size()))
            return;

        currentProgram = index;
        for (const auto& value : presets[static_cast<std::size_t>(index)].values)
            if (auto* parameter = parameters.getParameter(value.id))
                parameter->setValueNotifyingHost(parameter->convertTo0to1(value.value));
    }

    const juce::String getProgramName(int index) override
    {
        const auto& presets = getPresets();
        if (index < 0 || index >= static_cast<int>(presets.size()))
            return {};

        return presets[static_cast<std::size_t>(index)].name;
    }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        dsp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }

    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        const auto mainInput = layouts.getMainInputChannelSet();
        const auto mainOutput = layouts.getMainOutputChannelSet();
        return mainInput == mainOutput
            && (mainOutput == juce::AudioChannelSet::mono()
                || mainOutput == juce::AudioChannelSet::stereo());
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
            buffer.clear(channel, 0, buffer.getNumSamples());

        toneprint::Parameters values;
        values.driveDb = parameters.getRawParameterValue("drive")->load();
        values.tone = parameters.getRawParameterValue("tone")->load();
        values.delayMs = parameters.getRawParameterValue("delay")->load();
        values.feedback = parameters.getRawParameterValue("feedback")->load();
        values.modDepthMs = parameters.getRawParameterValue("modDepth")->load();
        values.modRateHz = parameters.getRawParameterValue("modRate")->load();
        values.width = parameters.getRawParameterValue("width")->load();
        values.mix = parameters.getRawParameterValue("mix")->load();
        values.verbMix = parameters.getRawParameterValue("verbMix")->load();
        values.verbDecay = parameters.getRawParameterValue("verbDecay")->load();
        values.verbSize = parameters.getRawParameterValue("verbSize")->load();
        values.verbDamping = parameters.getRawParameterValue("verbDamping")->load();
        values.preDelayMs = parameters.getRawParameterValue("preDelay")->load();
        values.shimmer = parameters.getRawParameterValue("shimmer")->load();
        values.shimmerTone = parameters.getRawParameterValue("shimmerTone")->load();
        values.afterimage = parameters.getRawParameterValue("afterimage")->load();
        values.afterimageCapture = parameters.getRawParameterValue("capture")->load();
        values.afterimageWarp = parameters.getRawParameterValue("warp")->load();
        values.driftSend = parameters.getRawParameterValue("driftSend")->load();
        values.outputDb = parameters.getRawParameterValue("output")->load();
        dsp.setParameters(values);

        dsp.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    juce::AudioProcessorValueTreeState& getParameterState() { return parameters; }
    const char* getPresetDescription(int index) const
    {
        const auto& presets = getPresets();
        if (index < 0 || index >= static_cast<int>(presets.size()))
            return "";

        return presets[static_cast<std::size_t>(index)].description;
    }

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        if (auto xml = parameters.copyState().createXml())
            copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            if (xml->hasTagName(parameters.state.getType()))
                parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

private:
    toneprint::Processor dsp;
    juce::AudioProcessorValueTreeState parameters;
    int currentProgram = 0;
};

class KnobControl final : public juce::Component
{
public:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    KnobControl(juce::AudioProcessorValueTreeState& parameters,
                const juce::String& parameterId,
                const juce::String& name)
        : attachment(parameters, parameterId, slider)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
        label.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff84d8ff));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff26323a));
        slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffd166));
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff12171b));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2f3a42));
        addAndMakeVisible(slider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        label.setBounds(area.removeFromTop(20));
        slider.setBounds(area);
    }

private:
    juce::Slider slider;
    juce::Label label;
    Attachment attachment;
};

class NickToneprintAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit NickToneprintAudioProcessorEditor(NickToneprintAudioProcessor& processor)
        : AudioProcessorEditor(processor),
          processorRef(processor)
    {
        struct ParamSpec
        {
            const char* id;
            const char* name;
        };

        constexpr std::array<ParamSpec, 20> specs {{
            { "drive", "Drive" },
            { "tone", "Tone" },
            { "delay", "Slap" },
            { "feedback", "Feedback" },
            { "modDepth", "Drift" },
            { "modRate", "Drift Rate" },
            { "width", "Width" },
            { "mix", "Mix" },
            { "verbMix", "Verb Mix" },
            { "verbDecay", "Decay" },
            { "verbSize", "Size" },
            { "verbDamping", "Damping" },
            { "preDelay", "PreDelay" },
            { "shimmer", "Shimmer" },
            { "shimmerTone", "Shim Tone" },
            { "afterimage", "Afterimage" },
            { "capture", "Capture" },
            { "warp", "Warp" },
            { "driftSend", "Drift Send" },
            { "output", "Output" },
        }};

        controls.reserve(specs.size());
        for (const auto& spec : specs)
        {
            auto control = std::make_unique<KnobControl>(processor.getParameterState(), spec.id, spec.name);
            addAndMakeVisible(*control);
            controls.push_back(std::move(control));
        }

        presetLabel.setText("Preset", juce::dontSendNotification);
        presetLabel.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
        presetLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(presetLabel);

        const auto& presets = getPresets();
        for (std::size_t index = 0; index < presets.size(); ++index)
            presetBox.addItem(presets[index].name, static_cast<int>(index) + 1);

        presetBox.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
        presetBox.onChange = [this]
        {
            const auto selected = presetBox.getSelectedId() - 1;
            processorRef.setCurrentProgram(selected);
            updatePresetDescription();
        };
        addAndMakeVisible(presetBox);

        presetDescription.setColour(juce::Label::textColourId, juce::Colour(0xffc8d1d5));
        presetDescription.setFont(juce::Font(13.0f));
        presetDescription.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(presetDescription);
        updatePresetDescription();

        setSize(900, 540);
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.fillAll(juce::Colour(0xff0f1316));
        graphics.setColour(juce::Colour(0xff84d8ff));
        graphics.setFont(juce::Font(22.0f, juce::Font::bold));
        graphics.drawText("Nick Toneprint", 18, 8, 260, 28, juce::Justification::centredLeft);

        graphics.setColour(juce::Colour(0xff9aa7ad));
        graphics.setFont(juce::Font(13.0f));
        graphics.drawText("drift slap shimmer afterimage machine", 216, 12, 320, 22, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(18);
        area.removeFromTop(42);
        auto presetArea = area.removeFromTop(42);
        presetLabel.setBounds(presetArea.removeFromLeft(54));
        presetBox.setBounds(presetArea.removeFromLeft(210).reduced(0, 4));
        presetDescription.setBounds(presetArea.reduced(14, 0));
        area.removeFromTop(8);

        constexpr auto columns = 6;
        const auto cellWidth = area.getWidth() / columns;
        constexpr auto cellHeight = 104;

        for (std::size_t index = 0; index < controls.size(); ++index)
        {
            const auto row = static_cast<int>(index) / columns;
            const auto column = static_cast<int>(index) % columns;
            controls[index]->setBounds(area.getX() + column * cellWidth,
                                       area.getY() + row * cellHeight,
                                       cellWidth,
                                       cellHeight);
        }
    }

private:
    void updatePresetDescription()
    {
        presetDescription.setText(processorRef.getPresetDescription(presetBox.getSelectedId() - 1),
                                  juce::dontSendNotification);
    }

    NickToneprintAudioProcessor& processorRef;
    juce::Label presetLabel;
    juce::ComboBox presetBox;
    juce::Label presetDescription;
    std::vector<std::unique_ptr<KnobControl>> controls;
};

juce::AudioProcessorEditor* NickToneprintAudioProcessor::createEditor()
{
    return new NickToneprintAudioProcessorEditor(*this);
}
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NickToneprintAudioProcessor();
}
