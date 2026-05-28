#include "dsp/ToneprintDSP.h"

#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    toneprint::Processor processor;
    processor.prepare(48000.0, 512, 2);

    toneprint::Parameters params;
    params.driveDb = 12.0f;
    params.mix = 0.5f;
    params.verbMix = 0.7f;
    params.verbDecay = 0.88f;
    params.shimmer = 0.55f;
    params.afterimage = 0.35f;
    params.afterimageCapture = 0.55f;
    params.afterimageWarp = 0.48f;
    params.driftSend = 0.8f;
    processor.setParameters(params);

    std::vector<float> left(96000, 0.0f);
    std::vector<float> right(96000, 0.0f);
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

    toneprint::Processor dryShimmerProcessor;
    toneprint::Processor wetShimmerProcessor;
    dryShimmerProcessor.prepare(48000.0, 512, 2);
    wetShimmerProcessor.prepare(48000.0, 512, 2);

    params.shimmer = 0.0f;
    dryShimmerProcessor.setParameters(params);
    params.shimmer = 0.75f;
    wetShimmerProcessor.setParameters(params);

    std::vector<float> dryShimmerLeft(96000, 0.0f);
    std::vector<float> dryShimmerRight(96000, 0.0f);
    std::vector<float> wetShimmerLeft(96000, 0.0f);
    std::vector<float> wetShimmerRight(96000, 0.0f);
    dryShimmerLeft[0] = wetShimmerLeft[0] = 0.6f;
    dryShimmerRight[0] = wetShimmerRight[0] = -0.4f;

    float* dryShimmerChannels[] = { dryShimmerLeft.data(), dryShimmerRight.data() };
    float* wetShimmerChannels[] = { wetShimmerLeft.data(), wetShimmerRight.data() };
    dryShimmerProcessor.process(dryShimmerChannels, 2, static_cast<int>(dryShimmerLeft.size()));
    wetShimmerProcessor.process(wetShimmerChannels, 2, static_cast<int>(wetShimmerLeft.size()));

    auto shimmerDifference = 0.0f;
    for (std::size_t i = 0; i < wetShimmerLeft.size(); ++i)
    {
        shimmerDifference += std::fabs(wetShimmerLeft[i] - dryShimmerLeft[i]);
        shimmerDifference += std::fabs(wetShimmerRight[i] - dryShimmerRight[i]);
    }

    if (shimmerDifference <= 0.001f)
    {
        std::cerr << "Shimmer control did not change output. difference=" << shimmerDifference << '\n';
        return 1;
    }

    toneprint::Processor dryAfterimageProcessor;
    toneprint::Processor wetAfterimageProcessor;
    dryAfterimageProcessor.prepare(48000.0, 512, 2);
    wetAfterimageProcessor.prepare(48000.0, 512, 2);

    params.shimmer = 0.25f;
    params.verbMix = 0.8f;
    params.afterimage = 0.0f;
    dryAfterimageProcessor.setParameters(params);
    params.afterimage = 0.85f;
    params.afterimageCapture = 0.80f;
    params.afterimageWarp = 0.72f;
    wetAfterimageProcessor.setParameters(params);

    std::vector<float> dryAfterimageLeft(96000, 0.0f);
    std::vector<float> dryAfterimageRight(96000, 0.0f);
    std::vector<float> wetAfterimageLeft(96000, 0.0f);
    std::vector<float> wetAfterimageRight(96000, 0.0f);
    for (std::size_t i = 0; i < 2400; ++i)
    {
        const auto sample = static_cast<float>(std::sin(static_cast<double>(i) * 0.065)) * 0.45f;
        dryAfterimageLeft[i] = wetAfterimageLeft[i] = sample;
        dryAfterimageRight[i] = wetAfterimageRight[i] = -sample * 0.7f;
    }

    float* dryAfterimageChannels[] = { dryAfterimageLeft.data(), dryAfterimageRight.data() };
    float* wetAfterimageChannels[] = { wetAfterimageLeft.data(), wetAfterimageRight.data() };
    dryAfterimageProcessor.process(dryAfterimageChannels, 2, static_cast<int>(dryAfterimageLeft.size()));
    wetAfterimageProcessor.process(wetAfterimageChannels, 2, static_cast<int>(wetAfterimageLeft.size()));

    auto afterimageTailEnergy = 0.0f;
    auto dryTailEnergy = 0.0f;
    for (std::size_t i = 24000; i < wetAfterimageLeft.size(); ++i)
    {
        afterimageTailEnergy += wetAfterimageLeft[i] * wetAfterimageLeft[i]
            + wetAfterimageRight[i] * wetAfterimageRight[i];
        dryTailEnergy += dryAfterimageLeft[i] * dryAfterimageLeft[i]
            + dryAfterimageRight[i] * dryAfterimageRight[i];
    }

    if (afterimageTailEnergy <= dryTailEnergy * 1.05f)
    {
        std::cerr << "Afterimage did not increase tail energy. dry=" << dryTailEnergy
                  << " wet=" << afterimageTailEnergy << '\n';
        return 1;
    }

    toneprint::Processor tajHallProcessor;
    tajHallProcessor.prepare(48000.0, 512, 2);

    toneprint::Parameters tajHall;
    tajHall.driveDb = 3.5f;
    tajHall.tone = 0.72f;
    tajHall.delayMs = 48.0f;
    tajHall.feedback = 0.08f;
    tajHall.modDepthMs = 2.4f;
    tajHall.modRateHz = 0.16f;
    tajHall.width = 1.20f;
    tajHall.mix = 0.54f;
    tajHall.verbMix = 0.78f;
    tajHall.verbDecay = 0.93f;
    tajHall.verbSize = 1.18f;
    tajHall.verbDamping = 0.40f;
    tajHall.preDelayMs = 62.0f;
    tajHall.shimmer = 0.0f;
    tajHall.afterimage = 0.30f;
    tajHall.afterimageCapture = 0.36f;
    tajHall.afterimageWarp = 0.20f;
    tajHall.driftSend = 0.92f;
    tajHall.outputDb = -5.5f;
    tajHallProcessor.setParameters(tajHall);

    std::vector<float> tajLeft(192000, 0.0f);
    std::vector<float> tajRight(192000, 0.0f);
    for (std::size_t i = 0; i < 1800; ++i)
    {
        const auto envelope = 1.0f - static_cast<float>(i) / 1800.0f;
        const auto note = std::sin(static_cast<double>(i) * 0.049) * 0.33f * envelope;
        tajLeft[i] = note;
        tajRight[i] = note * 0.92f;
    }

    float* tajChannels[] = { tajLeft.data(), tajRight.data() };
    tajHallProcessor.process(tajChannels, 2, static_cast<int>(tajLeft.size()));

    auto tajPeak = 0.0f;
    auto tajLateTailEnergy = 0.0f;
    for (std::size_t i = 0; i < tajLeft.size(); ++i)
    {
        if (!std::isfinite(tajLeft[i]) || !std::isfinite(tajRight[i]))
        {
            std::cerr << "Non-finite Taj Hall output at sample " << i << '\n';
            return 1;
        }

        tajPeak = std::max(tajPeak, std::fabs(tajLeft[i]));
        tajPeak = std::max(tajPeak, std::fabs(tajRight[i]));

        if (i > 72000)
            tajLateTailEnergy += tajLeft[i] * tajLeft[i] + tajRight[i] * tajRight[i];
    }

    if (tajPeak <= 0.0f || tajPeak > 1.5f || tajLateTailEnergy <= 0.0001f)
    {
        std::cerr << "Taj Hall preset lost the long reverb behavior. peak=" << tajPeak
                  << " tail=" << tajLateTailEnergy << '\n';
        return 1;
    }

    std::cout << "DSP smoke test passed. peak=" << peak << " energy=" << energy << '\n';
    return 0;
}
