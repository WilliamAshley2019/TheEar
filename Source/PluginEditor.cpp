#include "PluginEditor.h"

TheEAREditor::TheEAREditor(TheEARProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    setOpaque(true);

    // Background must be added first so it sits at the bottom of z-order.
    addAndMakeVisible(background);
    background.setInterceptsMouseClicks(false, false);

    // ── Global ─────────────────────────────────────────────
    addAndMakeVisible(channelMode);
    channelMode.addItemList({ "Stereo", "Mono Sum", "Mono L", "Mono R" }, 1);
    channelAtt = std::make_unique<ComboAttachment>(
        processor.apvts, "channelMode", channelMode);

    addKnob("inputTrim", "In Trim");
    addKnob("outputTrim", "Out Trim");
    addKnob("mix", "Mix");
    addKnob("perceptualWeight", "Perceptual");

    // ── Stage amounts ──────────────────────────────────────
    addKnob("outerAmount", "Outer Amt");
    addKnob("midAmount", "Middle Amt");
    addKnob("innerAmount", "Inner Amt");
    addKnob("brainAmount", "Brain Amt");

    // ── Outer ──────────────────────────────────────────────
    addKnob("canalRes", "Canal Res");
    addKnob("canalLen", "Canal Len");
    addKnob("pinnaTilt", "Pinna");

    // ── Middle ─────────────────────────────────────────────
    addKnob("tympGain", "Tympanic");
    addKnob("impedance", "Impedance");
    addKnob("tensor", "Tensor");
    addKnob("stapedius", "Stapedius");
    addKnob("stapAttack", "Stap A");
    addKnob("stapRelease", "Stap R");
    addKnob("stapSidechain", "Stap SC");
    addKnob("eustachian", "Eustachian");

    // ── Inner ──────────────────────────────────────────────
    addKnob("numBands", "Bands");
    addKnob("bandComp", "Band Comp");
    addKnob("bandAttack", "Band A");
    addKnob("bandRelease", "Band R");
    addKnob("hairAge", "Hair Age");

    // ── Brain ──────────────────────────────────────────────
    addKnob("monitor", "Monitor");
    addKnob("tinnitusAmt", "Tinnitus");
    addKnob("tinnitusFreq", "Tinn Freq");

    // ── Toggles ────────────────────────────────────────────
    addAndMakeVisible(loudnessBtn);
    addAndMakeVisible(ghostBtn);
    loudnessAtt = std::make_unique<ButtonAttachment>(
        processor.apvts, "loudness", loudnessBtn);
    ghostAtt = std::make_unique<ButtonAttachment>(
        processor.apvts, "ghost", ghostBtn);

    // Ensure background stays behind everything, no matter what
    background.toBack();

    setSize(1200, 820);
}

TheEAREditor::~TheEAREditor()
{
    setLookAndFeel(nullptr);
}

TheEAREditor::Knob& TheEAREditor::addKnob(const juce::String& id,
    const juce::String& name)
{
    auto k = std::make_unique<Knob>();

    k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    k->slider.setColour(juce::Slider::textBoxTextColourId,
        juce::Colour(0xfff3e0d4));
    k->slider.setColour(juce::Slider::textBoxBackgroundColourId,
        juce::Colours::transparentBlack);
    k->slider.setColour(juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);
    k->slider.setVisible(true);

    k->label.setText(name, juce::dontSendNotification);
    k->label.setJustificationType(juce::Justification::centred);
    k->label.setFont(juce::FontOptions(12.0f));
    k->label.setColour(juce::Label::textColourId, juce::Colour(0xfff3e0d4));
    k->label.setVisible(true);

    k->attach = std::make_unique<SliderAttachment>(processor.apvts, id, k->slider);

    addAndMakeVisible(k->slider);
    addAndMakeVisible(k->label);

    knobs.push_back(std::move(k));
    return *knobs.back();
}

void TheEAREditor::layoutRow(juce::Rectangle<int> area,
    std::initializer_list<Knob*> row)
{
    const int n = (int)row.size();
    if (n <= 0 || area.getWidth() <= 0 || area.getHeight() <= 0) return;

    const int w = area.getWidth() / n;
    for (auto* k : row)
    {
        auto cell = area.removeFromLeft(w).reduced(6);
        if (cell.getWidth() <= 0 || cell.getHeight() <= 0) continue;

        // Reserve top 18px for the label
        auto labelArea = cell.removeFromTop(18);
        k->label.setBounds(labelArea);
        k->slider.setBounds(cell);   // remaining area is the slider
    }
}

void TheEAREditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff120d0b));

    // Section headers so users know what they're looking at
    g.setColour(juce::Colour(0xfff3e0d4).withAlpha(0.85f));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));

    auto b = getLocalBounds();
    auto global = b.removeFromTop(150);
    g.drawText("GLOBAL", global.removeFromLeft(200).reduced(10),
        juce::Justification::topLeft);
    g.drawText("STAGE AMOUNTS", global.removeFromLeft(400).reduced(10),
        juce::Justification::topLeft);

    auto outer = b.removeFromTop(140);
    g.drawText("OUTER EAR", outer.removeFromLeft(200).reduced(10),
        juce::Justification::topLeft);

    auto mid = b.removeFromTop(170);
    g.drawText("MIDDLE EAR", mid.removeFromLeft(200).reduced(10),
        juce::Justification::topLeft);

    auto inner = b.removeFromTop(170);
    g.drawText("INNER EAR", inner.removeFromLeft(200).reduced(10),
        juce::Justification::topLeft);

    auto brain = b.removeFromTop(150);
    g.drawText("BRAIN", brain.removeFromLeft(200).reduced(10),
        juce::Justification::topLeft);
}

void TheEAREditor::resized()
{
    auto area = getLocalBounds();
    background.setBounds(area);

    // Row 1: GLOBAL (combo + 4 knobs) + STAGE AMOUNTS (4 knobs)
    auto globalRow = area.removeFromTop(150).reduced(10);
    auto modeCell = globalRow.removeFromLeft(170).reduced(6);
    channelMode.setBounds(modeCell.removeFromTop(28));
    layoutRow(globalRow, { &inputTrim, &outputTrim, &mix, &perceptualWeight,
                            &outerAmount, &midAmount,
                            &innerAmount, &brainAmount });

    // Row 2: OUTER EAR
    auto outerRow = area.removeFromTop(140).reduced(10);
    layoutRow(outerRow, { &canalRes, &canalLen, &pinnaTilt });

    // Row 3: MIDDLE EAR
    auto midRow = area.removeFromTop(170).reduced(10);
    layoutRow(midRow, { &tympGain, &impedance, &tensor, &stapedius,
                         &stapAttack, &stapRelease, &stapSC, &eustachian });

    // Row 4: INNER EAR
    auto innerRow = area.removeFromTop(170).reduced(10);
    layoutRow(innerRow, { &numBands, &bandComp, &bandAttack,
                           &bandRelease, &hairAge });

    // Row 5: BRAIN + toggles
    auto brainRow = area.removeFromTop(150).reduced(10);
    auto toggleCol = brainRow.removeFromRight(180);
    layoutRow(brainRow, { &monitor, &tinnitusAmt, &tinnitusFreq });

    loudnessBtn.setBounds(toggleCol.removeFromTop(30).reduced(4));
    ghostBtn.setBounds(toggleCol.removeFromTop(30).reduced(4));

    // Always keep the background behind everything
    background.toBack();
}