#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/JJ8NLSDSP.h"
#include "FLStudioLookAndFeel.h"
#include <atomic>

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
    void setOversamplingMode (int newOS) noexcept { oversamplingMode.store (juce::jlimit (0, 3, newOS), std::memory_order_relaxed); }

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

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* subPunchParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;
    std::atomic<float>* bypassParam   = nullptr;

    std::atomic<int> skinIndex        { flheat::kSkinFLStudio };
    std::atomic<int> oversamplingMode { 2 };
    std::atomic<int> savedEditorWidth  { 130 };
    std::atomic<int> savedEditorHeight { 240 };
    std::atomic<float> harmonicActivity { 0.0f };

    jj8nls::JJ8NLSChainDSP dspEngine;
    double lastDriveVal    = -999.0;
    double lastSubPunchVal = -999.0;
    bool lastAutoGainVal   = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JJ8NLSAudioProcessor)
};

using FLHeatAudioProcessor = JJ8NLSAudioProcessor;

