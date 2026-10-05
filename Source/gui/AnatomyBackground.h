#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// A stylized ear anatomy diagram drawn with paths.
// Inspired by Figure 1 from the SOS article.
class AnatomyBackground : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff120d0b));

        auto b = getLocalBounds().toFloat().reduced(20.0f);
        const float w = b.getWidth();
        const float h = b.getHeight();

        // --- Outer ear (pinna) ---
        juce::Path pinna;
        pinna.startNewSubPath(b.getX() + w * 0.05f, b.getY() + h * 0.3f);
        pinna.cubicTo(b.getX() + w * 0.02f, b.getY() + h * 0.6f,
            b.getX() + w * 0.08f, b.getY() + h * 0.9f,
            b.getX() + w * 0.15f, b.getY() + h * 0.85f);
        pinna.cubicTo(b.getX() + w * 0.18f, b.getY() + h * 0.7f,
            b.getX() + w * 0.18f, b.getY() + h * 0.4f,
            b.getX() + w * 0.15f, b.getY() + h * 0.3f);
        g.setColour(juce::Colour(0xffc46a5a).withAlpha(0.85f));
        g.fillPath(pinna);

        // --- Auditory canal ---
        juce::Path canal;
        canal.startNewSubPath(b.getX() + w * 0.15f, b.getY() + h * 0.55f);
        canal.lineTo(b.getX() + w * 0.35f, b.getY() + h * 0.55f);
        canal.lineTo(b.getX() + w * 0.35f, b.getY() + h * 0.65f);
        canal.lineTo(b.getX() + w * 0.15f, b.getY() + h * 0.65f);
        canal.closeSubPath();
        g.setColour(juce::Colour(0xff8a3a2c));
        g.fillPath(canal);

        // --- Eardrum ---
        g.setColour(juce::Colour(0xfff3e0d4));
        g.fillEllipse(b.getX() + w * 0.34f, b.getY() + h * 0.54f,
            w * 0.02f, h * 0.12f);

        // --- Ossicles (malleus, incus, stapes) ---
        g.setColour(juce::Colour(0xfff3e0d4));
        // Malleus
        g.fillEllipse(b.getX() + w * 0.37f, b.getY() + h * 0.54f, w * 0.02f, h * 0.05f);
        // Incus
        g.fillEllipse(b.getX() + w * 0.40f, b.getY() + h * 0.58f, w * 0.02f, h * 0.05f);
        // Stapes
        g.fillEllipse(b.getX() + w * 0.43f, b.getY() + h * 0.62f, w * 0.02f, h * 0.04f);

        // --- Cochlea (snail) ---
        juce::Path cochlea;
        auto cochleaCentre = juce::Point<float>(b.getX() + w * 0.6f,
            b.getY() + h * 0.55f);
        const float maxR = h * 0.18f;
        for (float a = 0.0f; a < juce::MathConstants<float>::twoPi * 2.5f; a += 0.05f)
        {
            const float r = maxR * (1.0f - a / (juce::MathConstants<float>::twoPi * 2.5f));
            const float px = cochleaCentre.x + std::cos(a) * r;
            const float py = cochleaCentre.y + std::sin(a) * r;
            if (a == 0.0f) cochlea.startNewSubPath(px, py);
            else           cochlea.lineTo(px, py);
        }
        g.setColour(juce::Colour(0xffe0a878));
        g.strokePath(cochlea, juce::PathStrokeType(4.0f));

        // --- Cochlear nerve ---
        juce::Path nerve;
        nerve.startNewSubPath(cochleaCentre.x + maxR, cochleaCentre.y);
        nerve.cubicTo(b.getX() + w * 0.75f, b.getY() + h * 0.55f,
            b.getX() + w * 0.85f, b.getY() + h * 0.45f,
            b.getX() + w * 0.95f, b.getY() + h * 0.45f);
        g.setColour(juce::Colour(0xfff3e0d4).withAlpha(0.7f));
        g.strokePath(nerve, juce::PathStrokeType(2.5f));

        // --- Section labels ---
        g.setColour(juce::Colour(0xfff3e0d4).withAlpha(0.5f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::italic));
        g.drawText("OUTER", b.withTrimmedLeft(w * 0.05f).withWidth(w * 0.2f)
            .withY(b.getY() + h * 0.92f).withHeight(20.0f),
            juce::Justification::centred);
        g.drawText("MIDDLE", b.withTrimmedLeft(w * 0.32f).withWidth(w * 0.2f)
            .withY(b.getY() + h * 0.92f).withHeight(20.0f),
            juce::Justification::centred);
        g.drawText("INNER", b.withTrimmedLeft(w * 0.55f).withWidth(w * 0.2f)
            .withY(b.getY() + h * 0.92f).withHeight(20.0f),
            juce::Justification::centred);
        g.drawText("BRAIN", b.withTrimmedLeft(w * 0.78f).withWidth(w * 0.2f)
            .withY(b.getY() + h * 0.92f).withHeight(20.0f),
            juce::Justification::centred);
    }
};