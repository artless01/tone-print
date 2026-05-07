#include <JuceHeader.h>
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
    double getTailLengthSeconds() const override { return 2.0; }

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
        values.outputDb = parameters.getRawParameterValue("output")->load();
        dsp.setParameters(values);

        dsp.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor(*this);
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
};
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NickToneprintAudioProcessor();
}
