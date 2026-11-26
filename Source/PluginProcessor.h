#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/JJ8NLSDSP.h"
#include "FLStudioLookAndFeel.h"
#include <array>
#include <atomic>
#include <memory>

class JJ8NLSAudioProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr const char* kParamDrive    = "drive";
    static constexpr const char* kParamTone     = "sub_punch";
    static constexpr const char* kParamSubPunch = "sub_punch";
    static constexpr const char* kParamLink     = "autogain";
    static constexpr const char* kParamAutoGain = "autogain";
    static constexpr const char* kParamBypass   = "bypass";

    JJ8NLSAudioProcessor();
    ~JJ8NLSAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int getSkinIndex() const noexcept { return skinIndex.load (std::memory_order_relaxed); }
    void setSkinIndex (int newSkin) noexcept { skinIndex.store (juce::jlimit (0, static_cast<int> (flheat::kNumSkins) - 1, newSkin), std::memory_order_relaxed); }

    int getOversamplingMode() const noexcept { return oversamplingMode.load (std::memory_order_relaxed); }
    int getActiveOversamplingMode() const noexcept { return runningOSMode.load (std::memory_order_relaxed); }
    void setOversamplingMode (int newOS) noexcept { oversamplingMode.store ((newOS <= 0 ? 0 : juce::jlimit (2, 3, newOS)), std::memory_order_relaxed); }

    int getConsoleVariant() const noexcept { return consoleVariant.load (std::memory_order_relaxed); }
    void setConsoleVariant (int newVar) noexcept { consoleVariant.store (juce::jlimit (0, 4, newVar), std::memory_order_relaxed); }

    int getActiveStages() const noexcept
    {
        const int mask = mikeMask.load();
        if (getConsoleVariant() == 0 && mask >= 0)
            return 4 + juce::countNumberOfBits (static_cast<juce::uint32> (mask));
        return activeStages.load();
    }
    int getMikeMask() const noexcept
    {
        const int custom = mikeMask.load();
        return custom >= 0 ? custom : (1 << juce::jlimit (0, 4, activeStages.load() - 4)) - 1;
    }
    void toggleMike (int index) noexcept
    {
        if (juce::isPositiveAndBelow (index, 4))
            mikeMask.store (getMikeMask() ^ (1 << index));
    }
    void setActiveStages (int n) noexcept { activeStages.store (juce::jlimit (1, 8, n), std::memory_order_relaxed); mikeMask.store (-1); }

    bool getAnalogNoiseEnabled() const noexcept { return analogNoiseEnabled.load (std::memory_order_relaxed); }
    void setAnalogNoiseEnabled (bool en) noexcept { analogNoiseEnabled.store (en, std::memory_order_relaxed); }

    bool isLinkedMode() const noexcept
    {
        return (autoGainParam != nullptr) ? (autoGainParam->load (std::memory_order_relaxed) >= 0.5f) : true;
    }

    void pullSharedBusIfNeeded() noexcept {}

    int getSavedEditorWidth() const noexcept  { return savedEditorWidth.load (std::memory_order_relaxed); }
    int getSavedEditorHeight() const noexcept { return savedEditorHeight.load (std::memory_order_relaxed); }
    void setSavedEditorSize (int w, int h) noexcept
    {
        savedEditorWidth.store (w, std::memory_order_relaxed);
        savedEditorHeight.store (h, std::memory_order_relaxed);
    }

    float getHarmonicActivity() const noexcept { return harmonicActivity.load (std::memory_order_relaxed); }
    float getHarmonicPolarity() const noexcept
    {
        const float d = (driveParam != nullptr) ? driveParam->load (std::memory_order_relaxed) : 1.0f;
        return (d >= 0.0f) ? 1.0f : -1.0f;
    }

private:
    void timerCallback() override;
    void syncParametersToEngine() noexcept;
    void applyOversamplingConfiguration (int osMode);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* subPunchParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;
    std::atomic<float>* bypassParam   = nullptr;

    std::atomic<int> skinIndex        { flheat::kSkinFLStudio };
    std::atomic<int> oversamplingMode { 3 };
    std::atomic<int> runningOSMode { 3 };
    std::atomic<int> consoleVariant   { 0 };
    std::atomic<int> activeStages     { 8 };
    std::atomic<bool> analogNoiseEnabled { true };
    std::atomic<int> savedEditorWidth  { 130 };
    std::atomic<int> savedEditorHeight { 240 };
    std::atomic<float> harmonicActivity { 0.0f };

    // All quality domains are allocated/prepared off the audio thread.
    // Only one is processed; switching goes through latency-aligned dry audio.
    std::array<jj8nls::JJ8NLSChainDSP, 3> dspEngines;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    std::array<juce::SmoothedValue<double>, 3> driveSmoothed, toneSmoothed;
    std::array<int, 3> toneTicks {}, qualityLatencies {};
    juce::AudioBuffer<float> dryDelay, wetDelay;
    int dryPosition = 0;
    int qualityWarmSamples = 0;
    int maxQualityLatency = 0;
    int renderLatency = 0;
    std::atomic<int> pendingHostLatency { -1 }, targetHostLatency { 0 };
    juce::SmoothedValue<double> dryLatencySamples;
    bool hostBypassed = false;
    juce::SmoothedValue<float> bypassMix, qualityMix;
    double baseSampleRate  = 48000.0;
    int maxBlockSize       = 512;
    int activeOSMode       = -1;
    double lastDriveVal    = -999.0;
    double lastSubPunchVal = -999.0;
    bool lastAutoGainVal   = true;
    int lastConsoleVariant = -1;
    std::atomic<int> mikeMask { -1 };
    int lastMikeMask = -2;
    int lastActiveStages   = -1;
    bool lastNoiseEnabled  = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JJ8NLSAudioProcessor)
};

using FLHeatAudioProcessor = JJ8NLSAudioProcessor;

