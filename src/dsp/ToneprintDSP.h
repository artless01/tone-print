#pragma once

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
    float processTone(int channel, float input);
    float readDelay(int channel, float delaySamples) const;

    Parameters parameters;
    double sr = 44100.0;
    int channelsPrepared = 0;
    int writePosition = 0;
    float lfoPhase = 0.0f;
    std::vector<std::vector<float>> delayLines;
    std::vector<float> toneState;
};
} // namespace toneprint
