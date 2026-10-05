#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PerceptualWeighting.h"

class Brain
{
public:
    void prepare(double sampleRate, int maxBlock, int numChannels)
    {
        sr = sampleRate;
        numCh = juce::jmax(1, numChannels);

        juce::dsp::ProcessSpec spec{ sampleRate,
                                      (juce::uint32)maxBlock,
                                      (juce::uint32)numCh };

        loudnessLow.clear(); loudnessHigh.clear();
        ghostNotch.clear();  monitorLP.clear();

        for (int c = 0; c < numCh; ++c)
        {
            loudnessLow.add(new juce::dsp::IIR::Filter<float>());
            loudnessHigh.add(new juce::dsp::IIR::Filter<float>());
            ghostNotch.add(new juce::dsp::IIR::Filter<float>());
            monitorLP.add(new juce::dsp::IIR::Filter<float>());

            loudnessLow[c]->prepare(spec);
            loudnessHigh[c]->prepare(spec);
            ghostNotch[c]->prepare(spec);
            monitorLP[c]->prepare(spec);
        }

        tinnitusPhase.assign((size_t)numCh, 0.0f);
        updateCoefficients();
    }

    void reset()
    {
        for (auto* f : loudnessLow)  f->reset();
        for (auto* f : loudnessHigh) f->reset();
        for (auto* f : ghostNotch)   f->reset();
        for (auto* f : monitorLP)    f->reset();
        std::fill(tinnitusPhase.begin(), tinnitusPhase.end(), 0.0f);
    }

    void setAmount(float a) { amount = juce::jlimit(0.0f, 1.0f, a); updateCoefficients(); }
    void setPerceptualStrength(float s) { perceptualStrength = juce::jlimit(0.0f, 1.0f, s); updateCoefficients(); }
    void setLoudnessComp(bool on) { loudnessOn = on; updateCoefficients(); }
    void setGhostFundamental(bool on) { ghostOn = on; updateCoefficients(); }
    void setMonitorLevel(float db) { monitorDb = db; updateCoefficients(); }
    void setTinnitusAmount(float a) { tinnitusAmt = juce::jlimit(0.0f, 1.0f, a); }
    void setTinnitusFreq(float f) { tinnitusFreq = juce::jlimit(20.0f, (float)sr * 0.45f, f); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        const int ch = (int)block.getNumChannels();
        const int n = (int)block.getNumSamples();
        const double twoPiOverSr = juce::MathConstants<double>::twoPi / sr;

        for (int c = 0; c < juce::jmin(ch, numCh); ++c)
        {
            auto channelBlock = block.getSingleChannelBlock((size_t)c);
            juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);

            if (loudnessOn)
            {
                loudnessLow[c]->process(ctx);
                loudnessHigh[c]->process(ctx);
            }
            if (ghostOn) ghostNotch[c]->process(ctx);
            monitorLP[c]->process(ctx);

            if (tinnitusAmt > 0.0f)
            {
                auto* w = channelBlock.getChannelPointer(0);
                auto& phase = tinnitusPhase[(size_t)c];
                for (int i = 0; i < n; ++i)
                {
                    w[i] += std::sin((float)phase) * tinnitusAmt * 0.05f * amount;
                    phase += twoPiOverSr * tinnitusFreq;
                    if (phase > juce::MathConstants<double>::twoPi)
                        phase -= juce::MathConstants<double>::twoPi;
                }
            }
        }
    }

private:
    void updateCoefficients()
    {
        const float wLow = Perceptual::weightedAmount(200.0f, amount, perceptualStrength * 1.0f);
        const float wHigh = Perceptual::weightedAmount(6000.0f, amount, perceptualStrength * 1.0f);

        auto lowShelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 200.0f, 0.7f,
            juce::Decibels::decibelsToGain(4.0f * wLow));

        auto highShelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, 6000.0f, 0.7f,
            juce::Decibels::decibelsToGain(3.0f * wHigh));

        auto notch = juce::dsp::IIR::Coefficients<float>::makeNotch(
            sr, 100.0f, 3.0f);

        const float cutoff = juce::jmap(monitorDb, -40.0f, 0.0f, 800.0f, 20000.0f);
        auto lp = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sr, cutoff, 0.7f);

        for (auto* f : loudnessLow)  *f->coefficients = *lowShelf;
        for (auto* f : loudnessHigh) *f->coefficients = *highShelf;
        for (auto* f : ghostNotch)   *f->coefficients = *notch;
        for (auto* f : monitorLP)    *f->coefficients = *lp;
    }

    double sr = 44100.0;
    int    numCh = 2;
    float  amount = 1.0f;
    float  perceptualStrength = 1.0f;
    bool   loudnessOn = false, ghostOn = false;
    float  monitorDb = 0.0f;
    float  tinnitusAmt = 0.0f;
    float  tinnitusFreq = 6000.0f;

    juce::OwnedArray<juce::dsp::IIR::Filter<float>> loudnessLow;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> loudnessHigh;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> ghostNotch;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> monitorLP;
    std::vector<double> tinnitusPhase;
};