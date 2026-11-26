#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout JJ8NLSAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global eight-stage drive; existing parameter ID and range remain session compatible.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kParamDrive, 1 },
        "Drive",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f),
        1.0f));

    // Tone: controlled transformer resonance, Tight Sub and paired pre/de-emphasis.
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

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);

    // Reference-inspired model order, full eight-stage cascade by default:
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
double JJ8NLSAudioProcessor::getTailLengthSeconds() const { return 0.5; }

int JJ8NLSAudioProcessor::getNumPrograms() { return 1; }
int JJ8NLSAudioProcessor::getCurrentProgram() { return 0; }
void JJ8NLSAudioProcessor::setCurrentProgram (int) {}
const juce::String JJ8NLSAudioProcessor::getProgramName (int) { return "Jaycen Joshua 8x (Default)"; }
void JJ8NLSAudioProcessor::changeProgramName (int, const juce::String&) {}

void JJ8NLSAudioProcessor::applyOversamplingConfiguration (int osMode)
{
    juce::ignoreUnused (osMode);
    activeOSMode = 3;
    const double effectiveSampleRate = baseSampleRate * 8.0;
    dspEngine.prepare (effectiveSampleRate);
    driveSmoothed.reset (effectiveSampleRate, 0.030);
    toneSmoothed.reset (effectiveSampleRate, 0.030);
    driveSmoothed.setCurrentAndTargetValue (driveParam->load());
    toneSmoothed.setCurrentAndTargetValue (subPunchParam->load());
    dspEngine.setBipolarDrive (driveSmoothed.getCurrentValue());
    dspEngine.setBipolarTone (toneSmoothed.getCurrentValue());
    setLatencySamples (static_cast<int> (std::lround (oversampler->getLatencyInSamples())));

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
        toneSmoothed.setTargetValue (curSub);
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

    oversampler->initProcessing (static_cast<size_t> (maxBlockSize));
    oversampler->reset();
    bypassMix.reset (baseSampleRate, 0.020);
    bypassMix.setCurrentAndTargetValue (bypassParam->load() >= 0.5f ? 1.0f : 0.0f);
    toneTick = 0;
    lastDriveVal       = -999.0;
    lastSubPunchVal    = -999.0;
    lastConsoleVariant = -1;
    lastActiveStages   = -1;
    applyOversamplingConfiguration (getOversamplingMode());
    syncParametersToEngine();
    dryDelay.setSize (2, getLatencySamples() + maxBlockSize + 1);
    dryDelay.clear();
    dryPosition = 0;
}

void JJ8NLSAudioProcessor::releaseResources()
{
    dspEngine.reset();
    oversampler->reset();
    dryDelay.clear();
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
    const int channels = juce::jlimit (0, 2, getTotalNumInputChannels());
    const int samples = buffer.getNumSamples();
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, samples);
    if (channels == 0 || samples == 0) return;
    syncParametersToEngine();
    bypassMix.setTargetValue (hostBypassed || bypassParam->load (std::memory_order_relaxed) >= 0.5f ? 1.0f : 0.0f);
    float peak = 0.0f;
    // Hosts may deliver blocks larger than the preparation hint. Chunk without allocating.
    for (int offset = 0; offset < samples; offset += maxBlockSize)
    {
        const int count = std::min (maxBlockSize, samples - offset);
        const int startPosition = dryPosition;
        const int ringSize = dryDelay.getNumSamples();
        for (int n=0; n<count; ++n)
        {
            for (int ch=0; ch<channels; ++ch)
                dryDelay.setSample (ch, (startPosition+n) % ringSize, buffer.getSample (ch, offset+n));
        }
        juce::dsp::AudioBlock<float> whole (buffer);
        auto base = whole.getSubsetChannelBlock (0, static_cast<size_t> (channels))
                         .getSubBlock (static_cast<size_t> (offset), static_cast<size_t> (count));
        auto high = oversampler->processSamplesUp (base);
        auto* left = high.getChannelPointer (0);
        auto* right = channels == 2 ? high.getChannelPointer (1) : nullptr;
        for (size_t n=0; n<high.getNumSamples(); ++n)
        {
            dspEngine.setBipolarDrive (driveSmoothed.getNextValue());
            const double tone = toneSmoothed.getNextValue();
            // Coefficient designs run at a bounded control rate; filter coefficients
            // themselves slew continuously and preserve their delay states.
            if (++toneTick >= 32)
            {
                toneTick = 0;
                dspEngine.setBipolarTone (tone);
            }
            if (right != nullptr) dspEngine.processSampleStereo (left[n], right[n]);
            else dspEngine.processSampleMono (left[n]);
        }
        oversampler->processSamplesDown (base);
        for (int n=0; n<count; ++n)
        {
            const float mix = bypassMix.getNextValue();
            const int read = (startPosition+n-getLatencySamples()+ringSize) % ringSize;
            for (int ch=0; ch<channels; ++ch)
            {
                const float wet = buffer.getSample (ch, offset+n);
                const float dry = dryDelay.getSample (ch, read);
                const float out = wet + mix * (dry-wet);
                buffer.setSample (ch, offset+n, out);
                peak = std::max (peak, std::abs (out));
            }
        }
        dryPosition = (startPosition+count) % ringSize;
    }
    harmonicActivity.store (juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (peak, -60.0f)+48.0f)/48.0f),
                            std::memory_order_relaxed);
}

void JJ8NLSAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    hostBypassed = true;
    processBlock (buffer, midi);
    hostBypassed = false;
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

