#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PerceptualWeighting.h"

class OuterEar
{
public:
    void prepare(double sampleRate, int maxBlock, int numChannels)
    {
        sr = sampleRate;
        numCh = juce::jmax(1, numChannels);

        juce::dsp::ProcessSpec spec{ sampleRate,
                                      (juce::uint32)maxBlock,
                                      (juce::uint32)numCh };

        canalFilter.clear();
        pinnaFilter.clear();
        for (int c = 0; c < numCh; ++c)
        {
            canalFilter.add(new juce::dsp::IIR::Filter<float>());
            pinnaFilter.add(new juce::dsp::IIR::Filter<float>());
            canalFilter[c]->prepare(spec);
            pinnaFilter[c]->prepare(spec);
        }
        updateCoefficients();
    }

    void reset()
    {
        for (auto* f : canalFilter) f->reset();
        for (auto* f : pinnaFilter) f->reset();
    }

    void setAmount(float a) { amount = juce::jlimit(0.0f, 1.0f, a); updateCoefficients(); }
    void setPerceptualStrength(float s) { perceptualStrength = juce::jlimit(0.0f, 1.0f, s); updateCoefficients(); }
    void setCanalResonanceDb(float db) { canalGainDb = db; updateCoefficients(); }
    void setCanalLengthCm(float cm) { canalLength = cm; updateCoefficients(); }
    void setPinnaTilt(float t) { pinnaTilt = t;    updateCoefficients(); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        const int ch = (int)block.getNumChannels();
        for (int c = 0; c < juce::jmin(ch, numCh); ++c)
        {
            auto channelBlock = block.getSingleChannelBlock((size_t)c);
            juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);
            canalFilter[c]->process(ctx);
            pinnaFilter[c]->process(ctx);
        }
    }

private:
    void updateCoefficients()
    {
        const float L = juce::jmax(0.5f, canalLength) / 100.0f;
        const float f0 = 343.0f / (4.0f * L);
        const float Q = 2.0f + (3.5f - canalLength);

        // Fletcher-Munson-weighted fade
        const float wCanal = Perceptual::weightedAmount(f0, amount, perceptualStrength * 0.7f);
        const float wPinna = Perceptual::weightedAmount(6000.0f, amount, perceptualStrength * 0.5f);

        const float canalGain = canalGainDb * wCanal;
        const float pinnaGain = pinnaTilt * 6.0f * wPinna;

        auto canalCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr,
            juce::jlimit(500.0f, 8000.0f, f0),
            Q,
            juce::Decibels::decibelsToGain(canalGain));

        auto pinnaCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, 4000.0f, 0.7f,
            juce::Decibels::decibelsToGain(pinnaGain));

        for (auto* f : canalFilter) *f->coefficients = *canalCoeffs;
        for (auto* f : pinnaFilter) *f->coefficients = *pinnaCoeffs;
    }

    double sr = 44100.0;
    int    numCh = 2;
    float  amount = 1.0f;
    float  perceptualStrength = 1.0f;
    float  canalGainDb = 15.0f;
    float  canalLength = 2.5f;
    float  pinnaTilt = 0.0f;

    juce::OwnedArray<juce::dsp::IIR::Filter<float>> canalFilter;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> pinnaFilter;
};