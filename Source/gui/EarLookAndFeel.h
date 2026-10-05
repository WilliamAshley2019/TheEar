#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class EarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EarLookAndFeel()
    {
        // Warm anatomical palette
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffc46a5a)); // flesh
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2b1f1c));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xfff3e0d4)); // bone
        setColour(juce::Label::textColourId, juce::Colour(0xfff3e0d4));
        setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff3e0d4));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffc46a5a));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2b1f1c));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff3e0d4));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff5b3a33));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
        float sliderPos, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int>(x, y, w, h).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Track
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f,
            0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.strokePath(track, juce::PathStrokeType(3.0f));

        // Fill
        juce::Path fill;
        fill.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f,
            0.0f, rotaryStartAngle, angle, true);
        g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(fill, juce::PathStrokeType(3.0f));

        // Knob body
        g.setColour(juce::Colour(0xff1a1310));
        g.fillEllipse(centre.x - radius * 0.6f, centre.y - radius * 0.6f,
            radius * 1.2f, radius * 1.2f);

        // Indicator
        juce::Path pointer;
        pointer.addRectangle(-1.5f, -radius * 0.55f, 3.0f, radius * 0.35f);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre));
    }
};