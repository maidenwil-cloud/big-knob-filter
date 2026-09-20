#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BigKnobFilterAudioProcessor::BigKnobFilterAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BigKnobFilterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "position", 1 }, "Position", 1, 11, 6));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 }, "Grain",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.55f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> { 0.0f, 1.5f }, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

void BigKnobFilterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    filter.prepare(spec);
    shaper.prepare(spec);
    makeupGain.prepare(spec);
    outputGain.prepare(spec);

    smoothedFreq.reset(sampleRate, 0.03);
    smoothedQ.reset(sampleRate, 0.03);
    smoothedDrive.reset(sampleRate, 0.03);
    smoothedLoss.reset(sampleRate, 0.03);

    const auto& p = kPositions[5];
    smoothedFreq.setCurrentAndTargetValue(1000.0f);
    smoothedQ.setCurrentAndTargetValue(0.5f);
    smoothedDrive.setCurrentAndTargetValue(p.drive);
    smoothedLoss.setCurrentAndTargetValue(p.loss);

    lastPositionIndex = -1;
    updateFilterCoefficients();
}

bool BigKnobFilterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == mono || in == stereo;
}

void BigKnobFilterAudioProcessor::updateShaperCurve(float driveAmount)
{
    // Soft-clip tanh curve, amount scales the drive knob * position drive
    shaper.functionToUse = [driveAmount](float x)
    {
        const float k = driveAmount * 35.0f + 0.001f;
        return std::tanh(k * x) / std::tanh(k);
    };
}

void BigKnobFilterAudioProcessor::updateFilterCoefficients()
{
    const int posIndex = juce::jlimit(0, 10, (int) *apvts.getRawParameterValue("position") - 1);
    const auto& p = kPositions[posIndex];
    currentPositionForUI.store(posIndex + 1);

    const float driveSlider = *apvts.getRawParameterValue("drive");
    const bool bypassed = *apvts.getRawParameterValue("bypass") > 0.5f;

    float targetFreq, targetQ;
    if (p.type == FilterShape::Bypass || bypassed)
    {
        targetFreq = 1000.0f;
        targetQ = 0.5f; // allpass-ish neutral, coefficients set to allpass below
    }
    else
    {
        targetFreq = p.freq;
        targetQ = p.q;
    }

    smoothedFreq.setTargetValue(targetFreq);
    smoothedQ.setTargetValue(targetQ);
    smoothedDrive.setTargetValue(p.drive * driveSlider);
    smoothedLoss.setTargetValue(bypassed ? 1.0f : p.loss);

    lastPositionIndex = posIndex;

    const double sr = currentSampleRate > 0 ? currentSampleRate : 44100.0;
    const float freq = juce::jlimit(20.0f, (float) (sr * 0.45), smoothedFreq.getCurrentValue());
    const float q = juce::jmax(0.05f, smoothedQ.getCurrentValue());

    if (bypassed || p.type == FilterShape::Bypass)
        *filter.state = *IIRCoeffs::makeAllPass(sr, freq, q);
    else if (p.type == FilterShape::LowPass)
        *filter.state = *IIRCoeffs::makeLowPass(sr, freq, q);
    else
        *filter.state = *IIRCoeffs::makeHighPass(sr, freq, q);

    updateShaperCurve(smoothedDrive.getCurrentValue());

    const float driveNow = smoothedDrive.getCurrentValue();
    const float lossNow = smoothedLoss.getCurrentValue();
    const float makeup = bypassed ? 1.0f : (lossNow * (0.55f + driveSlider * 0.35f) + 0.35f);
    makeupGain.setGainLinear(makeup);

    outputGain.setGainLinear(*apvts.getRawParameterValue("output"));
}

void BigKnobFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int posIndex = juce::jlimit(0, 10, (int) *apvts.getRawParameterValue("position") - 1);
    if (posIndex != lastPositionIndex)
        updateFilterCoefficients();
    else
    {
        // keep gain/output reactive to slider moves every block without a full coeff rebuild
        const float driveSlider = *apvts.getRawParameterValue("drive");
        const bool bypassed = *apvts.getRawParameterValue("bypass") > 0.5f;
        const auto& p = kPositions[posIndex];
        const float lossNow = bypassed ? 1.0f : p.loss;
        makeupGain.setGainLinear(bypassed ? 1.0f : (lossNow * (0.55f + driveSlider * 0.35f) + 0.35f));
        outputGain.setGainLinear(*apvts.getRawParameterValue("output"));
        updateShaperCurve(p.drive * driveSlider);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    filter.process(context);
    shaper.process(context);
    makeupGain.process(context);
    outputGain.process(context);
}

void BigKnobFilterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
}

void BigKnobFilterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* BigKnobFilterAudioProcessor::createEditor()
{
    return new BigKnobFilterAudioProcessorEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BigKnobFilterAudioProcessor();
}
