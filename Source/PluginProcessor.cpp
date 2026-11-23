#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout JJ8NLSAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Slot 1 Mike Drive: Bipolar [-10.0, +10.0], Default +1.0 (Jaycen Joshua 8x NLS Buss Drum Chain default)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kParamDrive, 1 },
        "Drive",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f),
        1.0f));

    // Sub Punch / Tone (27 Hz Transformer Bump): Bipolar [-10.0, +10.0], Default 0.0 = 100% 1:1 Jaycen Joshua (+6.2 dB @ 27 Hz)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kParamSubPunch, 1 },
        "Tone",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f),
        0.0f));

    // Stereo-linked AutoGain: ON by default
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kParamAutoGain, 1 },
        "AutoGain",
        true));

    // Clean Bypass: OFF (Active) by default
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kParamBypass, 1 },
        "Bypass",
        false));

    return layout;
}

JJ8NLSAudioProcessor::JJ8NLSAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SummingConsoleState", createParameterLayout())
{
    driveParam    = apvts.getRawParameterValue (kParamDrive);
    subPunchParam = apvts.getRawParameterValue (kParamSubPunch);
    autoGainParam = apvts.getRawParameterValue (kParamAutoGain);
    bypassParam   = apvts.getRawParameterValue (kParamBypass);

    // Fixed 1:1 Jaycen Joshua 8x chain by default:
    // 1:Mike (Drv 1.0, Noise Off) -> 2:Spike -> 3:Mike -> 4:Mike -> 5:Mike -> 6:Spike -> 7:Nevo -> 8:Mike
    dspEngine.applyDefaultJaycenJoshuaChain();
    dspEngine.setStereoBalanceMode (jj8nls::StereoBalanceMode::SubAndMidLocked);
    dspEngine.setAutoPeakRmsBalance (false);
    dspEngine.setManualBalanceDb (0.0);
    dspEngine.setOutputTrimDb (0.0);
    syncParametersToEngine();
}

JJ8NLSAudioProcessor::~JJ8NLSAudioProcessor() = default;

const juce::String JJ8NLSAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JJ8NLSAudioProcessor::acceptsMidi() const  { return false; }
bool JJ8NLSAudioProcessor::producesMidi() const { return false; }
bool JJ8NLSAudioProcessor::isMidiEffect() const { return false; }
double JJ8NLSAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int JJ8NLSAudioProcessor::getNumPrograms() { return 1; }
int JJ8NLSAudioProcessor::getCurrentProgram() { return 0; }
void JJ8NLSAudioProcessor::setCurrentProgram (int) {}
const juce::String JJ8NLSAudioProcessor::getProgramName (int) { return "Jaycen Joshua 8x (Default)"; }
void JJ8NLSAudioProcessor::changeProgramName (int, const juce::String&) {}

void JJ8NLSAudioProcessor::syncParametersToEngine() noexcept
{
    const double curDrive = (driveParam != nullptr) ? static_cast<double> (driveParam->load (std::memory_order_relaxed)) : 1.0;
    const double curSub   = (subPunchParam != nullptr) ? static_cast<double> (subPunchParam->load (std::memory_order_relaxed)) : 0.0;
    const bool curAG      = (autoGainParam != nullptr) ? (autoGainParam->load (std::memory_order_relaxed) >= 0.5f) : true;

    if (std::abs (curDrive - lastDriveVal) > 1.0e-4)
    {
        lastDriveVal = curDrive;
        // Default +1.0 = 1.0 Jaycen Joshua drive; negative reduces toward 0.0, positive boosts up to 12.0
        const double mappedDrive = (curDrive >= 0.0)
                                     ? (curDrive * 1.2)
                                     : std::max (0.0, 1.0 + curDrive * 0.1);
        dspEngine.setSlotDrive (0, mappedDrive);
    }

    if (std::abs (curSub - lastSubPunchVal) > 1.0e-4)
    {
        lastSubPunchVal = curSub;
        // 0.0 = 1.0x (+6.2 dB @ 27 Hz 1:1 JJ), -10.0 = 0.0x (Flat Sub), +10.0 = 1.5x (+9.3 dB Sub)
        const double scale = (curSub >= 0.0) ? (1.0 + (curSub / 10.0) * 0.5)
                                             : std::max (0.0, 1.0 + (curSub / 10.0));
        dspEngine.setSubBumpScale (scale);
    }

    if (curAG != lastAutoGainVal)
    {
        lastAutoGainVal = curAG;
        dspEngine.setAutoGainEnabled (curAG);
    }
}

void JJ8NLSAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    dspEngine.prepare (sampleRate);
    lastDriveVal    = -999.0;
    lastSubPunchVal = -999.0;
    syncParametersToEngine();
}

void JJ8NLSAudioProcessor::releaseResources()
{
    dspEngine.reset();
}

bool JJ8NLSAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return (mainOut == juce::AudioChannelSet::mono()
         || mainOut == juce::AudioChannelSet::stereo());
}

void JJ8NLSAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    syncParametersToEngine();

    const bool isBypassed = (bypassParam != nullptr) && (bypassParam->load (std::memory_order_relaxed) >= 0.5f);
    if (isBypassed || totalNumInputChannels <= 0 || numSamples <= 0)
    {
        harmonicActivity.store (0.0f, std::memory_order_relaxed);
        return;
    }

    float blockPeak = 0.0f;

    if (totalNumInputChannels == 1)
    {
        float* ch0 = buffer.getWritePointer (0);
        for (int n = 0; n < numSamples; ++n)
        {
            float sL = ch0[n];
            float sR = sL;
            dspEngine.processSampleStereo (sL, sR);
            ch0[n] = sL;
            blockPeak = std::max (blockPeak, std::abs (sL));
        }
    }
    else
    {
        float* chL = buffer.getWritePointer (0);
        float* chR = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            dspEngine.processSampleStereo (chL[n], chR[n]);
            blockPeak = std::max (blockPeak, std::max (std::abs (chL[n]), std::abs (chR[n])));
        }
    }

    const float db = juce::Decibels::gainToDecibels (blockPeak, -60.0f);
    const float normAct = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
    const float prevAct = harmonicActivity.load (std::memory_order_relaxed);
    harmonicActivity.store (normAct > prevAct ? normAct : (prevAct * 0.85f + normAct * 0.15f),
                            std::memory_order_relaxed);
}

bool JJ8NLSAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* JJ8NLSAudioProcessor::createEditor()
{
    return new FLHeatAudioProcessorEditor (*this);
}

void JJ8NLSAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("skinIndex", getSkinIndex(), nullptr);
    state.setProperty ("editorW",   getSavedEditorWidth(), nullptr);
    state.setProperty ("editorH",   getSavedEditorHeight(), nullptr);
    if (std::unique_ptr<juce::XmlElement> xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void JJ8NLSAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xmlState);
            if (vt.hasProperty ("skinIndex"))
                setSkinIndex (static_cast<int> (vt.getProperty ("skinIndex")));
            if (vt.hasProperty ("editorW") && vt.hasProperty ("editorH"))
                setSavedEditorSize (static_cast<int> (vt.getProperty ("editorW")),
                                    static_cast<int> (vt.getProperty ("editorH")));
            apvts.replaceState (vt);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JJ8NLSAudioProcessor();
}

