#include "dsp/ToneprintDSP.h"

#include <algorithm>
#include <cmath>

namespace toneprint
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr std::array<int, 4> baseCombDelays { 1557, 1617, 1491, 1422 };
constexpr std::array<int, 2> baseAllpassDelays { 225, 556 };

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
    const auto maxPreDelaySamples = static_cast<std::size_t>(std::ceil(sr * 0.25));
    const auto reverbLineSamples = static_cast<std::size_t>(std::ceil(sr * 0.16));
    delayLines.assign(static_cast<std::size_t>(channelsPrepared),
                      std::vector<float>(maxDelaySamples, 0.0f));
    toneState.assign(static_cast<std::size_t>(channelsPrepared), 0.0f);
    preDelayLines.assign(static_cast<std::size_t>(channelsPrepared),
                         std::vector<float>(maxPreDelaySamples, 0.0f));
    preDelayPositions.assign(static_cast<std::size_t>(channelsPrepared), 0);
    combLines.assign(static_cast<std::size_t>(channelsPrepared * combCount),
                     std::vector<float>(reverbLineSamples, 0.0f));
    combPositions.assign(static_cast<std::size_t>(channelsPrepared * combCount), 0);
    combDampingState.assign(static_cast<std::size_t>(channelsPrepared * combCount), 0.0f);
    allpassLines.assign(static_cast<std::size_t>(channelsPrepared * allpassCount),
                        std::vector<float>(reverbLineSamples, 0.0f));
    allpassPositions.assign(static_cast<std::size_t>(channelsPrepared * allpassCount), 0);
    shimmerToneState.assign(static_cast<std::size_t>(channelsPrepared), 0.0f);
    reset();
}

void Processor::reset()
{
    for (auto& line : delayLines)
        std::fill(line.begin(), line.end(), 0.0f);
    for (auto& line : preDelayLines)
        std::fill(line.begin(), line.end(), 0.0f);
    for (auto& line : combLines)
        std::fill(line.begin(), line.end(), 0.0f);
    for (auto& line : allpassLines)
        std::fill(line.begin(), line.end(), 0.0f);
    std::fill(toneState.begin(), toneState.end(), 0.0f);
    std::fill(preDelayPositions.begin(), preDelayPositions.end(), 0);
    std::fill(combPositions.begin(), combPositions.end(), 0);
    std::fill(combDampingState.begin(), combDampingState.end(), 0.0f);
    std::fill(allpassPositions.begin(), allpassPositions.end(), 0);
    std::fill(shimmerToneState.begin(), shimmerToneState.end(), 0.0f);
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
    parameters.verbMix = clamp(parameters.verbMix, 0.0f, 1.0f);
    parameters.verbDecay = clamp(parameters.verbDecay, 0.0f, 0.98f);
    parameters.verbSize = clamp(parameters.verbSize, 0.25f, 1.35f);
    parameters.verbDamping = clamp(parameters.verbDamping, 0.0f, 0.98f);
    parameters.preDelayMs = clamp(parameters.preDelayMs, 0.0f, 220.0f);
    parameters.shimmer = clamp(parameters.shimmer, 0.0f, 1.0f);
    parameters.shimmerTone = clamp(parameters.shimmerTone, 0.0f, 1.0f);
    parameters.driftSend = clamp(parameters.driftSend, 0.0f, 1.0f);
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

float Processor::readFromLine(const std::vector<float>& line, int lineWritePosition, int delaySamples) const
{
    const auto size = static_cast<int>(line.size());
    const auto clampedDelay = std::max(1, std::min(delaySamples, size - 1));
    auto readPosition = lineWritePosition - clampedDelay;
    while (readPosition < 0)
        readPosition += size;

    return line[static_cast<std::size_t>(readPosition % size)];
}

float Processor::processComb(int lineIndex, float input, int delaySamples, float feedback, float damping)
{
    auto& line = combLines[static_cast<std::size_t>(lineIndex)];
    auto& position = combPositions[static_cast<std::size_t>(lineIndex)];
    auto& damped = combDampingState[static_cast<std::size_t>(lineIndex)];

    const auto delayed = readFromLine(line, position, delaySamples);
    damped += (1.0f - damping) * (delayed - damped);
    line[static_cast<std::size_t>(position)] = input + damped * feedback;
    position = (position + 1) % static_cast<int>(line.size());
    return delayed;
}

float Processor::processAllpass(int lineIndex, float input, int delaySamples)
{
    auto& line = allpassLines[static_cast<std::size_t>(lineIndex)];
    auto& position = allpassPositions[static_cast<std::size_t>(lineIndex)];

    const auto delayed = readFromLine(line, position, delaySamples);
    constexpr auto feedback = 0.52f;
    const auto output = -input + delayed;
    line[static_cast<std::size_t>(position)] = input + delayed * feedback;
    position = (position + 1) % static_cast<int>(line.size());
    return output;
}

float Processor::processReverb(int channel, float input)
{
    auto& preDelayLine = preDelayLines[static_cast<std::size_t>(channel)];
    auto& preDelayPosition = preDelayPositions[static_cast<std::size_t>(channel)];
    const auto preDelaySamples = static_cast<int>(sr * parameters.preDelayMs / 1000.0);
    const auto predelayed = readFromLine(preDelayLine, preDelayPosition, preDelaySamples);
    preDelayLine[static_cast<std::size_t>(preDelayPosition)] = input;
    preDelayPosition = (preDelayPosition + 1) % static_cast<int>(preDelayLine.size());

    auto& shimmerState = shimmerToneState[static_cast<std::size_t>(channel)];
    const auto shimmerCutoff = 1400.0f + parameters.shimmerTone * parameters.shimmerTone * 9200.0f;
    const auto shimmerAlpha = 1.0f - std::exp(-2.0f * pi * shimmerCutoff / static_cast<float>(sr));
    const auto octaveExciter = std::fabs(predelayed) * 2.0f - std::fabs(input);
    shimmerState += shimmerAlpha * (octaveExciter - shimmerState);

    const auto tankInput = predelayed + shimmerState * parameters.shimmer * 0.55f;
    const auto delayScale = parameters.verbSize * static_cast<float>(sr / 44100.0);
    const auto feedback = 0.52f + parameters.verbDecay * 0.38f;
    const auto damping = 0.12f + parameters.verbDamping * 0.78f;
    auto sum = 0.0f;

    for (int i = 0; i < combCount; ++i)
    {
        const auto delay = static_cast<int>(static_cast<float>(baseCombDelays[static_cast<std::size_t>(i)] + channel * 37) * delayScale);
        sum += processComb(channel * combCount + i, tankInput, delay, feedback, damping);
    }

    auto diffused = sum * 0.24f;
    for (int i = 0; i < allpassCount; ++i)
    {
        const auto delay = static_cast<int>(static_cast<float>(baseAllpassDelays[static_cast<std::size_t>(i)] + channel * 19) * delayScale);
        diffused = processAllpass(channel * allpassCount + i, diffused, delay);
    }

    return std::tanh(diffused * 1.25f);
}

void Processor::process(float* const* channels, int channelCount, int sampleCount)
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
            const auto verbLeft = processReverb(0, widenedLeft * parameters.driftSend);
            const auto verbRight = processReverb(1, widenedRight * parameters.driftSend);
            const auto bloomedLeft = widenedLeft + verbLeft * parameters.verbMix;
            const auto bloomedRight = widenedRight + verbRight * parameters.verbMix;

            channels[0][sample] = (dryLeft * (1.0f - parameters.mix) + bloomedLeft * parameters.mix) * outputGain;
            channels[1][sample] = (dryRight * (1.0f - parameters.mix) + bloomedRight * parameters.mix) * outputGain;
        }
        else if (activeChannels == 1)
        {
            const auto verb = processReverb(0, wetLeft * parameters.driftSend);
            channels[0][sample] = (dryLeft * (1.0f - parameters.mix)
                + (wetLeft + verb * parameters.verbMix) * parameters.mix) * outputGain;
        }

        writePosition = (writePosition + 1) % static_cast<int>(delayLines.front().size());
    }
}
} // namespace toneprint
