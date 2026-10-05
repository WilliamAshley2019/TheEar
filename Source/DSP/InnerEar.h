#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PerceptualWeighting.h"

class InnerEar
{
public:
    static constexpr int kMaxBands = 16;

    void prepare(double sampleRate, int maxBlock, int numChannels)
    {
        sr = sampleRate;
        numCh = juce::jmax(1, numChannels);
        maxBlockSize = maxBlock;

        juce::dsp::ProcessSpec spec{ sampleRate,
                                      (juce::uint32)maxBlock,
                                      (juce::uint32)numCh };

        static const float defaultFreqs[kMaxBands] = {
            40.f,   80.f,   150.f,   300.f,   600.f,   1200.f,  2400.f,  4800.f,
            7000.f, 9500.f, 12000.f, 14000.f, 16000.f, 18000.f, 19000.f, 20000.f
        };
        for (int i = 0; i < kMaxBands; ++i)
            bandFreq[i] = defaultFreqs[i];

        bandpass.clear(); comp.clear(); bandGain.clear();
        for (int b = 0; b < kMaxBands; ++b)
        {
            auto* bp = new juce::OwnedArray<juce::dsp::IIR::Filter<float>>();
            auto* cp = new juce::OwnedArray<juce::dsp::Compressor<float>>();
            auto* gp = new juce::OwnedArray<juce::dsp::IIR::Filter<float>>();
            for (int c = 0; c < numCh; ++c)
            {
                bp->add(new juce::dsp::IIR::Filter<float>());
                cp->add(new juce::dsp::Compressor<float>());
                gp->add(new juce::dsp::IIR::Filter<float>());
                bp->getLast()->prepare(spec);
                cp->getLast()->prepare(spec);
                cp->getLast()->setAttack(5.0f);
                cp->getLast()->setRelease(80.0f);
                gp->getLast()->prepare(spec);
            }
            bandpass.add(bp);
            comp.add(cp);
            bandGain.add(gp);
        }

        scratch.setSize(numCh, maxBlock, false, true, true);

        updateFilters();
        setHairCellAge(0.0f);
    }

    void reset()
    {
        for (auto* b : bandpass) for (auto* f : *b) f->reset();
        for (auto* b : comp)     for (auto* c : *b) c->reset();
        for (auto* b : bandGain) for (auto* g : *b) g->reset();
    }

    void setAmount(float a) { amount = juce::jlimit(0.0f, 1.0f, a); }
    void setPerceptualStrength(float s) { perceptualStrength = juce::jlimit(0.0f, 1.0f, s); }
    void setNumBands(int n) { numBands = juce::jlimit(1, kMaxBands, n); updateFilters(); }

    void setPerBandComp(float a)
    {
        for (int b = 0; b < numBands; ++b)
        {
            const float w = Perceptual::weightedAmount(bandFreq[b], a, perceptualStrength * 0.6f);
            for (auto* c : *comp[b])
            {
                c->setThreshold(juce::jmap(w, 0.0f, 1.0f, 0.0f, -18.0f));
                c->setRatio(1.0f + w * 6.0f);
            }
        }
    }

    void setBandAttack(float ms) { for (auto* b : comp) for (auto* c : *b) c->setAttack(ms); }
    void setBandRelease(float ms) { for (auto* b : comp) for (auto* c : *b) c->setRelease(ms); }

    void setHairCellAge(float age)
    {
        for (int i = 0; i < kMaxBands; ++i)
        {
            const float relFreq = (float)i / (float)(kMaxBands - 1);
            const float w = Perceptual::weightedAmount(bandFreq[i], 1.0f, perceptualStrength * 0.3f);
            const float lossDb = -juce::jmap(age, 0.0f, 1.0f, 0.0f, 30.0f)
                * relFreq * relFreq * amount * w;

            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sr, 2000.0f, 0.7f,
                juce::Decibels::decibelsToGain(lossDb));
            for (auto* g : *bandGain[i]) *g->coefficients = *coeffs;
        }
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        const int n = (int)block.getNumSamples();
        const int ch = (int)block.getNumChannels();

        if (scratch.getNumSamples() < n || scratch.getNumChannels() < ch)
            scratch.setSize(ch, n, false, true, true);

        juce::AudioBuffer<float> output(ch, n);
        output.clear();

        for (int b = 0; b < numBands; ++b)
        {
            for (int c = 0; c < ch; ++c)
                scratch.copyFrom(c, 0, block.getChannelPointer(c), n);

            for (int c = 0; c < juce::jmin(ch, numCh); ++c)
            {
                auto channelBlock = juce::dsp::AudioBlock<float>(
                    scratch.getArrayOfWritePointers() + c, 1, n);
                juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);

                (*bandpass[b])[c]->process(ctx);
                (*comp[b])[c]->process(ctx);
                (*bandGain[b])[c]->process(ctx);
            }

            for (int c = 0; c < ch; ++c)
                output.addFrom(c, 0, scratch, c, 0, n);
        }

        const float norm = 1.0f / std::sqrt((float)numBands);
        for (int c = 0; c < ch; ++c)
            juce::FloatVectorOperations::copy(block.getChannelPointer(c),
                output.getReadPointer(c), n);
        block.multiplyBy(norm);
    }

private:
    void updateFilters()
    {
        for (int b = 0; b < numBands; ++b)
        {
            const float f = juce::jlimit(20.0f, (float)sr * 0.45f, bandFreq[b]);
            const float bw = (b == 0) ? 2.0f : 3.0f;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sr, f, bw);
            for (auto* bp : *bandpass[b]) *bp->coefficients = *coeffs;
        }
    }

    double sr = 44100.0;
    int    numCh = 2, maxBlockSize = 512, numBands = 8;
    float  amount = 1.0f;
    float  perceptualStrength = 1.0f;
    float  bandFreq[kMaxBands]{};
    juce::AudioBuffer<float> scratch;

    juce::OwnedArray<juce::OwnedArray<juce::dsp::IIR::Filter<float>>> bandpass;
    juce::OwnedArray<juce::OwnedArray<juce::dsp::Compressor<float>>>  comp;
    juce::OwnedArray<juce::OwnedArray<juce::dsp::IIR::Filter<float>>> bandGain;
};