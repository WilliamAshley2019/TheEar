#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PerceptualWeighting.h"

class MiddleEar
{
public:
    void prepare(double sampleRate, int maxBlock, int numChannels)
    {
        sr = sampleRate;
        numCh = juce::jmax(1, numChannels);

        juce::dsp::ProcessSpec spec{ sampleRate,
                                      (juce::uint32)maxBlock,
                                      (juce::uint32)numCh };

        tympanic.clear();        impedance.clear();
        eustachianShelf.clear(); eustachianLP.clear();
        tensorTympani.clear();   stapedius.clear();

        for (int c = 0; c < numCh; ++c)
        {
            tympanic.add(new juce::dsp::IIR::Filter<float>());
            impedance.add(new juce::dsp::IIR::Filter<float>());
            eustachianShelf.add(new juce::dsp::IIR::Filter<float>());
            eustachianLP.add(new juce::dsp::IIR::Filter<float>());
            tensorTympani.add(new juce::dsp::Compressor<float>());
            stapedius.add(new juce::dsp::Compressor<float>());

            tympanic[c]->prepare(spec);
            impedance[c]->prepare(spec);
            eustachianShelf[c]->prepare(spec);
            eustachianLP[c]->prepare(spec);
            tensorTympani[c]->prepare(spec);
            stapedius[c]->prepare(spec);

            tensorTympani[c]->setAttack(0.5f);
            tensorTympani[c]->setRelease(50.0f);
            stapedius[c]->setAttack(10.0f);
            stapedius[c]->setRelease(200.0f);
        }

        updateCoefficients();
    }

    void reset()
    {
        for (auto* f : tympanic)        f->reset();
        for (auto* f : impedance)       f->reset();
        for (auto* f : eustachianShelf) f->reset();
        for (auto* f : eustachianLP)    f->reset();
        for (auto* c : tensorTympani)   c->reset();
        for (auto* c : stapedius)       c->reset();
    }

    void setAmount(float a) { amount = juce::jlimit(0.0f, 1.0f, a); updateCoefficients(); }
    void setPerceptualStrength(float s) { perceptualStrength = juce::jlimit(0.0f, 1.0f, s); updateCoefficients(); }
    void setTympanicGainDb(float db) { tympanicGainDb = db; updateCoefficients(); }
    void setImpedanceMatch(float p) { impedanceMatch = p;  updateCoefficients(); }
    void setEustachianBlock(float p)
    {
        eustachianBlock = juce::jlimit(0.0f, 1.0f, p);
        updateCoefficients();
    }

    void setTensorTympani(float a)
    {
        const float w = Perceptual::weightedAmount(1000.0f, a, perceptualStrength * 0.5f);
        for (auto* c : tensorTympani)
        {
            c->setThreshold(juce::jmap(w, 0.0f, 1.0f, 0.0f, -24.0f));
            c->setRatio(1.0f + w * 8.0f);
        }
    }
    void setStapedius(float a) { stapediusAmt = juce::jlimit(0.0f, 1.0f, a); applyStapedius(); }
    void setStapediusAttack(float ms) { stapAttack = ms;   applyStapedius(); }
    void setStapediusRelease(float ms) { stapRelease = ms;  applyStapedius(); }
    void setStapediusSC(float hz) { stapSC = hz;       applyStapedius(); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        const int ch = (int)block.getNumChannels();
        for (int c = 0; c < juce::jmin(ch, numCh); ++c)
        {
            auto channelBlock = block.getSingleChannelBlock((size_t)c);
            juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);
            tympanic[c]->process(ctx);
            impedance[c]->process(ctx);
            tensorTympani[c]->process(ctx);
            stapedius[c]->process(ctx);
            eustachianShelf[c]->process(ctx);
            eustachianLP[c]->process(ctx);
        }
    }

private:
    void applyStapedius()
    {
        const float w = Perceptual::weightedAmount(1500.0f, stapediusAmt, perceptualStrength * 0.5f);
        for (auto* c : stapedius)
        {
            c->setThreshold(juce::jmap(w, 0.0f, 1.0f, 0.0f, -18.0f));
            c->setRatio(1.0f + w * 5.0f);
            c->setAttack(stapAttack);
            c->setRelease(stapRelease);
        }
    }

    void updateCoefficients()
    {
        const float wTymp = Perceptual::weightedAmount(1200.0f, amount, perceptualStrength * 1.0f);
        const float wImp = Perceptual::weightedAmount(800.0f, amount, perceptualStrength * 0.8f);
        const float wEus = Perceptual::weightedAmount(300.0f, amount, perceptualStrength * 0.6f);

        auto tympCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, 1200.0f, 1.2f,
            juce::Decibels::decibelsToGain(tympanicGainDb * wTymp));

        auto impCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 800.0f, 0.7f,
            juce::Decibels::decibelsToGain(impedanceMatch * 6.0f * wImp));

        auto eusShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 300.0f, 0.7f,
            juce::Decibels::decibelsToGain(-eustachianBlock * 10.0f * wEus));

        auto eusLPCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sr,
            juce::jmap(eustachianBlock, 0.0f, 1.0f, 20000.0f, 2500.0f),
            0.7f);

        for (auto* f : tympanic)        *f->coefficients = *tympCoeffs;
        for (auto* f : impedance)       *f->coefficients = *impCoeffs;
        for (auto* f : eustachianShelf) *f->coefficients = *eusShelfCoeffs;
        for (auto* f : eustachianLP)    *f->coefficients = *eusLPCoeffs;
    }

    double sr = 44100.0;
    int    numCh = 2;
    float  amount = 1.0f;
    float  perceptualStrength = 1.0f;
    float  tympanicGainDb = 6.0f;
    float  impedanceMatch = 0.5f;
    float  eustachianBlock = 0.0f;
    float  stapediusAmt = 0.3f;
    float  stapAttack = 10.0f;
    float  stapRelease = 200.0f;
    float  stapSC = 200.0f;

    juce::OwnedArray<juce::dsp::IIR::Filter<float>> tympanic;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> impedance;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> eustachianShelf;
    juce::OwnedArray<juce::dsp::IIR::Filter<float>> eustachianLP;
    juce::OwnedArray<juce::dsp::Compressor<float>>  tensorTympani;
    juce::OwnedArray<juce::dsp::Compressor<float>>  stapedius;
};