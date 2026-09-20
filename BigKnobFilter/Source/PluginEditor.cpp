#include "PluginProcessor.h"
#include "PluginEditor.h"

BigKnobFilterAudioProcessorEditor::BigKnobFilterAudioProcessorEditor(BigKnobFilterAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    setSize(360, 460);

    titleLabel.setText("BIG KNOB \xe2\x80\x94 11 positions", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffece6d8));
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(knob);
    const int startPos = *processorRef.apvts.getRawParameterValue("position");
    knob.setPosition(startPos);
    knob.onPositionChanged = [this](int pos)
    {
        processorRef.apvts.getParameter("position")->setValueNotifyingHost(
            processorRef.apvts.getParameter("position")->convertTo0to1((float) pos));
    };

    positionLabel.setFont(juce::Font(30.0f, juce::Font::bold));
    positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9d2b));
    positionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(positionLabel);

    nameLabel.setFont(juce::Font(13.0f));
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa89f8c));
    nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel);

    driveLabel.setText("Grain", juce::dontSendNotification);
    driveLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa89f8c));
    addAndMakeVisible(driveLabel);
    driveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    addAndMakeVisible(driveSlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, "drive", driveSlider);

    outputLabel.setText("Sortie", juce::dontSendNotification);
    outputLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa89f8c));
    addAndMakeVisible(outputLabel);
    outputSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    addAndMakeVisible(outputSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, "output", outputSlider);

    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffece6d8));
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, "bypass", bypassButton);

    startTimerHz(30);
}

void BigKnobFilterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff14100c));
}

void BigKnobFilterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(8);

    knob.setBounds(area.removeFromTop(220).withSizeKeepingCentre(220, 220));
    area.removeFromTop(6);
    positionLabel.setBounds(area.removeFromTop(36));
    nameLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);

    auto driveRow = area.removeFromTop(26);
    driveLabel.setBounds(driveRow.removeFromLeft(60));
    driveSlider.setBounds(driveRow);
    area.removeFromTop(8);

    auto outputRow = area.removeFromTop(26);
    outputLabel.setBounds(outputRow.removeFromLeft(60));
    outputSlider.setBounds(outputRow);
    area.removeFromTop(10);

    bypassButton.setBounds(area.removeFromTop(26));
}

void BigKnobFilterAudioProcessorEditor::timerCallback()
{
    const int pos = processorRef.currentPositionForUI.load();
    knob.setPosition(pos);
    positionLabel.setText(juce::String(pos), juce::dontSendNotification);
    nameLabel.setText(kPositions[juce::jlimit(0, 10, pos - 1)].name, juce::dontSendNotification);
}
