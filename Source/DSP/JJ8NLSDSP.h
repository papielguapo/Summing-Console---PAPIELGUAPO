#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <juce_audio_basics/juce_audio_basics.h>

namespace jj8nls
{
    static constexpr double kPi = 3.14159265358979323846;

    enum class ConsoleModel : int
    {
        Spike = 0, // SSL 4000G
        Mike  = 1, // EMI TG12345 Mk IV
        Nevo  = 2  // Neve 5116
    };

    enum class StereoBalanceMode : int
    {
        SubAndMidLocked = 0, // Default: Sub (20-40 Hz) + Mid/High (200 Hz-20 kHz) reference-inspired L=R Locked
        SubLockedMidTol = 1, // Sub (20-90 Hz) L=R Locked + Analog Mid Tolerance (Pan/RMS matched)
        RawUnbalanced   = 2  // Original Unbalanced 8x Chain (L sub louder +6.3 dB, R mid louder +0.7 dB)
    };

    enum class ChainVariant : int
    {
        JJDrumsSubMidLocked = 0, // 8x JJ Chain + Sub & Mid L=R Locked (0.00 dB skew)
        JJDrumsTightSub     = 1, // 8x JJ Chain + Controlled Sub (55% 27 Hz bump) + Sub & Mid Locked
        JJDrumsHotDrive     = 2, // 8x JJ Chain + Slot 1 Drive 3.5 + Sub & Mid Locked
        JJDrumsAnalogMidTol = 3, // 8x JJ Chain + Sub Locked + Analog Mid Tolerance
        JJDrumsRawUnbal     = 4, // 8x JJ Chain Raw Unbalanced (Original Waves 8x behavior)
        AllMike8x           = 5, // 8x EMI TG12345 (All Mike) + Sub & Mid Locked
        AllSpike8x          = 6, // 8x SSL 4000G (All Spike) + Sub & Mid Locked
        AllNevo8x           = 7  // 8x Neve 5116 (All Nevo) + Sub & Mid Locked
    };

    struct BiquadSection
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;
        std::array<double, 5> target { 1.0, 0.0, 0.0, 0.0, 0.0 };
        double slew = 1.0;
        int coefficientsMoving = 0;
        void setTarget (const BiquadSection& s) noexcept
        {
            const std::array<double,5> next { s.b0, s.b1, s.b2, s.a1, s.a2 };
            if (next == target) return;
            target = next;
            coefficientsMoving = static_cast<int> (std::ceil (12.0 / slew));
        }
        void snap() noexcept
        { b0=target[0]; b1=target[1]; b2=target[2]; a1=target[3]; a2=target[4]; coefficientsMoving=0; }


        void reset() noexcept
        {
            z1 = 0.0;
            z2 = 0.0;
        }

        inline double process (double x) noexcept
        {
            if (coefficientsMoving > 0)
            {
                b0 += slew * (target[0]-b0); b1 += slew * (target[1]-b1);
                b2 += slew * (target[2]-b2); a1 += slew * (target[3]-a1); a2 += slew * (target[4]-a2);
                if (--coefficientsMoving == 0) snap();
            }
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        static BiquadSection makeFirstOrderHighPass (double hz, double fs) noexcept
        {
            BiquadSection s;
            const double k = std::tan (kPi * hz / fs);
            s.b0 = 1.0 / (1.0 + k); s.b1 = -s.b0;
            s.a1 = (k - 1.0) / (k + 1.0);
            return s;
        }

        static BiquadSection makeHighPass (double f0, double q, double fs) noexcept
        {
            BiquadSection s;
            const double fc = std::clamp (f0, 2.0, 0.47 * fs);
            const double w0 = 2.0 * kPi * fc / fs;
            const double cw = std::cos (w0);
            const double sw = std::sin (w0);
            const double alpha = sw / (2.0 * std::max (0.05, q));
            const double a0 = 1.0 + alpha;
            s.b0 = ((1.0 + cw) * 0.5) / a0;
            s.b1 = (-(1.0 + cw)) / a0;
            s.b2 = ((1.0 + cw) * 0.5) / a0;
            s.a1 = (-2.0 * cw) / a0;
            s.a2 = (1.0 - alpha) / a0;
            return s;
        }

        static BiquadSection makeLowPass (double f0, double q, double fs) noexcept
        {
            BiquadSection s;
            const double fc = std::clamp (f0, 20.0, 0.47 * fs);
            const double w0 = 2.0 * kPi * fc / fs;
            const double cw = std::cos (w0);
            const double sw = std::sin (w0);
            const double alpha = sw / (2.0 * std::max (0.05, q));
            const double a0 = 1.0 + alpha;
            s.b0 = ((1.0 - cw) * 0.5) / a0;
            s.b1 = (1.0 - cw) / a0;
            s.b2 = ((1.0 - cw) * 0.5) / a0;
            s.a1 = (-2.0 * cw) / a0;
            s.a2 = (1.0 - alpha) / a0;
            return s;
        }

        static BiquadSection makePeaking (double f0, double q, double gainDb, double fs) noexcept
        {
            BiquadSection s;
            if (std::abs (gainDb) < 1.0e-5)
                return s;

            const double fc = std::clamp (f0, 5.0, 0.47 * fs);
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * kPi * fc / fs;
            const double cw = std::cos (w0);
            const double sw = std::sin (w0);
            const double alpha = sw / (2.0 * std::max (0.05, q));
            const double a0 = 1.0 + alpha / A;
            s.b0 = (1.0 + alpha * A) / a0;
            s.b1 = (-2.0 * cw) / a0;
            s.b2 = (1.0 - alpha * A) / a0;
            s.a1 = (-2.0 * cw) / a0;
            s.a2 = (1.0 - alpha / A) / a0;
            return s;
        }

        static BiquadSection makeHighShelf (double f0, double q, double gainDb, double fs) noexcept
        {
            BiquadSection s;
            if (std::abs (gainDb) < 1.0e-5)
                return s;

            const double fc = std::clamp (f0, 100.0, 0.47 * fs);
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * kPi * fc / fs;
            const double cw = std::cos (w0);
            const double sw = std::sin (w0);
            const double alpha = sw / (2.0 * std::max (0.05, q));
            const double twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;
            const double a0 = (A + 1.0) - (A - 1.0) * cw + twoSqrtAAlpha;
            s.b0 = (A * ((A + 1.0) + (A - 1.0) * cw + twoSqrtAAlpha)) / a0;
            s.b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cw)) / a0;
            s.b2 = (A * ((A + 1.0) + (A - 1.0) * cw - twoSqrtAAlpha)) / a0;
            s.a1 = (2.0 * ((A - 1.0) - (A + 1.0) * cw)) / a0;
            s.a2 = ((A + 1.0) - (A - 1.0) * cw - twoSqrtAAlpha) / a0;
            return s;
        }
    };

    struct SlotConfig
    {
        ConsoleModel model = ConsoleModel::Mike;
        double drive = 0.0;     // 0.0 .. 12.0
        double trimDb = 0.0;    // -12.0 .. +12.0
        bool noiseOn = true;
        bool enabled = true;
    };

    struct StageChannelState
    {
        std::array<BiquadSection, 5> filters;
        double tolerance = 1.0, bias = 0.0;
        double cachedBias = 999.0, biasTanh = 0.0, biasSlope = 1.0;
        juce::SmoothedValue<double> modelAmount, wet;

        double pinkB0 = 0.0;
        double pinkB1 = 0.0;
        double pinkB2 = 0.0;

        void reset() noexcept
        {
            for (auto& f : filters)
                f.reset();
            pinkB0 = 0.0;
            pinkB1 = 0.0;
            pinkB2 = 0.0;
        }
    };

    class JJ8NLSChainDSP
    {
    public:
        static constexpr int kNumSlots = 8;

        JJ8NLSChainDSP()
        {
            applyDefaultJaycenJoshuaChain();
        }

        void applyDefaultJaycenJoshuaChain() noexcept
        {
            static constexpr ConsoleModel kJJModels[kNumSlots] =
            {
                ConsoleModel::Mike,
                ConsoleModel::Spike,
                ConsoleModel::Mike,
                ConsoleModel::Mike,
                ConsoleModel::Mike,
                ConsoleModel::Spike,
                ConsoleModel::Nevo,
                ConsoleModel::Mike
            };

            for (int i = 0; i < kNumSlots; ++i)
            {
                slots[static_cast<size_t> (i)].model   = kJJModels[i];
                slots[static_cast<size_t> (i)].drive   = (i == 0) ? 1.0 : 0.0;
                slots[static_cast<size_t> (i)].trimDb  = 0.0;
                slots[static_cast<size_t> (i)].noiseOn = (i != 0);
                slots[static_cast<size_t> (i)].enabled = true;
            }

            currentConsoleVariant = 0;
            updateAllStageFilters();
        }

        void setConsoleVariant (int variantIndex) noexcept
        {
            const int clamped = std::clamp (variantIndex, 0, 4);
            if (currentConsoleVariant == clamped)
                return;

            currentConsoleVariant = clamped;
            if (clamped == 0)
            {
                static constexpr ConsoleModel kJJModels[kNumSlots] =
                {
                    ConsoleModel::Mike,
                    ConsoleModel::Spike,
                    ConsoleModel::Mike,
                    ConsoleModel::Mike,
                    ConsoleModel::Mike,
                    ConsoleModel::Spike,
                    ConsoleModel::Nevo,
                    ConsoleModel::Mike
                };
                for (int i = 0; i < kNumSlots; ++i)
                    slots[static_cast<size_t> (i)].model = kJJModels[i];
            }
            else if (clamped == 4)
            {
                static constexpr ConsoleModel kFLHeatModels[kNumSlots] =
                {
                    ConsoleModel::Spike,
                    ConsoleModel::Nevo,
                    ConsoleModel::Spike,
                    ConsoleModel::Nevo,
                    ConsoleModel::Spike,
                    ConsoleModel::Nevo,
                    ConsoleModel::Spike,
                    ConsoleModel::Nevo
                };
                for (int i = 0; i < kNumSlots; ++i)
                    slots[static_cast<size_t> (i)].model = kFLHeatModels[i];
            }
            else
            {
                const ConsoleModel targetModel = (clamped == 1) ? ConsoleModel::Spike
                                               : (clamped == 2) ? ConsoleModel::Nevo
                                                                : ConsoleModel::Mike;
                for (int i = 0; i < kNumSlots; ++i)
                    slots[static_cast<size_t> (i)].model = targetModel;
            }

            setBipolarDrive (lastRawDrive);
            const double tone = lastRawTone; lastRawTone = -999.0;
            setBipolarTone (tone);
            updateAllStageFilters();
        }

        int getConsoleVariant() const noexcept { return currentConsoleVariant; }

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = std::max (8000.0, newSampleRate);
            autoGainRmsCoeff = 1.0 - std::exp (-1.0 / (0.120 * sampleRate)); // 120ms stereo-linked AutoGain RMS
            autoGainSmoothCoeff = 1.0 - std::exp (-1.0 / (0.045 * sampleRate)); // 45ms smooth AutoGain gain slewing
            harmonicMix.reset (sampleRate, 0.030); harmonicMix.setCurrentAndTargetValue (globalHarmonicScale);
            biasMix.reset (sampleRate, 0.030); biasMix.setCurrentAndTargetValue (dynamicEvenBias);
            noiseMix.reset (sampleRate, 0.030);
            noiseMix.setCurrentAndTargetValue (masterNoiseEnabled ? 1.0 : 0.0);
            autoGainMix.reset (sampleRate, 0.030);
            autoGainMix.setCurrentAndTargetValue (autoGainEnabled ? 1.0 : 0.0);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < kNumSlots; ++i)
                {
                    auto& st = stageStates[ch][i];
                    const double unit = static_cast<double> ((i * 37 + ch * 53 + 11) % 101) / 100.0;
                    st.tolerance = 1.0 + ((i + ch) % 2 ? -1.0 : 1.0) * (0.010 + 0.005 * unit);
                    st.bias = 0.0025 + 0.003 * unit;
                    st.modelAmount.reset (sampleRate, 0.030);
                    st.modelAmount.setCurrentAndTargetValue (static_cast<double> (slots[i].model));
                    st.wet.reset (sampleRate, 0.030);
                    st.wet.setCurrentAndTargetValue (i < activeStageCount ? 1.0 : 0.0);
                    for (auto& f : st.filters) f.slew = 1.0 - std::exp (-1.0 / (0.010 * sampleRate));
                }
            reset();
            updateAllStageFilters();
            updateGlobalFilters();
            for (auto& ch : stageStates) for (auto& st : ch) for (auto& f : st.filters) f.snap();
            for (int ch=0; ch<2; ++ch)
            {
                subsonic[ch].snap(); outputDC[ch].snap(); preEmphasis[ch].snap(); deEmphasis[ch].snap(); outputAir[ch].snap();
            }
        }

        void reset() noexcept
        {
            for (auto& ch : stageStates)
                for (auto& st : ch)
                    st.reset();

            for (int ch=0; ch<2; ++ch)
            { subsonic[ch].reset(); outputDC[ch].reset(); preEmphasis[ch].reset(); deEmphasis[ch].reset(); outputAir[ch].reset(); }
            rngState = 0x853C49E6748FEA9BULL;
            humPhase50  = 0.0;
            humPhase100 = 0.0;
            inRmsEnv    = 1.0e-8;
            outRmsEnv   = 1.0e-8;
            autoGainLinear = 1.0;
        }

        void setStereoBalanceMode (StereoBalanceMode m) noexcept
        {
            if (balanceMode != m)
            {
                balanceMode = m;
                updateAllStageFilters();
            }
        }

        StereoBalanceMode getStereoBalanceMode() const noexcept { return balanceMode; }

        void setSubBumpScale (double scale) noexcept
        {
            const double clamped = std::clamp (scale, 0.0, 1.5);
            if (std::abs (subBumpScale - clamped) > 1.0e-4)
            {
                subBumpScale = clamped;
                updateAllStageFilters();
            }
        }

        double getSubBumpScale() const noexcept { return subBumpScale; }

        void setBipolarTone (double rawTone) noexcept
        {
            const double tone = std::clamp (rawTone, -10.0, 10.0);
            if (std::abs (tone-lastRawTone) < 1.0e-7) return;
            lastRawTone = tone;
            // Left: uniform/open; right: weight/warmth. Center retains the v2.0 voicing.
            subBumpScale = tone < 0.0 ? 1.0+tone/10.0 : 1.0+0.35*tone/10.0;
            if (currentConsoleVariant == 4) subBumpScale *= 0.60;
            updateAllStageFilters();
        }

        void setSlotConfig (int index, const SlotConfig& cfg) noexcept
        {
            if (index < 0 || index >= kNumSlots)
                return;

            auto& cur = slots[static_cast<size_t> (index)];
            const bool modelChanged = (cur.model != cfg.model);
            cur = cfg;
            if (modelChanged)
                updateStageFilter (index);
        }

        const SlotConfig& getSlotConfig (int index) const noexcept
        {
            return slots[static_cast<size_t> (std::clamp (index, 0, kNumSlots - 1))];
        }

        void setSlotDrive (int index, double driveVal) noexcept
        {
            if (index >= 0 && index < kNumSlots)
                slots[static_cast<size_t> (index)].drive = std::clamp (driveVal, 0.0, 12.0);
        }

        void setBipolarDrive (double rawDrive) noexcept
        {
            lastRawDrive = std::clamp (rawDrive, -10.0, 10.0);
            globalHarmonicScale = currentConsoleVariant == 4 ? 0.50 + 0.050 * std::abs (lastRawDrive)
                                                          : std::clamp (1.0 + lastRawDrive * 0.07, 0.3, 1.0);
            dynamicEvenBias = (currentConsoleVariant == 4 && lastRawDrive < 0.0) ? 0.3 : 1.0;
            harmonicMix.setTargetValue (globalHarmonicScale);
            biasMix.setTargetValue (dynamicEvenBias);
        }

        void setSlotModel (int index, ConsoleModel model) noexcept
        {
            if (index >= 0 && index < kNumSlots)
            {
                if (slots[static_cast<size_t> (index)].model != model)
                {
                    slots[static_cast<size_t> (index)].model = model;
                    updateStageFilter (index);
                }
            }
        }

        void setSlotNoise (int index, bool noiseOn) noexcept
        {
            if (index >= 0 && index < kNumSlots)
                slots[static_cast<size_t> (index)].noiseOn = noiseOn;
        }

        void setSlotTrimDb (int index, double trimDb) noexcept
        {
            if (index >= 0 && index < kNumSlots)
                slots[static_cast<size_t> (index)].trimDb = std::clamp (trimDb, -12.0, 12.0);
        }

        void setSlotEnabled (int index, bool en) noexcept
        {
            if (index >= 0 && index < kNumSlots)
                slots[static_cast<size_t> (index)].enabled = en;
        }

        void setMasterNoiseEnabled (bool en) noexcept { masterNoiseEnabled = en; noiseMix.setTargetValue (en ? 1.0 : 0.0); }
        bool getMasterNoiseEnabled() const noexcept   { return masterNoiseEnabled; }
        void setActiveStageCount (int count) noexcept { activeStageCount = std::clamp (count, 1, kNumSlots); }
        int getActiveStageCount() const noexcept      { return activeStageCount; }
        void setAutoPeakRmsBalance (bool en) noexcept { juce::ignoreUnused (en); }
        void setAutoGainEnabled (bool en) noexcept    { autoGainEnabled = en; autoGainMix.setTargetValue (en ? 1.0 : 0.0); }
        bool getAutoGainEnabled() const noexcept      { return autoGainEnabled; }
        void setManualBalanceDb (double db) noexcept  { manualBalanceDb = std::clamp (db, -3.0, 3.0); }
        void setOutputTrimDb (double db) noexcept     { outputTrimLinear = std::pow (10.0, std::clamp (db, -18.0, 18.0) / 20.0); }

        void processSampleStereo (float& left, float& right) noexcept
        {
            double xL = static_cast<double> (left);
            double xR = static_cast<double> (right);
            const double agMix = autoGainMix.getNextValue();
            currentNoiseMix = noiseMix.getNextValue();
            currentHarmonicMix = harmonicMix.getNextValue(); currentBiasMix = biasMix.getNextValue();
            const double inPower = 0.5 * (xL * xL + xR * xR);

            xL = preEmphasis[0].process (subsonic[0].process (xL));
            xR = preEmphasis[1].process (subsonic[1].process (xR));
            double stageRefL = 0.17803894;
            double stageRefR = 0.17803894;

            const double inc50  = 2.0 * kPi * 50.0 / sampleRate;
            const double inc100 = 2.0 * kPi * 100.0 / sampleRate;
            humPhase50  += inc50;
            humPhase100 += inc100;
            if (humPhase50 >= 2.0 * kPi)  humPhase50  -= 2.0 * kPi;
            if (humPhase100 >= 2.0 * kPi) humPhase100 -= 2.0 * kPi;

            const double humSample = 3.2e-7 * std::sin (humPhase50) + 1.8e-7 * std::sin (humPhase100);

            for (int slotIdx = 0; slotIdx < kNumSlots; ++slotIdx)
            {
                const auto& cfg = slots[static_cast<size_t> (slotIdx)];
                for (auto& ch : stageStates)
                    ch[slotIdx].wet.setTargetValue (cfg.enabled && slotIdx < activeStageCount ? 1.0 : 0.0);

                xL = processSingleStage (xL, 0, slotIdx, cfg, humSample, stageRefL);
                xR = processSingleStage (xR, 1, slotIdx, cfg, humSample, stageRefR);
            }


            xL = outputDC[0].process (outputAir[0].process (deEmphasis[0].process (xL)));
            xR = outputDC[1].process (outputAir[1].process (deEmphasis[1].process (xR)));
            if (std::abs (manualBalanceDb) > 1.0e-4)
            {
                const double gL = std::pow (10.0, -0.5 * manualBalanceDb / 20.0);
                const double gR = std::pow (10.0, +0.5 * manualBalanceDb / 20.0);
                xL *= gL;
                xR *= gR;
            }


            {
                const double outPower = 0.5 * (xL * xL + xR * xR);
                inRmsEnv  += autoGainRmsCoeff * (inPower  - inRmsEnv);
                outRmsEnv += autoGainRmsCoeff * (outPower - outRmsEnv);

                if (inRmsEnv > 1.0e-6 && outRmsEnv > 1.0e-6)
                {
                    const double ratio = std::sqrt (inRmsEnv / outRmsEnv);
                    const double targetGain = std::clamp (ratio, 0.2511886, 3.9810717);
                    autoGainLinear += autoGainSmoothCoeff * (targetGain - autoGainLinear);
                }
                else
                {
                    autoGainLinear += autoGainSmoothCoeff * (1.0 - autoGainLinear);
                }

                xL *= 1.0 + agMix * (autoGainLinear - 1.0);
                xR *= 1.0 + agMix * (autoGainLinear - 1.0);
            }

            xL *= outputTrimLinear;
            xR *= outputTrimLinear;

            left  = static_cast<float> (xL);
            right = static_cast<float> (xR);
        }

        void processSampleMono (float& sample) noexcept
        {
            double x = static_cast<double> (sample);
            const double agMix = autoGainMix.getNextValue();
            currentNoiseMix = noiseMix.getNextValue();
            currentHarmonicMix = harmonicMix.getNextValue(); currentBiasMix = biasMix.getNextValue();
            const double inPower = x * x;

            x = preEmphasis[0].process (subsonic[0].process (x));
            double stageRef = 0.17803894;

            const double inc50  = 2.0 * kPi * 50.0 / sampleRate;
            const double inc100 = 2.0 * kPi * 100.0 / sampleRate;
            humPhase50  += inc50;
            humPhase100 += inc100;
            if (humPhase50 >= 2.0 * kPi)  humPhase50  -= 2.0 * kPi;
            if (humPhase100 >= 2.0 * kPi) humPhase100 -= 2.0 * kPi;

            const double humSample = 3.2e-7 * std::sin (humPhase50) + 1.8e-7 * std::sin (humPhase100);

            for (int slotIdx = 0; slotIdx < kNumSlots; ++slotIdx)
            {
                const auto& cfg = slots[static_cast<size_t> (slotIdx)];
                for (auto& ch : stageStates)
                    ch[slotIdx].wet.setTargetValue (cfg.enabled && slotIdx < activeStageCount ? 1.0 : 0.0);

                x = processSingleStage (x, 0, slotIdx, cfg, humSample, stageRef);
            }

            x = outputDC[0].process (outputAir[0].process (deEmphasis[0].process (x)));

            {
                const double outPower = x * x;
                inRmsEnv  += autoGainRmsCoeff * (inPower  - inRmsEnv);
                outRmsEnv += autoGainRmsCoeff * (outPower - outRmsEnv);

                if (inRmsEnv > 1.0e-6 && outRmsEnv > 1.0e-6)
                {
                    const double ratio = std::sqrt (inRmsEnv / outRmsEnv);
                    const double targetGain = std::clamp (ratio, 0.2511886, 3.9810717);
                    autoGainLinear += autoGainSmoothCoeff * (targetGain - autoGainLinear);
                }
                else
                {
                    autoGainLinear += autoGainSmoothCoeff * (1.0 - autoGainLinear);
                }

                x *= 1.0 + agMix * (autoGainLinear - 1.0);
            }

            x *= outputTrimLinear;
            sample = static_cast<float> (x);
        }

    private:
        inline double nextWhiteNoise() noexcept
        {
            rngState ^= rngState >> 12;
            rngState ^= rngState << 25;
            rngState ^= rngState >> 27;
            const uint64_t val = rngState * 2685821657736338717ULL;
            return (static_cast<double> (val >> 11) * (1.0 / 9007199254740992.0)) * 2.0 - 1.0;
        }

        double processSingleStage (double x,
                                   int ch,
                                   int slotIdx,
                                   const SlotConfig& cfg,
                                   double humSample,
                                   double& stageRef) noexcept
        {
            auto& st = stageStates[static_cast<size_t> (ch)][static_cast<size_t> (slotIdx)];

            double y = x;

            for (auto& biq : st.filters)
                y = biq.process (y);

            y = applyConsoleHarmonics (y, cfg.model, cfg.drive, st, stageRef);

            if (cfg.noiseOn && currentNoiseMix > 0.0)
            {
                const double w = nextWhiteNoise();
                st.pinkB0 = 0.99765 * st.pinkB0 + w * 0.0990460;
                st.pinkB1 = 0.96300 * st.pinkB1 + w * 0.2965164;
                st.pinkB2 = 0.57000 * st.pinkB2 + w * 1.0526913;
                const double pink = (st.pinkB0 + st.pinkB1 + st.pinkB2 + w * 0.1848) * 1.6e-6;
                const double white = w * 6.5e-7;
                y += currentNoiseMix * (pink + white + humSample * 0.35);
            }

            if (std::abs (cfg.trimDb) > 1.0e-4)
            {
                const double trimLin = std::pow (10.0, cfg.trimDb / 20.0);
                y *= trimLin;
                stageRef *= trimLin;
            }

            return x + st.wet.getNextValue() * (y - x);
        }

        double applyConsoleHarmonics (double s,
                                      ConsoleModel model,
                                      double drive,
                                      StageChannelState& st,
                                      double stageRef) const noexcept
        {
            juce::ignoreUnused (model, drive, stageRef);
            const double colour = st.modelAmount.getNextValue();
            const double amount = (0.115 + 0.025 * colour) * (1.0 + 0.10 * std::abs (lastRawDrive));
            const double b = st.bias * currentBiasMix;
            if (b != st.cachedBias)
            {
                st.cachedBias = b; st.biasTanh = std::tanh (b);
                st.biasSlope = 1.0 - st.biasTanh * st.biasTanh;
            }
            const double tb = st.biasTanh;
            const double u = amount * s;
            const double soft = (std::tanh (u + b) - tb) / (amount * st.biasSlope);
            const double mix = std::clamp (currentHarmonicMix, 0.0, 1.0);
            return s + mix * (soft - s);
        }

        void updateGlobalFilters() noexcept
        {
            const double t = lastRawTone / 10.0;
            const double opening = std::max (0.0, -t);
            const double weight = std::max (0.0, t);
            // Audible balance is independent of the reciprocal saturation-emphasis pair.
            const double airDb = 1.25 * opening - 1.75 * weight;
            for (int ch=0; ch<2; ++ch)
            {
                const double slew = 1.0 - std::exp (-1.0 / (0.010 * sampleRate));
                for (auto* f : { &subsonic[ch], &outputDC[ch], &preEmphasis[ch], &deEmphasis[ch], &outputAir[ch] }) f->slew = slew;
                subsonic[ch].setTarget (BiquadSection::makeFirstOrderHighPass (21.0, sampleRate));
                outputDC[ch].setTarget (BiquadSection::makeFirstOrderHighPass (5.0, sampleRate));
                const auto pre = BiquadSection::makeHighShelf (4500.0, 0.70710678, -2.0 * t, sampleRate);
                preEmphasis[ch].setTarget (pre);
                BiquadSection inverse;
                inverse.b0=1.0/pre.b0; inverse.b1=pre.a1/pre.b0; inverse.b2=pre.a2/pre.b0;
                inverse.a1=pre.b1/pre.b0; inverse.a2=pre.b2/pre.b0;
                deEmphasis[ch].setTarget (inverse);
                outputAir[ch].setTarget (BiquadSection::makeHighShelf (8000.0, 0.70710678, airDb, sampleRate));
            }
        }

        void updateAllStageFilters() noexcept
        {
            updateGlobalFilters();
            for (int i = 0; i < kNumSlots; ++i)
                updateStageFilter (i);
        }

        void updateStageFilter (int index) noexcept
        {
            struct Curve { double bump, punch, dip, air, airHz; };
            static constexpr Curve curves[] { {0.24, 0.08, -0.10, -0.19, 17500.0},
                                               {1.12, 0.36, -0.30, -0.36, 16000.0},
                                               {0.85, 0.23, -0.24, -0.44, 15000.0} };
            const auto model = slots[index].model;
            const auto& c = curves[static_cast<int> (model)];
            for (int ch=0; ch<2; ++ch)
            {
                auto& st = stageStates[ch][index];
                st.modelAmount.setTargetValue (static_cast<double> (model));
                const double t = st.tolerance;
                const double opening = std::max (0.0, -lastRawTone / 10.0);
                const double weight = std::max (0.0, lastRawTone / 10.0);
                const double punchScale = 1.0 - 0.35 * opening + 0.20 * weight;
                const double dipScale = 1.0 - opening;
                const double rolloffScale = 1.0 - opening;
                st.filters[0].setTarget (BiquadSection::makePeaking (3200.0*t, 0.65, model == ConsoleModel::Spike ? 0.025 : -0.045, sampleRate));
                st.filters[1].setTarget (BiquadSection::makePeaking (27.0*t, 1.7, c.bump*0.30*subBumpScale, sampleRate));
                st.filters[2].setTarget (BiquadSection::makePeaking (52.8*t, 2.0, c.punch*punchScale, sampleRate));
                st.filters[3].setTarget (BiquadSection::makePeaking (92.0*t, 1.4, c.dip*dipScale, sampleRate));
                st.filters[4].setTarget (BiquadSection::makeHighShelf (c.airHz*t, 0.65, c.air*rolloffScale, sampleRate));
            }
        }

        std::array<BiquadSection, 2> subsonic, outputDC, preEmphasis, deEmphasis, outputAir;
        juce::SmoothedValue<double> autoGainMix, noiseMix, harmonicMix, biasMix;
        double currentHarmonicMix = 1.0, currentBiasMix = 1.0;
        double currentNoiseMix = 0.0;
        double sampleRate = 48000.0;
        double autoGainRmsCoeff = 0.00017;
        double autoGainSmoothCoeff = 0.00046;

        StereoBalanceMode balanceMode = StereoBalanceMode::SubAndMidLocked;
        int currentConsoleVariant = 0;
        int activeStageCount     = kNumSlots;
        double lastRawDrive      = 1.0;
        double lastRawTone       = 0.0;
        double subBumpScale      = 1.0;
        double globalHarmonicScale = 1.0;
        double dynamicEvenBias   = 1.0;
        bool masterNoiseEnabled  = true;
        bool autoGainEnabled     = true;
        double manualBalanceDb   = 0.0;
        double outputTrimLinear  = 1.0;

        double humPhase50  = 0.0;
        double humPhase100 = 0.0;
        double inRmsEnv    = 1.0e-8;
        double outRmsEnv   = 1.0e-8;
        double autoGainLinear = 1.0;
        uint64_t rngState  = 0x853C49E6748FEA9BULL;

        std::array<SlotConfig, kNumSlots> slots;
        std::array<std::array<StageChannelState, kNumSlots>, 2> stageStates;
    };
} // namespace jj8nls
