#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Custom rotary component that snaps to the 11 fixed positions, drawn like
// a stepped rotary switch (brushed metal cap, tick marks, amber pointer).
//==============================================================================
class BigKnobComponent final : public juce::Component
{
public:
    std::function<void(int)> onPositionChanged; // 1..11

    void setPosition(int pos1to11)
    {
        position = juce::jlimit(1, 11, pos1to11);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        auto centre = bounds.getCentre();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        g.setColour(juce::Colour(0xff211b16));
        g.fillEllipse(bounds);
        g.setColour(juce::Colour(0xff3a3228));
        g.drawEllipse(bounds, 1.0f);

        for (int i = 0; i < 11; ++i)
        {
            const float angle = angleForIndex(i);
            const bool centreDetent = (i == 5);
            juce::Point<float> p1 = centre.getPointOnCircumference(radius * 0.90f, angle);
            juce::Point<float> p2 = centre.getPointOnCircumference(radius * 0.98f, angle);
            g.setColour(centreDetent ? juce::Colour(0xff6fb37a) : juce::Colour(0xff7a7160));
            g.drawLine({ p1, p2 }, centreDetent ? 3.0f : 2.0f);

            juce::Point<float> lp = centre.getPointOnCircumference(radius * 0.78f, angle);
            g.setFont(11.0f);
            g.drawText(juce::String(i + 1), juce::Rectangle<float>(20, 14).withCentre(lp), juce::Justification::centred);
        }

        const float capRadius = radius * 0.6f;
        juce::ColourGradient grad(juce::Colour(0xfff2ecdd), centre.x - capRadius * 0.3f, centre.y - capRadius * 0.4f,
                                   juce::Colour(0xff847c69), centre.x + capRadius, centre.y + capRadius, true);
        g.setGradientFill(grad);
        g.fillEllipse(juce::Rectangle<float>(capRadius * 2, capRadius * 2).withCentre(centre));
        g.setColour(juce::Colour(0xff5c5544));
        g.drawEllipse(juce::Rectangle<float>(capRadius * 2, capRadius * 2).withCentre(centre), 1.5f);

        const float pointerAngle = angleForIndex(position - 1);
        juce::Point<float> tip = centre.getPointOnCircumference(capRadius * 0.85f, pointerAngle);
        g.setColour(juce::Colour(0xff231d15));
        g.drawLine({ centre.getPointOnCircumference(capRadius * 0.25f, pointerAngle), tip }, 6.0f);
        g.setColour(juce::Colour(0xffff9d2b));
        g.fillEllipse(juce::Rectangle<float>(8, 8).withCentre(tip));

        g.setColour(juce::Colour(0xff4c4636));
        g.fillEllipse(juce::Rectangle<float>(10, 10).withCentre(centre));
    }

    void mouseDown(const juce::MouseEvent& e) override { updateFromMouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { updateFromMouse(e); }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        setPosition(position + (wheel.deltaY > 0 ? 1 : -1));
        if (onPositionChanged) onPositionChanged(position);
    }

private:
    int position = 6;

    // JUCE's getPointOnCircumference measures angle clockwise from 12 o'clock (0 rad = up).
    static float angleForIndex(int i)
    {
        const float startDeg = -135.0f, endDeg = 135.0f;
        const float deg = startDeg + (endDeg - startDeg) * (i / 10.0f);
        return juce::degreesToRadians(deg);
    }

    void updateFromMouse(const juce::MouseEvent& e)
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        auto p = e.position;
        float deg = juce::radiansToDegrees(std::atan2(p.x - centre.x, -(p.y - centre.y)));
        deg = juce::jlimit(-135.0f, 135.0f, deg);
        const float frac = (deg + 135.0f) / 270.0f;
        const int idx = juce::roundToInt(frac * 10.0f);
        setPosition(idx + 1);
        if (onPositionChanged) onPositionChanged(position);
    }
};

//==============================================================================
class BigKnobFilterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit BigKnobFilterAudioProcessorEditor(BigKnobFilterAudioProcessor&);
    ~BigKnobFilterAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BigKnobFilterAudioProcessor& processorRef;

    BigKnobComponent knob;
    juce::Label positionLabel, nameLabel, titleLabel;
    juce::Slider driveSlider, outputSlider;
    juce::Label driveLabel, outputLabel;
    juce::ToggleButton bypassButton { "Bypass" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment, outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BigKnobFilterAudioProcessorEditor)
};
