#include "dsp/ToneprintDSP.h"

#include <algorithm>
#include <cmath>

namespace toneprint
{
namespace
{
constexpr float pi = 3.14159265358979323846f;

float dbToGain(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float clamp(float value, float minValue, float maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}
} // namespace

void Processor::prepare(double sampleRate, int, int channelCount)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    channelsPrepared = std::max(1, channelCount);

    const auto maxDelaySamples = static_cast<std::size_t>(std::ceil(sr * 2.0));
    delayLines.assign(static_cast<std::size_t>(channelsPrepared),
                      std::vector<float>(maxDelaySamples, 0.0f));
    toneState.assign(static_cast<std::size_t>(channelsPrepared), 0.0f);
    reset();
}

void Processor::reset()
{
    for (auto& line : delayLines)
        std::fill(line.begin(), line.end(), 0.0f);
    std::fill(toneState.begin(), toneState.end(), 0.0f);
    writePosition = 0;
    lfoPhase = 0.0f;
}

void Processor::setParameters(const Parameters& newParameters)
{
    parameters = newParameters;
    parameters.tone = clamp(parameters.tone, 0.0f, 1.0f);
    parameters.feedback = clamp(parameters.feedback, 0.0f, 0.92f);
    parameters.width = clamp(parameters.width, 0.0f, 1.6f);
    parameters.mix = clamp(parameters.mix, 0.0f, 1.0f);
    parameters.delayMs = clamp(parameters.delayMs, 1.0f, 1200.0f);
    parameters.modDepthMs = clamp(parameters.modDepthMs, 0.0f, 40.0f);
    parameters.modRateHz = clamp(parameters.modRateHz, 0.0f, 12.0f);
}

float Processor::processTone(int channel, float input)
{
    const auto cutoff = 450.0f + (parameters.tone * parameters.tone * 7600.0f);
    const auto alpha = 1.0f - std::exp(-2.0f * pi * cutoff / static_cast<float>(sr));
    auto& state = toneState[static_cast<std::size_t>(channel)];
    state += alpha * (input - state);

    const auto dark = state;
    const auto bright = input - (0.45f * state);
    return dark * (1.0f - parameters.tone) + bright * parameters.tone;
}

float Processor::readDelay(int channel, float delaySamples) const
{
    const auto& line = delayLines[static_cast<std::size_t>(channel)];
    const auto size = static_cast<int>(line.size());
    auto readPosition = static_cast<float>(writePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(size);

    const auto index0 = static_cast<int>(readPosition) % size;
    const auto index1 = (index0 + 1) % size;
    const auto fraction = readPosition - static_cast<float>(index0);
    return line[static_cast<std::size_t>(index0)] * (1.0f - fraction)
        + line[static_cast<std::size_t>(index1)] * fraction;
}

void Processor::process(float** channels, int channelCount, int sampleCount)
{
    if (channels == nullptr || channelCount <= 0 || sampleCount <= 0 || delayLines.empty())
        return;

    const auto activeChannels = std::min(channelCount, channelsPrepared);
    const auto driveGain = dbToGain(parameters.driveDb);
    const auto outputGain = dbToGain(parameters.outputDb);
    const auto baseDelaySamples = static_cast<float>(sr * parameters.delayMs / 1000.0);
    const auto depthSamples = static_cast<float>(sr * parameters.modDepthMs / 1000.0);
    const auto phaseIncrement = parameters.modRateHz / static_cast<float>(sr);

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        const auto lfo = std::sin(2.0f * pi * lfoPhase);
        lfoPhase += phaseIncrement;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;

        float dryLeft = 0.0f;
        float dryRight = 0.0f;
        float wetLeft = 0.0f;
        float wetRight = 0.0f;

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto* channelData = channels[channel];
            const auto dry = channelData[sample];
            const auto delayOffset = channel == 0 ? lfo : -lfo;
            const auto delayed = readDelay(channel, baseDelaySamples + depthSamples * delayOffset);
            const auto driven = std::tanh((dry + delayed * 0.28f) * driveGain);
            const auto shaped = processTone(channel, driven);
            const auto wet = shaped + delayed * 0.42f;

            delayLines[static_cast<std::size_t>(channel)]
                      [static_cast<std::size_t>(writePosition)] = dry + wet * parameters.feedback;

            if (channel == 0)
            {
                dryLeft = dry;
                wetLeft = wet;
            }
            else if (channel == 1)
            {
                dryRight = dry;
                wetRight = wet;
            }

            channelData[sample] = (dry * (1.0f - parameters.mix) + wet * parameters.mix) * outputGain;
        }

        if (activeChannels >= 2)
        {
            const auto mid = (wetLeft + wetRight) * 0.5f;
            const auto side = (wetLeft - wetRight) * 0.5f * parameters.width;
            const auto widenedLeft = mid + side;
            const auto widenedRight = mid - side;

            channels[0][sample] = (dryLeft * (1.0f - parameters.mix) + widenedLeft * parameters.mix) * outputGain;
            channels[1][sample] = (dryRight * (1.0f - parameters.mix) + widenedRight * parameters.mix) * outputGain;
        }

        writePosition = (writePosition + 1) % static_cast<int>(delayLines.front().size());
    }
}
} // namespace toneprint
