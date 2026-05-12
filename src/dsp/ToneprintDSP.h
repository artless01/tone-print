#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace toneprint
{
struct Parameters
{
    float driveDb = 9.0f;
    float tone = 0.62f;
    float delayMs = 82.0f;
    float feedback = 0.18f;
    float modDepthMs = 2.5f;
    float modRateHz = 0.33f;
    float width = 1.08f;
    float mix = 0.34f;
    float verbMix = 0.0f;
    float verbDecay = 0.72f;
    float verbSize = 0.68f;
    float verbDamping = 0.46f;
    float preDelayMs = 24.0f;
    float shimmer = 0.0f;
    float shimmerTone = 0.64f;
    float afterimage = 0.0f;
    float afterimageCapture = 0.45f;
    float afterimageWarp = 0.38f;
    float driftSend = 0.65f;
    float outputDb = -1.5f;
};

class Processor
{
public:
    void prepare(double sampleRate, int maxBlockSize, int channelCount);
    void reset();
    void setParameters(const Parameters& newParameters);
    void process(float* const* channels, int channelCount, int sampleCount);

private:
    static constexpr int combCount = 4;
    static constexpr int allpassCount = 2;

    float processTone(int channel, float input);
    float readDelay(int channel, float delaySamples) const;
    float processReverb(int channel, float input);
    float processShimmer(int channel, float input);
    float processAfterimage(int channel, float input);
    float readInterpolated(const std::vector<float>& line, float readPosition) const;
    float processComb(int lineIndex, float input, int delaySamples, float feedback, float damping);
    float processAllpass(int lineIndex, float input, int delaySamples);
    float readFromLine(const std::vector<float>& line, int writePosition, int delaySamples) const;

    Parameters parameters;
    double sr = 44100.0;
    int channelsPrepared = 0;
    int writePosition = 0;
    float lfoPhase = 0.0f;
    std::vector<std::vector<float>> delayLines;
    std::vector<float> toneState;
    std::vector<std::vector<float>> preDelayLines;
    std::vector<int> preDelayPositions;
    std::vector<std::vector<float>> shimmerLines;
    std::vector<int> shimmerPositions;
    std::vector<float> shimmerPhases;
    std::vector<std::vector<float>> afterimageLines;
    std::vector<int> afterimagePositions;
    std::vector<float> afterimagePhases;
    std::vector<float> afterimageFastEnvelope;
    std::vector<float> afterimageSlowEnvelope;
    std::vector<float> afterimageHold;
    std::vector<float> afterimageToneState;
    std::vector<std::vector<float>> combLines;
    std::vector<int> combPositions;
    std::vector<float> combDampingState;
    std::vector<std::vector<float>> allpassLines;
    std::vector<int> allpassPositions;
    std::vector<float> shimmerToneState;
};
} // namespace toneprint
