#include "dsp/ToneprintDSP.h"

#include <array>
#include <cmath>
#include <iostream>

int main()
{
    toneprint::Processor processor;
    processor.prepare(48000.0, 512, 2);

    toneprint::Parameters params;
    params.driveDb = 12.0f;
    params.mix = 0.5f;
    processor.setParameters(params);

    std::array<float, 1024> left {};
    std::array<float, 1024> right {};
    left[0] = 0.8f;
    right[0] = -0.5f;

    float* channels[] = { left.data(), right.data() };
    processor.process(channels, 2, static_cast<int>(left.size()));

    auto peak = 0.0f;
    auto energy = 0.0f;
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
        {
            std::cerr << "Non-finite output at sample " << i << '\n';
            return 1;
        }

        peak = std::max(peak, std::fabs(left[i]));
        peak = std::max(peak, std::fabs(right[i]));
        energy += left[i] * left[i] + right[i] * right[i];
    }

    if (peak <= 0.0f || peak > 2.0f || energy <= 0.0f)
    {
        std::cerr << "Unexpected output peak=" << peak << " energy=" << energy << '\n';
        return 1;
    }

    std::cout << "DSP smoke test passed. peak=" << peak << " energy=" << energy << '\n';
    return 0;
}
