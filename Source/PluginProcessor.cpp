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
        0.0f));

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
        false));

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

    for (size_t i=0; i<oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, i==0 ? 0 : i+1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        auto& engine = dspEngines[i];
        engine.applyDefaultJaycenJoshuaChain();
        engine.setStereoBalanceMode (jj8nls::StereoBalanceMode::SubAndMidLocked);
        engine.setAutoPeakRmsBalance (false);
        engine.setManualBalanceDb (0.0);
        engine.setOutputTrimDb (0.0);
        engine.setBipolarDrive (0.0);
    }
    syncParametersToEngine();
    startTimer (20);
}

JJ8NLSAudioProcessor::~JJ8NLSAudioProcessor() { stopTimer(); }

void JJ8NLSAudioProcessor::timerCallback()
{
    // Host latency notification stays on the message thread. The audio callback
    // only writes an atomic request, avoiding message posting/allocation there.
    const int latency = pendingHostLatency.exchange (-1, std::memory_order_acq_rel);
    if (latency >= 0 && latency == targetHostLatency.load (std::memory_order_acquire) && latency != getLatencySamples()) setLatencySamples (latency);
}

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
    activeOSMode = osMode <= 0 ? 0 : juce::jlimit (2, 3, osMode);
    runningOSMode.store (activeOSMode, std::memory_order_relaxed);
    int latency = 0;
    for (size_t i=0; i<oversamplers.size(); ++i)
    {
        const double rate = baseSampleRate * static_cast<double> (1 << (i==0 ? 0 : static_cast<int> (i)+1));
        dspEngines[i].prepare (rate);
        driveSmoothed[i].reset (rate, 0.030);
        toneSmoothed[i].reset (rate, 0.030);
        driveSmoothed[i].setCurrentAndTargetValue (driveParam->load());
        toneSmoothed[i].setCurrentAndTargetValue (subPunchParam->load());
        dspEngines[i].setBipolarDrive (driveSmoothed[i].getCurrentValue());
        dspEngines[i].setBipolarTone (toneSmoothed[i].getCurrentValue());
        qualityLatencies[i] = static_cast<int> (std::lround (oversamplers[i]->getLatencyInSamples()));
        latency = std::max (latency, qualityLatencies[i]);
    }
    maxQualityLatency = latency;
    renderLatency = activeOSMode == 0 ? 0 : maxQualityLatency;
    dryLatencySamples.reset (baseSampleRate, 0.010);
    dryLatencySamples.setCurrentAndTargetValue (static_cast<double> (renderLatency));
    targetHostLatency.store (renderLatency, std::memory_order_release);
    pendingHostLatency.store (-1, std::memory_order_relaxed);
    setLatencySamples (renderLatency);

}

void JJ8NLSAudioProcessor::syncParametersToEngine() noexcept
{
    const double curDrive = (driveParam != nullptr) ? static_cast<double> (driveParam->load (std::memory_order_relaxed)) : 1.0;
    const double curSub   = (subPunchParam != nullptr) ? static_cast<double> (subPunchParam->load (std::memory_order_relaxed)) : 0.0;
    const bool curAG      = (autoGainParam != nullptr) ? (autoGainParam->load (std::memory_order_relaxed) >= 0.5f) : true;

    if (std::abs (curDrive - lastDriveVal) > 1.0e-4)
    {
        lastDriveVal = curDrive;
        for (auto& value : driveSmoothed) value.setTargetValue (curDrive);
    }

    if (std::abs (curSub - lastSubPunchVal) > 1.0e-4)
    {
        lastSubPunchVal = curSub;
        for (auto& value : toneSmoothed) value.setTargetValue (curSub);
    }

    if (curAG != lastAutoGainVal)
    {
        lastAutoGainVal = curAG;
        for (auto& engine : dspEngines) engine.setAutoGainEnabled (curAG);
    }

    const int mask = mikeMask.load();
    const int curStages = mask >= 0 ? 8 : getActiveStages();
    if (mask != lastMikeMask)
    {
        for (auto& engine : dspEngines)
            for (int i = 0; i < 4; ++i) engine.setSlotEnabled (i+1, mask < 0 || (mask & (1 << i)) != 0);
        lastMikeMask = mask;
    }
    if (curStages != lastActiveStages)
    {
        lastActiveStages = curStages;
        for (auto& engine : dspEngines) engine.setActiveStageCount (curStages);
    }

    const bool curNoise = getAnalogNoiseEnabled();
    if (curNoise != lastNoiseEnabled)
    {
        lastNoiseEnabled = curNoise;
        for (auto& engine : dspEngines) engine.setMasterNoiseEnabled (curNoise);
    }
}

void JJ8NLSAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseSampleRate = std::max (8000.0, sampleRate);
    maxBlockSize   = std::max (1, samplesPerBlock);

    for (auto& os : oversamplers)
    {
        os->initProcessing (static_cast<size_t> (maxBlockSize));
        os->reset();
    }
    bypassMix.reset (baseSampleRate, 0.020);
    bypassMix.setCurrentAndTargetValue (bypassParam->load() >= 0.5f ? 1.0f : 0.0f);
    toneTicks.fill (0);
    qualityWarmSamples = 0;
    qualityMix.reset (baseSampleRate, 0.020);
    qualityMix.setCurrentAndTargetValue (1.0f);
    lastDriveVal       = -999.0;
    lastSubPunchVal    = -999.0;
    lastActiveStages   = -1;
    applyOversamplingConfiguration (getOversamplingMode());
    syncParametersToEngine();
    dryDelay.setSize (2, maxQualityLatency + maxBlockSize + 1);
    dryDelay.clear();
    wetDelay.setSize (2, dryDelay.getNumSamples());
    wetDelay.clear();
    dryPosition = 0;
}

void JJ8NLSAudioProcessor::releaseResources()
{
    for (auto& engine : dspEngines) engine.reset();
    for (auto& os : oversamplers) os->reset();
    dryDelay.clear();
    wetDelay.clear();
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
    const int wanted = getOversamplingMode();
    if (wanted != activeOSMode) qualityMix.setTargetValue (0.0f);
    else if (qualityWarmSamples == 0) qualityMix.setTargetValue (1.0f);
    float peak = 0.0f;
    // Hosts may deliver blocks larger than the preparation hint. Chunk without allocating.
    for (int offset = 0; offset < samples; offset += maxBlockSize)
    {
        if (! qualityMix.isSmoothing() && qualityMix.getCurrentValue() == 0.0f
            && getOversamplingMode() != activeOSMode)
        {
            activeOSMode = getOversamplingMode();
            renderLatency = activeOSMode == 0 ? 0 : maxQualityLatency;
            dryLatencySamples.setTargetValue (static_cast<double> (renderLatency));
            targetHostLatency.store (renderLatency, std::memory_order_release);
            pendingHostLatency.store (renderLatency, std::memory_order_release);
            const auto next = static_cast<size_t> (activeOSMode == 0 ? 0 : activeOSMode-1);
            oversamplers[next]->reset();
            dspEngines[next].reset();
            driveSmoothed[next].setCurrentAndTargetValue (driveParam->load (std::memory_order_relaxed));
            toneSmoothed[next].setCurrentAndTargetValue (subPunchParam->load (std::memory_order_relaxed));
            dspEngines[next].setBipolarDrive (driveSmoothed[next].getCurrentValue());
            dspEngines[next].setBipolarTone (toneSmoothed[next].getCurrentValue());
            toneTicks[next] = 0;
            wetDelay.clear();
            qualityWarmSamples = static_cast<int> (std::ceil (0.040 * baseSampleRate));
            runningOSMode.store (activeOSMode, std::memory_order_relaxed);
        }
        const auto quality = static_cast<size_t> (activeOSMode == 0 ? 0 : activeOSMode-1);
        auto& os = *oversamplers[quality];
        auto& engine = dspEngines[quality];
        auto& drive = driveSmoothed[quality];
        auto& toneValue = toneSmoothed[quality];
        auto& toneTick = toneTicks[quality];
        const int wetPadding = renderLatency-qualityLatencies[quality];
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
        auto high = os.processSamplesUp (base);
        auto* left = high.getChannelPointer (0);
        auto* right = channels == 2 ? high.getChannelPointer (1) : nullptr;
        for (size_t n=0; n<high.getNumSamples(); ++n)
        {
            const double driveValue = drive.getNextValue();
            if (toneTick == 31) engine.setBipolarDrive (driveValue);
            const double tone = toneValue.getNextValue();
            // Coefficient designs run at a bounded control rate; filter coefficients
            // themselves slew continuously and preserve their delay states.
            if (++toneTick >= 32)
            {
                toneTick = 0;
                engine.setBipolarTone (tone);
            }
            if (right != nullptr) engine.processSampleStereo (left[n], right[n]);
            else engine.processSampleMono (left[n]);
        }
        os.processSamplesDown (base);
        for (int n=0; n<count; ++n) for (int ch=0; ch<channels; ++ch)
            wetDelay.setSample (ch, (startPosition+n) % ringSize, buffer.getSample (ch, offset+n));
        for (int n=0; n<count; ++n)
        {
            if (qualityWarmSamples > 0 && --qualityWarmSamples == 0 && getOversamplingMode() == activeOSMode)
                qualityMix.setTargetValue (1.0f);
            const float wetMix = (1.0f-bypassMix.getNextValue()) * qualityMix.getNextValue();
            const int wetRead = (startPosition+n-wetPadding+ringSize) % ringSize;
            const double dryLatency = dryLatencySamples.getNextValue();
            const int wholeDelay = static_cast<int> (dryLatency);
            const float fraction = static_cast<float> (dryLatency-static_cast<double> (wholeDelay));
            const int read = (startPosition+n-wholeDelay+ringSize) % ringSize;
            const int previousRead = (read+ringSize-1) % ringSize;
            for (int ch=0; ch<channels; ++ch)
            {
                const float wet = wetDelay.getSample (ch, wetRead);
                const float dryA = dryDelay.getSample (ch, read);
                const float dry = dryA + fraction * (dryDelay.getSample (ch, previousRead)-dryA);
                const float out = wetMix == 0.0f ? dry : (wetMix == 1.0f ? wet : dry + wetMix * (wet-dry));
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
    state.setProperty ("activeStages", activeStages.load(), nullptr);
    state.setProperty ("jaycenMikeMask", mikeMask.load(), nullptr);
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
            vt.removeProperty ("consoleVariant", nullptr);
            if (vt.hasProperty ("activeStages"))
                setActiveStages (static_cast<int> (vt.getProperty ("activeStages")));
            mikeMask.store (juce::jlimit (-1, 15, static_cast<int> (vt.getProperty ("jaycenMikeMask", -1))));
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

