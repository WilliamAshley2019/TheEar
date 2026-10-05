#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    Perceptual weighting based on an approximation of the
    ISO 226:2003 equal-loudness contours (Fletcher-Munson).

    Used to make stage 'amount' fades feel perceptually even across
    the audible spectrum, instead of linear (which sounds unnatural).
*/
namespace Perceptual
{
    /** Reference SPL value at 1000 Hz — computed once so we can normalise. */
    inline float splAt1000()
    {
        const float f = 1000.0f;
        const float f2 = f * f;
        const float num = std::pow(f2 + 20.6f * 20.6f, 2.0f)
            * std::pow(f2 + 12200.0f * 12200.0f, 1.0f);
        const float den = std::pow(f2 + 107.7f * 107.7f, 2.0f)
            * std::pow(f2 + 737.9f * 737.9f, 1.0f);
        return 20.0f * std::log10(num / den) + 0.3f;
    }

    /** Returns the perceptual weight for a frequency, normalised so
        1000 Hz -> 1.0. Values > 1 mean the ear is more sensitive
        there (so we need *less* stage gain to be perceived as equal).
    */
    inline float weighting(float frequency)
    {
        const float f = juce::jlimit(20.0f, 20000.0f, frequency);
        const float f2 = f * f;

        const float num = std::pow(f2 + 20.6f * 20.6f, 2.0f)
            * std::pow(f2 + 12200.0f * 12200.0f, 1.0f);
        const float den = std::pow(f2 + 107.7f * 107.7f, 2.0f)
            * std::pow(f2 + 737.9f * 737.9f, 1.0f);

        const float spl = 20.0f * std::log10(num / den) + 0.3f;

        static const float refSpl = splAt1000();
        const float relative = std::pow(10.0f, (spl - refSpl) / 40.0f);

        return juce::jlimit(0.25f, 2.5f, relative);
    }

    /** Returns a frequency-weighted amount for the given linear knob value.

        @param frequency   the characteristic frequency of the stage (Hz)
        @param amount      0..1 knob value
        @param strength    0..1 how much to apply the weighting (0 = linear)
    */
    inline float weightedAmount(float frequency,
        float amount,
        float strength = 1.0f)
    {
        const float w = weighting(frequency);
        const float mixed = 1.0f + (w - 1.0f) * strength;
        return juce::jlimit(0.0f, 2.0f, amount * mixed);
    }
}