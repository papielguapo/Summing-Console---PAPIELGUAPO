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

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            2,
            i + 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true);
    }

    // Fixed 1:1 Jaycen Joshua 8x chain by default:
    // 1:Mike (Drv 1.0, Noise Off) -> 2:Spike -> 3:Mike -> 4:Mike -> 5:Mike -> 6:Spike -> 7:Nevo -> 8:Mike
    dspEngine.applyDefaultJaycenJoshuaChain();
    dspEngine.setStereoBalanceMode (jj8nls::StereoBalanceMode::SubAndMidLocked);
    dspEngine.setAutoPeakRmsBalance (false);
    dspEngine.setManualBalanceDb (0.0);
    dspEngine.setOutputTrimDb (0.0);
    driveSmoothed.setCurrentAndTargetValue (1.0);
    dspEngine.setBipolarDrive (1.0);
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

void JJ8NLSAudioProcessor::applyOversamplingConfiguration (int osMode)
{
    const int clampedMode = juce::jlimit (0, 3, osMode);
    activeOSMode = clampedMode;

    const int osFactor = 1 << clampedMode;
    const double effectiveSampleRate = std::max (8000.0, baseSampleRate * static_cast<double> (osFactor));

    dspEngine.prepare (effectiveSampleRate);

    const double curDrive = (driveParam != nullptr) ? static_cast<double> (driveParam->load (std::memory_order_relaxed)) : 1.0;
    driveSmoothed.reset (effectiveSampleRate, 0.015);
    driveSmoothed.setCurrentAndTargetValue (curDrive);
    dspEngine.setBipolarDrive (curDrive);
    lastDriveVal = curDrive;

    int newLatency = 0;
    if (clampedMode >= 1 && clampedMode <= 3)
    {
        auto& os = oversamplers[static_cast<size_t> (clampedMode - 1)];
        if (os != nullptr)
        {
            os->reset();
            newLatency = static_cast<int> (std::lround (os->getLatencyInSamples()));
        }
    }
    setLatencySamples (newLatency);
}

void JJ8NLSAudioProcessor::syncParametersToEngine() noexcept
{
    const double curDrive = (driveParam != nullptr) ? static_cast<double> (driveParam->load (std::memory_order_relaxed)) : 1.0;
    const double curSub   = (subPunchParam != nullptr) ? static_cast<double> (subPunchParam->load (std::memory_order_relaxed)) : 0.0;
    const bool curAG      = (autoGainParam != nullptr) ? (autoGainParam->load (std::memory_order_relaxed) >= 0.5f) : true;

    if (std::abs (curDrive - lastDriveVal) > 1.0e-4)
    {
        lastDriveVal = curDrive;
        driveSmoothed.setTargetValue (curDrive);
    }

    if (std::abs (curSub - lastSubPunchVal) > 1.0e-4)
    {
        lastSubPunchVal = curSub;
        dspEngine.setBipolarTone (curSub);
    }

    if (curAG != lastAutoGainVal)
    {
        lastAutoGainVal = curAG;
        dspEngine.setAutoGainEnabled (curAG);
    }

    const int curVar = getConsoleVariant();
    if (curVar != lastConsoleVariant)
    {
        lastConsoleVariant = curVar;
        dspEngine.setConsoleVariant (curVar);
    }

    const int curStages = getActiveStages();
    if (curStages != lastActiveStages)
    {
        lastActiveStages = curStages;
        dspEngine.setActiveStageCount (curStages);
    }

    const bool curNoise = getAnalogNoiseEnabled();
    if (curNoise != lastNoiseEnabled)
    {
        lastNoiseEnabled = curNoise;
        dspEngine.setMasterNoiseEnabled (curNoise);
    }
}

void JJ8NLSAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseSampleRate = std::max (8000.0, sampleRate);
    maxBlockSize   = std::max (1, samplesPerBlock);

    for (auto& os : oversamplers)
    {
        if (os != nullptr)
        {
            os->initProcessing (static_cast<size_t> (maxBlockSize));
            os->reset();
        }
    }

    lastDriveVal       = -999.0;
    lastSubPunchVal    = -999.0;
    lastConsoleVariant = -1;
    lastActiveStages   = -1;
    applyOversamplingConfiguration (getOversamplingMode());
    syncParametersToEngine();
}

void JJ8NLSAudioProcessor::releaseResources()
{
    dspEngine.reset();
    for (auto& os : oversamplers)
        if (os != nullptr)
            os->reset();
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

    const int wantedOSMode = getOversamplingMode();
    if (wantedOSMode != activeOSMode)
        applyOversamplingConfiguration (wantedOSMode);

    syncParametersToEngine();

    const bool isBypassed = (bypassParam != nullptr) && (bypassParam->load (std::memory_order_relaxed) >= 0.5f);
    if (isBypassed || totalNumInputChannels <= 0 || numSamples <= 0)
    {
        harmonicActivity.store (0.0f, std::memory_order_relaxed);
        return;
    }

    float blockPeak = 0.0f;

    if (activeOSMode >= 1 && activeOSMode <= 3 && oversamplers[static_cast<size_t> (activeOSMode - 1)] != nullptr)
    {
        auto& os = *oversamplers[static_cast<size_t> (activeOSMode - 1)];
        juce::dsp::AudioBlock<float> baseBlock (buffer);
        const size_t activeCh = static_cast<size_t> (juce::jlimit (1, 2, totalNumInputChannels));
        auto activeBaseBlock = baseBlock.getSubsetChannelBlock (0, activeCh);
        auto osBlock = os.processSamplesUp (activeBaseBlock);
        const int osSamples = static_cast<int> (osBlock.getNumSamples());

        if (activeCh == 1)
        {
            float* ch0 = osBlock.getChannelPointer (0);
            for (int n = 0; n < osSamples; ++n)
            {
                if (driveSmoothed.isSmoothing())
                    dspEngine.setBipolarDrive (driveSmoothed.getNextValue());

                dspEngine.processSampleMono (ch0[n]);
                blockPeak = std::max (blockPeak, std::abs (ch0[n]));
            }
        }
        else
        {
            float* chL = osBlock.getChannelPointer (0);
            float* chR = osBlock.getChannelPointer (1);
            for (int n = 0; n < osSamples; ++n)
            {
                if (driveSmoothed.isSmoothing())
                    dspEngine.setBipolarDrive (driveSmoothed.getNextValue());

                dspEngine.processSampleStereo (chL[n], chR[n]);
                blockPeak = std::max (blockPeak, std::max (std::abs (chL[n]), std::abs (chR[n])));
            }
        }

        os.processSamplesDown (activeBaseBlock);
    }
    else
    {
        if (totalNumInputChannels == 1)
        {
            float* ch0 = buffer.getWritePointer (0);
            for (int n = 0; n < numSamples; ++n)
            {
                if (driveSmoothed.isSmoothing())
                    dspEngine.setBipolarDrive (driveSmoothed.getNextValue());

                dspEngine.processSampleMono (ch0[n]);
                blockPeak = std::max (blockPeak, std::abs (ch0[n]));
            }
        }
        else
        {
            float* chL = buffer.getWritePointer (0);
            float* chR = buffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                if (driveSmoothed.isSmoothing())
                    dspEngine.setBipolarDrive (driveSmoothed.getNextValue());

                dspEngine.processSampleStereo (chL[n], chR[n]);
                blockPeak = std::max (blockPeak, std::max (std::abs (chL[n]), std::abs (chR[n])));
            }
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
    state.setProperty ("skinIndex",          getSkinIndex(), nullptr);
    state.setProperty ("oversamplingMode",   getOversamplingMode(), nullptr);
    state.setProperty ("consoleVariant",     getConsoleVariant(), nullptr);
    state.setProperty ("activeStages",       getActiveStages(), nullptr);
    state.setProperty ("analogNoiseEnabled", getAnalogNoiseEnabled(), nullptr);
    state.setProperty ("editorW",            getSavedEditorWidth(), nullptr);
    state.setProperty ("editorH",            getSavedEditorHeight(), nullptr);
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
            if (vt.hasProperty ("oversamplingMode"))
                setOversamplingMode (static_cast<int> (vt.getProperty ("oversamplingMode")));
            if (vt.hasProperty ("consoleVariant"))
                setConsoleVariant (static_cast<int> (vt.getProperty ("consoleVariant")));
            if (vt.hasProperty ("activeStages"))
                setActiveStages (static_cast<int> (vt.getProperty ("activeStages")));
            if (vt.hasProperty ("analogNoiseEnabled"))
                setAnalogNoiseEnabled (static_cast<bool> (vt.getProperty ("analogNoiseEnabled")));
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

