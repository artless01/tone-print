#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "dsp/ToneprintDSP.h"

namespace
{
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
    }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
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
        values.driftSend = parameters.getRawParameterValue("driftSend")->load();
        values.outputDb = parameters.getRawParameterValue("output")->load();
        dsp.setParameters(values);

        dsp.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

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
        : AudioProcessorEditor(processor)
    {
        struct ParamSpec
        {
            const char* id;
            const char* name;
        };

        constexpr std::array<ParamSpec, 17> specs {{
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
            { "driftSend", "Drift Send" },
            { "output", "Output" },
        }};

        controls.reserve(specs.size());
        for (const auto& spec : specs)
        {
            auto control = std::make_unique<KnobControl>(processor.getParameters(), spec.id, spec.name);
            addAndMakeVisible(*control);
            controls.push_back(std::move(control));
        }

        setSize(900, 390);
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.fillAll(juce::Colour(0xff0f1316));
        graphics.setColour(juce::Colour(0xff84d8ff));
        graphics.setFont(juce::Font(22.0f, juce::Font::bold));
        graphics.drawText("Nick Toneprint", 18, 8, 260, 28, juce::Justification::centredLeft);

        graphics.setColour(juce::Colour(0xff9aa7ad));
        graphics.setFont(juce::Font(13.0f));
        graphics.drawText("drift slap shimmer machine", 216, 12, 240, 22, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(18);
        area.removeFromTop(42);

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
