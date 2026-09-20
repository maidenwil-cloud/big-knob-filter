#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// The 11 fixed positions of the passive dub filter switch.
// Index 0..10 -> knob position 1..11. Index 5 is the flat / bypass detent.
//==============================================================================
enum class FilterShape { LowPass, HighPass, Bypass };

struct KnobPosition
{
    const char* name;
    FilterShape type;
    float freq;   // Hz
    float q;
    float drive;  // 0..1, passive saturation amount
    float loss;   // 0..1, passive signal loss (1 = no loss)
};

static const KnobPosition kPositions[11] = {
    { "Grave profond",      FilterShape::LowPass,  140.0f, 2.4f, 0.75f, 0.62f },
    { "Grave lourd",        FilterShape::LowPass,  260.0f, 1.9f, 0.55f, 0.72f },
    { "Medium-grave",       FilterShape::LowPass,  480.0f, 1.4f, 0.35f, 0.82f },
    { "Medium etouffe",     FilterShape::LowPass,  900.0f, 1.05f, 0.20f, 0.90f },
    { "Presque ouvert",     FilterShape::LowPass,  1900.0f, 0.80f, 0.08f, 0.97f },
    { "Flat - hors circuit",FilterShape::Bypass,   0.0f,   0.0f, 0.00f, 1.00f },
    { "Ouvert-aigu",        FilterShape::HighPass, 220.0f, 0.85f, 0.08f, 0.97f },
    { "Medium perce",       FilterShape::HighPass, 520.0f, 1.30f, 0.22f, 0.88f },
    { "Telephone",          FilterShape::HighPass, 1050.0f,2.00f, 0.42f, 0.76f },
    { "Aigu mordant",       FilterShape::HighPass, 2200.0f,3.10f, 0.62f, 0.64f },
    { "Sifflement dub",     FilterShape::HighPass, 4200.0f,4.60f, 0.85f, 0.50f },
};

//==============================================================================
class BigKnobFilterAudioProcessor final : public juce::AudioProcessor
{
public:
    BigKnobFilterAudioProcessor();
    ~BigKnobFilterAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Big Knob - Passive Filter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // 1..11, matches kPositions index+1
    std::atomic<int> currentPositionForUI { 6 };

private:
    // Per-channel state: filter + saturation + makeup gain
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoeffs = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoeffs> filter;
    juce::dsp::WaveShaper<float> shaper;
    juce::dsp::Gain<float> makeupGain;
    juce::dsp::Gain<float> outputGain;

    double currentSampleRate = 44100.0;
    juce::SmoothedValue<float> smoothedFreq, smoothedQ, smoothedDrive, smoothedLoss;
    int lastPositionIndex = -1;

    void updateShaperCurve(float driveAmount);
    void updateFilterCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BigKnobFilterAudioProcessor)
};
