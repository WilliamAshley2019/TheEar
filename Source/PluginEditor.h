#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/EarLookAndFeel.h"
#include "gui/AnatomyBackground.h"

class TheEAREditor : public juce::AudioProcessorEditor
{
public:
    explicit TheEAREditor(TheEARProcessor&);
    ~TheEAREditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attach;
    };

    Knob& addKnob(const juce::String& paramID, const juce::String& name);
    void  layoutRow(juce::Rectangle<int> area, std::initializer_list<Knob*> knobs);

    TheEARProcessor& processor;
    EarLookAndFeel   lnf;
    AnatomyBackground background;

    // Global
    juce::ComboBox channelMode;
    std::unique_ptr<ComboAttachment> channelAtt;
    Knob inputTrim, outputTrim, mix, perceptualWeight;

    // Stage amounts
    Knob outerAmount, midAmount, innerAmount, brainAmount;

    // Outer
    Knob canalRes, canalLen, pinnaTilt;

    // Middle
    Knob tympGain, impedance, tensor, stapedius,
        stapAttack, stapRelease, stapSC, eustachian;

    // Inner
    Knob numBands, bandComp, bandAttack, bandRelease, hairAge;

    // Brain
    Knob monitor, tinnitusAmt, tinnitusFreq;
    juce::ToggleButton loudnessBtn{ "Loudness Comp" };
    juce::ToggleButton ghostBtn{ "Ghost Fundamental" };
    std::unique_ptr<ButtonAttachment> loudnessAtt, ghostAtt;

    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TheEAREditor)
};