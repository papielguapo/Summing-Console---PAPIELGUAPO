#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/JJ8NLSDSP.h"
#include "FLStudioLookAndFeel.h"
#include <array>
#include <atomic>
#include <memory>

class JJ8NLSAudioProcessor : public juce::AudioProcessor
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

    int getOversamplingMode() const noexcept { return 3; }
    void setOversamplingMode (int newOS) noexcept { juce::ignoreUnused (newOS); }

    int getConsoleVariant() const noexcept { return consoleVariant.load (std::memory_order_relaxed); }
    void setConsoleVariant (int newVar) noexcept { consoleVariant.store (juce::jlimit (0, 4, newVar), std::memory_order_relaxed); }

    int getActiveStages() const noexcept { return activeStages.load (std::memory_order_relaxed); }
    void setActiveStages (int n) noexcept { activeStages.store (juce::jlimit (1, 8, n), std::memory_order_relaxed); }

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
    void syncParametersToEngine() noexcept;
    void applyOversamplingConfiguration (int osMode);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* subPunchParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;
    std::atomic<float>* bypassParam   = nullptr;

    std::atomic<int> skinIndex        { flheat::kSkinFLStudio };
    // Legacy quality state is migrated to a fixed 8x processing domain.
    std::atomic<int> consoleVariant   { 0 };
    std::atomic<int> activeStages     { 8 };
    std::atomic<bool> analogNoiseEnabled { true };
    std::atomic<int> savedEditorWidth  { 130 };
    std::atomic<int> savedEditorHeight { 240 };
    std::atomic<float> harmonicActivity { 0.0f };

    jj8nls::JJ8NLSChainDSP dspEngine;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> dryDelay;
    int dryPosition = 0;
    bool hostBypassed = false;
    juce::SmoothedValue<float> bypassMix;
    juce::SmoothedValue<double> toneSmoothed;
    int toneTick = 0;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> driveSmoothed;
    double baseSampleRate  = 48000.0;
    int maxBlockSize       = 512;
    int activeOSMode       = -1;
    double lastDriveVal    = -999.0;
    double lastSubPunchVal = -999.0;
    bool lastAutoGainVal   = true;
    int lastConsoleVariant = -1;
    int lastActiveStages   = -1;
    bool lastNoiseEnabled  = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JJ8NLSAudioProcessor)
};

using FLHeatAudioProcessor = JJ8NLSAudioProcessor;

