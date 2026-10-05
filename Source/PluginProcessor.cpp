#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
TheEARProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&](const char* id, const char* name,
        Range<float> range, float def)
        {
            layout.add(std::make_unique<AudioParameterFloat>(id, name, range, def));
        };

    // ── Global ─────────────────────────────────────────────
    layout.add(std::make_unique<AudioParameterChoice>(
        "channelMode", "Channel Mode",
        StringArray{ "Stereo", "Mono (sum)", "Mono (L only)", "Mono (R only)" },
        0));
    addFloat("inputTrim", "Input Trim", { -24.0f, 24.0f }, 0.0f);
    addFloat("outputTrim", "Output Trim", { -24.0f, 24.0f }, 0.0f);
    addFloat("mix", "Mix", { 0.0f, 1.0f }, 1.0f);
    addFloat("perceptualWeight", "Perceptual Fade", { 0.0f, 1.0f }, 1.0f);

    // ── Outer Ear ──────────────────────────────────────────
    addFloat("outerAmount", "Outer Amount", { 0.0f, 1.0f }, 1.0f);
    addFloat("canalRes", "Canal Resonance", { 0.0f, 20.0f }, 15.0f);
    addFloat("canalLen", "Canal Length", { 2.0f, 3.5f }, 2.5f);
    addFloat("pinnaTilt", "Pinna Tilt", { -1.0f, 1.0f }, 0.0f);

    // ── Middle Ear ─────────────────────────────────────────
    addFloat("midAmount", "Middle Amount", { 0.0f, 1.0f }, 1.0f);
    addFloat("tympGain", "Tympanic Gain", { 0.0f, 12.0f }, 6.0f);
    addFloat("impedance", "Impedance Match", { 0.0f, 1.0f }, 0.5f);
    addFloat("tensor", "Tensor Tympani", { 0.0f, 1.0f }, 0.3f);
    addFloat("stapedius", "Stapedius", { 0.0f, 1.0f }, 0.3f);
    addFloat("stapAttack", "Stap. Attack", { 0.1f, 200.0f }, 10.0f);
    addFloat("stapRelease", "Stap. Release", { 10.0f, 2000.0f }, 200.0f);
    addFloat("stapSidechain", "Stap. SC HP", { 20.0f, 2000.0f }, 200.0f);
    addFloat("eustachian", "Eustachian Block", { 0.0f, 1.0f }, 0.0f);

    // ── Inner Ear ──────────────────────────────────────────
    addFloat("innerAmount", "Inner Amount", { 0.0f, 1.0f }, 1.0f);
    addFloat("numBands", "Hair Cell Bands", { 1.0f, 16.0f }, 8.0f);
    addFloat("bandComp", "Per-band Comp", { 0.0f, 1.0f }, 0.5f);
    addFloat("bandAttack", "Band Attack", { 1.0f, 100.0f }, 5.0f);
    addFloat("bandRelease", "Band Release", { 20.0f, 500.0f }, 80.0f);
    addFloat("hairAge", "Hair Cell Age", { 0.0f, 1.0f }, 0.0f);

    // ── Brain ──────────────────────────────────────────────
    addFloat("brainAmount", "Brain Amount", { 0.0f, 1.0f }, 1.0f);
    layout.add(std::make_unique<AudioParameterBool>("loudness", "Loudness Comp", false));
    layout.add(std::make_unique<AudioParameterBool>("ghost", "Ghost Fundamental", false));
    addFloat("monitor", "Monitor Level", { -40.0f, 0.0f }, 0.0f);

    // ── Tinnitus ───────────────────────────────────────────
    addFloat("tinnitusAmt", "Tinnitus", { 0.0f, 1.0f }, 0.0f);
    addFloat("tinnitusFreq", "Tinnitus Freq", { 1000.0f, 12000.0f }, 6000.0f);

    return layout;
}

TheEARProcessor::TheEARProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createLayout())
{
}

void TheEARProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int ch = juce::jmax(1, getTotalNumOutputChannels());
    outerEar.prepare(sampleRate, samplesPerBlock, ch);
    middleEar.prepare(sampleRate, samplesPerBlock, ch);
    innerEar.prepare(sampleRate, samplesPerBlock, ch);
    brain.prepare(sampleRate, samplesPerBlock, ch);

    dryBuffer.setSize(ch, samplesPerBlock, false, true, true);
    lastChannels = ch;
}

bool TheEARProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::mono())
        || (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo())
        || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo())
        || (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::mono());
}

void TheEARProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    if (n == 0 || ch == 0) return;

    // Defensive resize (FL Studio channel quirks)
    if (ch != lastChannels || dryBuffer.getNumSamples() < n)
    {
        dryBuffer.setSize(ch, juce::jmax(n, dryBuffer.getNumSamples()),
            false, true, true);
        lastChannels = ch;
    }

    // 1) Dry copy
    for (int c = 0; c < ch; ++c)
        dryBuffer.copyFrom(c, 0, buffer, c, 0, n);

    // 2) Channel mode
    const int channelMode = (int)*apvts.getRawParameterValue("channelMode");
    if (channelMode != 0 && ch >= 2)
    {
        switch (channelMode)
        {
        case 1: // sum
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (buffer.getReadPointer(0)[i]
                    + buffer.getReadPointer(1)[i]);
                buffer.getWritePointer(0)[i] = m;
                buffer.getWritePointer(1)[i] = m;
            }
            break;
        case 2: // L only
            for (int i = 0; i < n; ++i)
                buffer.getWritePointer(1)[i] = buffer.getReadPointer(0)[i];
            break;
        case 3: // R only
            for (int i = 0; i < n; ++i)
                buffer.getWritePointer(0)[i] = buffer.getReadPointer(1)[i];
            break;
        default: break;
        }
    }

    // 3) Global input trim
    buffer.applyGain(juce::Decibels::decibelsToGain(
        (float)*apvts.getRawParameterValue("inputTrim")));

    // 4) Read shared perceptual strength
    const float perceptual = (float)*apvts.getRawParameterValue("perceptualWeight");

    // 5) Push parameters into stages
    outerEar.setPerceptualStrength(perceptual);
    outerEar.setAmount((float)*apvts.getRawParameterValue("outerAmount"));
    outerEar.setCanalResonanceDb((float)*apvts.getRawParameterValue("canalRes"));
    outerEar.setCanalLengthCm((float)*apvts.getRawParameterValue("canalLen"));
    outerEar.setPinnaTilt((float)*apvts.getRawParameterValue("pinnaTilt"));

    middleEar.setPerceptualStrength(perceptual);
    middleEar.setAmount((float)*apvts.getRawParameterValue("midAmount"));
    middleEar.setTympanicGainDb((float)*apvts.getRawParameterValue("tympGain"));
    middleEar.setImpedanceMatch((float)*apvts.getRawParameterValue("impedance"));
    middleEar.setTensorTympani((float)*apvts.getRawParameterValue("tensor"));
    middleEar.setStapedius((float)*apvts.getRawParameterValue("stapedius"));
    middleEar.setStapediusAttack((float)*apvts.getRawParameterValue("stapAttack"));
    middleEar.setStapediusRelease((float)*apvts.getRawParameterValue("stapRelease"));
    middleEar.setStapediusSC((float)*apvts.getRawParameterValue("stapSidechain"));
    middleEar.setEustachianBlock((float)*apvts.getRawParameterValue("eustachian"));

    innerEar.setPerceptualStrength(perceptual);
    innerEar.setAmount((float)*apvts.getRawParameterValue("innerAmount"));
    innerEar.setNumBands((int)*apvts.getRawParameterValue("numBands"));
    innerEar.setPerBandComp((float)*apvts.getRawParameterValue("bandComp"));
    innerEar.setBandAttack((float)*apvts.getRawParameterValue("bandAttack"));
    innerEar.setBandRelease((float)*apvts.getRawParameterValue("bandRelease"));
    innerEar.setHairCellAge((float)*apvts.getRawParameterValue("hairAge"));

    brain.setPerceptualStrength(perceptual);
    brain.setAmount((float)*apvts.getRawParameterValue("brainAmount"));
    brain.setLoudnessComp(*apvts.getRawParameterValue("loudness") > 0.5f);
    brain.setGhostFundamental(*apvts.getRawParameterValue("ghost") > 0.5f);
    brain.setMonitorLevel((float)*apvts.getRawParameterValue("monitor"));
    brain.setTinnitusAmount((float)*apvts.getRawParameterValue("tinnitusAmt"));
    brain.setTinnitusFreq((float)*apvts.getRawParameterValue("tinnitusFreq"));

    // 6) Process
    juce::dsp::AudioBlock<float> block(buffer);
    outerEar.process(block);
    middleEar.process(block);
    innerEar.process(block);
    brain.process(block);

    // 7) Wet/dry mix
    const float mixAmt = *apvts.getRawParameterValue("mix");
    for (int c = 0; c < ch; ++c)
    {
        auto* w = buffer.getWritePointer(c);
        auto* d = dryBuffer.getReadPointer(c);
        for (int i = 0; i < n; ++i)
            w[i] = w[i] * mixAmt + d[i] * (1.0f - mixAmt);
    }

    // 8) Output trim
    buffer.applyGain(juce::Decibels::decibelsToGain(
        (float)*apvts.getRawParameterValue("outputTrim")));
}

juce::AudioProcessorEditor* TheEARProcessor::createEditor()
{
    return new TheEAREditor(*this);
}

void TheEARProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TheEARProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TheEARProcessor();
}