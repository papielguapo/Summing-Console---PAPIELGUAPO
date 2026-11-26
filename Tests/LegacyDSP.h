#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

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
        SubAndMidLocked = 0, // Default: Sub (20-40 Hz) + Mid/High (200 Hz-20 kHz) 1:1 L=R Locked
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

        void reset() noexcept
        {
            z1 = 0.0;
            z2 = 0.0;
        }

        inline double process (double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
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
        double linearGain = 1.0;

        // 1st-order DC blocker after Chebyshev even harmonics
        double dcX1 = 0.0;
        double dcY1 = 0.0;

        // Per-stage instant-attack / slow-release peak envelope so u = y / stagePeak
        // stays strictly in [-1.0, +1.0] at any level (-24 dBFS to +6 dBFS) without ever breaking audio
        double stagePeak = 0.2182729;

        // Paul Kellet 1/f pink noise filter state for authentic analog console low-end noise slope
        double pinkB0 = 0.0;
        double pinkB1 = 0.0;
        double pinkB2 = 0.0;

        void reset() noexcept
        {
            for (auto& f : filters)
                f.reset();
            dcX1 = 0.0;
            dcY1 = 0.0;
            stagePeak = 0.2182729;
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
            // Jaycen Joshua 8x NLS Buss Drum Chain:
            // 1: Mike (Drive 1.0, Noise Off)
            // 2: Spike (Drive 0.0, Noise On)
            // 3: Mike (Drive 0.0, Noise On)
            // 4: Mike (Drive 0.0, Noise On)
            // 5: Mike (Drive 0.0, Noise On)
            // 6: Spike (Drive 0.0, Noise On)
            // 7: Nevo (Drive 0.0, Noise On)
            // 8: Mike (Drive 0.0, Noise On)
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

        // Console Variant selector (0..4):
        //   0 = Jaycen Joshua 8x (5x Mike + 2x Spike + 1x Nevo)
        //   1 = All SSL Spike 8x (8x SSL 4000G — Cleanest / Flattest Sub / Open 17.5 kHz Top)
        //   2 = All Neve Nevo 8x (8x Neve 5116 — Warm Even Harmonics / Silky 15 kHz Top)
        //   3 = All EMI Mike 8x  (8x EMI TG12345 — Maximum +8.96 dB 27 Hz Sub & Punch)
        //   4 = FL HEAT Hybrid 8x (4x SSL Spike + 4x Neve Nevo — 1:1 FL HEAT / Crane Song Ocelot Bipolar Character:
        //       Drive > 0 = Triode/Tape Even Harmonics, Drive < 0 = Solid-State/Console Odd Harmonics + Tight Punch,
        //       Tone = Pre/Post-Emphasis Tilt & Sub/Air Balance)
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
                // FL HEAT Hybrid 8x: Alternates SSL Spike (Console/Solid-State Odd) and Neve Nevo (Triode/Transformer Even)
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
            setBipolarTone  (lastRawTone);
            updateAllStageFilters();
        }

        int getConsoleVariant() const noexcept { return currentConsoleVariant; }

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = std::max (8000.0, newSampleRate);
            dcPole = std::exp (-2.0 * kPi * 3.5 / sampleRate);
            rmsCoeff = 1.0 - std::exp (-1.0 / (0.150 * sampleRate)); // 150ms RMS ballistics
            peakReleaseCoeff = 1.0 - std::exp (-1.0 / (0.500 * sampleRate)); // 500ms smooth ceiling release
            autoGainRmsCoeff = 1.0 - std::exp (-1.0 / (0.120 * sampleRate)); // 120ms stereo-linked AutoGain RMS
            autoGainSmoothCoeff = 1.0 - std::exp (-1.0 / (0.045 * sampleRate)); // 45ms smooth AutoGain gain slewing
            transFastAttackCoeff = 1.0 - std::exp (-1.0 / (0.0015 * sampleRate)); // 1.5ms fast transient attack
            transFastReleaseCoeff = 1.0 - std::exp (-1.0 / (0.035 * sampleRate)); // 35ms fast transient release
            transSlowCoeff = 1.0 - std::exp (-1.0 / (0.090 * sampleRate)); // 90ms slow body tracker
            reset();
            updateAllStageFilters();
        }

        void reset() noexcept
        {
            for (auto& ch : stageStates)
                for (auto& st : ch)
                    st.reset();

            humPhase50  = 0.0;
            humPhase100 = 0.0;
            rmsEnvL     = 1.0e-8;
            rmsEnvR     = 1.0e-8;
            autoTrimL   = 1.0;
            autoTrimR   = 1.0;
            inRmsEnv    = 1.0e-8;
            outRmsEnv   = 1.0e-8;
            autoGainLinear = 1.0;
            transFastEnv = 1.0e-6;
            transSlowEnv = 1.0e-6;
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

        // Controls the 20-40 Hz transformer sub bump intensity (0.0 = flat sub, 1.0 = 1:1 NLS +6.2 dB, 1.5 = +9.3 dB)
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

        // Bipolar Tone [-10.0 .. +10.0] (Option B):
        //   0.0          = 100% 1:1 Jaycen Joshua (+6.2 dB @ 27 Hz, original -2.75 dB @ 16 kHz analog roll-off)
        //   0.0 .. +10.0 = Full Phat & Air: Boosts 27 Hz transformer sub up to 1.5x (+9.3 dB) AND opens 16 kHz high-shelf
        //                  (compensating analog roll-off up to +1.8 dB silky air at +10.0 so kicks/808s gain sub without losing top)
        //   0.0 .. -10.0 = Tight / Controlled Sub: Reduces 27 Hz transformer bump down to 0.0x (Flat Sub) while keeping warm analog top
        void setBipolarTone (double rawTone) noexcept
        {
            const double clamped = std::clamp (rawTone, -10.0, 10.0);
            lastRawTone = clamped;

            double newSubScale = 1.0;
            double newAirOffsetDb = 0.0;

            if (currentConsoleVariant == 4)
            {
                // FL HEAT Mode (Ocelot / Dave Hill style bipolar Tone):
                // Tone = 0.0: Controlled tight sub (0.45x = +2.0 dB @ 27 Hz) and neutral high-end (-0.8 dB @ 16 kHz)
                // Tone > 0.0: Bright Pre/Post-Emphasis (+0.85 dB/stage -> up to +4.2 dB silky air at 16 kHz) + moderate sub
                // Tone < 0.0: Warm Tape/Tube roll-off (-0.45 dB/stage darker top) + fuller warm bottom (up to 0.85x @ 27 Hz)
                newSubScale = (clamped >= 0.0)
                                ? (0.45 + (clamped / 10.0) * 0.25)
                                : (0.45 + (-clamped / 10.0) * 0.40);
                newAirOffsetDb = (clamped >= 0.0)
                                   ? (0.22 + (clamped / 10.0) * 0.63)
                                   : (0.22 + (clamped / 10.0) * 0.45);
            }
            else
            {
                newSubScale = (clamped >= 0.0)
                                ? (1.0 + (clamped / 10.0) * 0.5)
                                : std::max (0.0, 1.0 + (clamped / 10.0));
                // Per-stage high-shelf offset in dB: 0.0 dB at Tone <= 0, up to +0.57 dB/stage (+4.56 dB total across 8 stages) at Tone = +10
                newAirOffsetDb = (clamped > 0.0) ? ((clamped / 10.0) * 0.57) : 0.0;
            }

            if (std::abs (subBumpScale - newSubScale) > 1.0e-4
             || std::abs (airShelfOffsetDb - newAirOffsetDb) > 1.0e-4)
            {
                subBumpScale     = newSubScale;
                airShelfOffsetDb = newAirOffsetDb;
                updateAllStageFilters();
            }
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

        // Bipolar Drive helper [-10.0 .. +10.0]:
        //   +1.0         = 1:1 Jaycen Joshua 8x Chain (Slot 1 Drive 1.0, Global Harmonics 100%, Transient Exp 0.0)
        //   +1.0 .. +10.0 = Boosts Slot 1 Drive up to 12.0 + bus saturation
        //   +1.0 ..  0.0  = Reduces Slot 1 extra Drive down to 0.0
        //    0.0 .. -10.0 = Hybrid 1+3: Attenuates all 8 consoles' Chebyshev harmonics down to 0% (Clean Transformer)
        //                   and applies stereo-linked Transient Headroom Expansion (+1.5 dB punch recovery at -10.0)
        void setBipolarDrive (double rawDrive) noexcept
        {
            const double clamped = std::clamp (rawDrive, -10.0, 10.0);
            lastRawDrive = clamped;

            if (currentConsoleVariant == 4)
            {
                // Mode 5: FL HEAT Hybrid 8x (Dave Hill / Crane Song Ocelot Bipolar Law):
                //   Drive > 0 (0.0 .. +10.0): Bank 1 Triode / Tape Asymmetric Even-Harmonic Dominance (H2/H4/H6 boosted up to 2.4x)
                //   Drive < 0 (0.0 .. -10.0): Bank 0 Solid-State / Console Odd-Harmonic Dominance (H3/H5/H7 boosted up to 2.5x + transient punch)
                //   Drive == 0.0: Ultra-clean console transformer pass-through
                const double absNorm = std::clamp (std::abs (clamped) / 10.0, 0.0, 1.0);
                slots[0].drive = absNorm * 12.0;
                globalHarmonicScale = 0.25 + 1.35 * std::sqrt (absNorm);

                if (clamped >= 0.0)
                {
                    dynamicEvenBias = 1.0 + 1.40 * absNorm; // Strong Triode/Tape Even-Order (H2, H4, H6)
                    dynamicOddBias  = std::max (0.35, 1.0 - 0.55 * absNorm);
                    transientExpansionAmount = 0.0;
                }
                else
                {
                    dynamicEvenBias = std::max (0.25, 1.0 - 0.70 * absNorm);
                    dynamicOddBias  = 1.0 + 1.55 * absNorm; // Strong Solid-State/Console Odd-Order (H3, H5, H7)
                    transientExpansionAmount = 0.65 * absNorm; // Solid-state transient definition
                }
                return;
            }

            dynamicEvenBias = 1.0;
            dynamicOddBias  = 1.0;

            if (clamped >= 1.0)
            {
                slots[0].drive = std::clamp (clamped * 1.2, 0.0, 12.0);
                // Scale all 8 stages' harmonics proportionally when boosting Drive above +1.0 (1.0x at +1.0 up to 2.8x at +10.0)
                // so adding stages 1x -> 8x always scales all harmonics proportionally at any Drive position
                const double normPos = std::clamp ((clamped - 1.0) / 9.0, 0.0, 1.0);
                globalHarmonicScale = 1.0 + 1.8 * normPos;
                transientExpansionAmount = 0.0;
            }
            else if (clamped >= 0.0)
            {
                slots[0].drive = clamped;
                globalHarmonicScale = 1.0;
                transientExpansionAmount = 0.0;
            }
            else
            {
                slots[0].drive = 0.0;
                const double normNeg = std::clamp (-clamped / 10.0, 0.0, 1.0); // 0.0 at 0, 1.0 at -10
                // Smooth quadratic fade of all 8 stages' harmonics down to 0.0 (100% Clean Transformer EQ at -10)
                globalHarmonicScale = (1.0 - normNeg) * (1.0 - normNeg);
                transientExpansionAmount = normNeg;
            }
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

        void setMasterNoiseEnabled (bool en) noexcept { masterNoiseEnabled = en; }
        bool getMasterNoiseEnabled() const noexcept   { return masterNoiseEnabled; }
        void setActiveStageCount (int count) noexcept { activeStageCount = std::clamp (count, 1, kNumSlots); }
        int getActiveStageCount() const noexcept      { return activeStageCount; }
        void setAutoPeakRmsBalance (bool en) noexcept { autoPeakRmsBalance = en; }
        void setAutoGainEnabled (bool en) noexcept    { autoGainEnabled = en; }
        bool getAutoGainEnabled() const noexcept      { return autoGainEnabled; }
        void setManualBalanceDb (double db) noexcept  { manualBalanceDb = std::clamp (db, -3.0, 3.0); }
        void setOutputTrimDb (double db) noexcept     { outputTrimLinear = std::pow (10.0, std::clamp (db, -18.0, 18.0) / 20.0); }

        void processSampleStereo (float& left, float& right) noexcept
        {
            double xL = static_cast<double> (left);
            double xR = static_cast<double> (right);
            const double inPower = 0.5 * (xL * xL + xR * xR);

            double stageRefL = 0.2182729;
            double stageRefR = 0.2182729;

            // Advance 50 Hz & 100 Hz power-supply hum oscillators
            const double inc50  = 2.0 * kPi * 50.0 / sampleRate;
            const double inc100 = 2.0 * kPi * 100.0 / sampleRate;
            humPhase50  += inc50;
            humPhase100 += inc100;
            if (humPhase50 >= 2.0 * kPi)  humPhase50  -= 2.0 * kPi;
            if (humPhase100 >= 2.0 * kPi) humPhase100 -= 2.0 * kPi;

            const double humSample = 3.2e-7 * std::sin (humPhase50) + 1.8e-7 * std::sin (humPhase100);

            for (int slotIdx = 0; slotIdx < activeStageCount; ++slotIdx)
            {
                const auto& cfg = slots[static_cast<size_t> (slotIdx)];
                if (! cfg.enabled)
                    continue;

                xL = processSingleStage (xL, 0, slotIdx, cfg, humSample, stageRefL);
                xR = processSingleStage (xR, 1, slotIdx, cfg, humSample, stageRefR);
            }

            // Unity 1kHz reference makeup depending on balance mode and active console variant
            if (balanceMode == StereoBalanceMode::SubAndMidLocked)
            {
                const double makeup = getUnityMakeupForCurrentVariant();
                xL *= makeup;
                xR *= makeup;
            }
            else if (balanceMode == StereoBalanceMode::SubLockedMidTol)
            {
                const double makeup = getUnityMakeupForCurrentVariant();
                xL *= makeup;
                xR *= 1.0057730629; // +0.05 dB
            }

            // Optional real-time RMS/Peak auto-trim (keeps L/R identical even on asymmetric stereo input)
            if (autoPeakRmsBalance)
            {
                rmsEnvL += rmsCoeff * (xL * xL - rmsEnvL);
                rmsEnvR += rmsCoeff * (xR * xR - rmsEnvR);

                const double rmsL = std::sqrt (std::max (1.0e-10, rmsEnvL));
                const double rmsR = std::sqrt (std::max (1.0e-10, rmsEnvR));
                if (rmsL > 1.0e-4 && rmsR > 1.0e-4)
                {
                    const double avgRms = 0.5 * (rmsL + rmsR);
                    const double targetL = std::clamp (avgRms / rmsL, 0.794328, 1.258925); // +/- 2.0 dB max range
                    const double targetR = std::clamp (avgRms / rmsR, 0.794328, 1.258925);
                    autoTrimL += 0.0005 * (targetL - autoTrimL);
                    autoTrimR += 0.0005 * (targetR - autoTrimR);
                }
                xL *= autoTrimL;
                xR *= autoTrimR;
            }

            // Fine manual L/R balance pan (-3.0 dB .. +3.0 dB)
            if (std::abs (manualBalanceDb) > 1.0e-4)
            {
                const double gL = std::pow (10.0, -0.5 * manualBalanceDb / 20.0);
                const double gR = std::pow (10.0, +0.5 * manualBalanceDb / 20.0);
                xL *= gL;
                xR *= gR;
            }

            // Hybrid Negative Drive (0.0 .. -10.0): Stereo-linked Transient Headroom Expansion
            if (transientExpansionAmount > 1.0e-4)
            {
                const double peakLink = std::max (std::abs (xL), std::abs (xR));
                const double transGain = computeTransientExpansionGain (peakLink);
                xL *= transGain;
                xR *= transGain;
            }

            // Stereo-linked AutoGain: matches output RMS to input RMS (compensating for the +6.2 dB @ 27 Hz sub punch
            // and Drive boost when heavy 808s/kicks hit) while keeping L=R 100% locked (0.00 dB stereo skew)
            if (autoGainEnabled)
            {
                const double outPower = 0.5 * (xL * xL + xR * xR);
                inRmsEnv  += autoGainRmsCoeff * (inPower  - inRmsEnv);
                outRmsEnv += autoGainRmsCoeff * (outPower - outRmsEnv);

                // Gate at -60 dBFS so analog console noise floor in silence is never boosted
                if (inRmsEnv > 1.0e-6 && outRmsEnv > 1.0e-6)
                {
                    const double ratio = std::sqrt (inRmsEnv / outRmsEnv);
                    // Clamp AutoGain range to [-15.0 dB, +6.0 dB] so any Drive setting stays loudness-matched
                    const double targetGain = std::clamp (ratio, 0.1778279, 1.9952623);
                    autoGainLinear += autoGainSmoothCoeff * (targetGain - autoGainLinear);
                }
                else
                {
                    autoGainLinear += autoGainSmoothCoeff * (1.0 - autoGainLinear);
                }

                xL *= autoGainLinear;
                xR *= autoGainLinear;
            }

            xL *= outputTrimLinear;
            xR *= outputTrimLinear;

            left  = static_cast<float> (xL);
            right = static_cast<float> (xR);
        }

        void processSampleMono (float& sample) noexcept
        {
            double x = static_cast<double> (sample);
            const double inPower = x * x;

            double stageRef = 0.2182729;

            // Advance 50 Hz & 100 Hz power-supply hum oscillators
            const double inc50  = 2.0 * kPi * 50.0 / sampleRate;
            const double inc100 = 2.0 * kPi * 100.0 / sampleRate;
            humPhase50  += inc50;
            humPhase100 += inc100;
            if (humPhase50 >= 2.0 * kPi)  humPhase50  -= 2.0 * kPi;
            if (humPhase100 >= 2.0 * kPi) humPhase100 -= 2.0 * kPi;

            const double humSample = 3.2e-7 * std::sin (humPhase50) + 1.8e-7 * std::sin (humPhase100);

            for (int slotIdx = 0; slotIdx < activeStageCount; ++slotIdx)
            {
                const auto& cfg = slots[static_cast<size_t> (slotIdx)];
                if (! cfg.enabled)
                    continue;

                x = processSingleStage (x, 0, slotIdx, cfg, humSample, stageRef);
            }

            // Unity 1kHz reference makeup depending on balance mode and active console variant
            if (balanceMode == StereoBalanceMode::SubAndMidLocked
             || balanceMode == StereoBalanceMode::SubLockedMidTol)
            {
                x *= getUnityMakeupForCurrentVariant();
            }

            if (transientExpansionAmount > 1.0e-4)
                x *= computeTransientExpansionGain (std::abs (x));

            if (autoGainEnabled)
            {
                const double outPower = x * x;
                inRmsEnv  += autoGainRmsCoeff * (inPower  - inRmsEnv);
                outRmsEnv += autoGainRmsCoeff * (outPower - outRmsEnv);

                if (inRmsEnv > 1.0e-6 && outRmsEnv > 1.0e-6)
                {
                    const double ratio = std::sqrt (inRmsEnv / outRmsEnv);
                    const double targetGain = std::clamp (ratio, 0.1778279, 1.9952623);
                    autoGainLinear += autoGainSmoothCoeff * (targetGain - autoGainLinear);
                }
                else
                {
                    autoGainLinear += autoGainSmoothCoeff * (1.0 - autoGainLinear);
                }

                x *= autoGainLinear;
            }

            x *= outputTrimLinear;
            sample = static_cast<float> (x);
        }

    private:
        inline double getUnityMakeupForCurrentVariant() const noexcept
        {
            double full8Makeup = 1.0901844924;
            switch (currentConsoleVariant)
            {
                case 1:  full8Makeup = 1.0471285481; break; // All Spike 8x (+0.40 dB makeup for 8 x -0.05 dB)
                case 2:  full8Makeup = 1.0964781961; break; // All Nevo 8x  (+0.80 dB makeup for 8 x -0.10 dB)
                case 3:  full8Makeup = 1.1168632478; break; // All Mike 8x  (+0.96 dB makeup for 8 x -0.12 dB)
                case 4:  full8Makeup = 1.0715193052; break; // FL HEAT Hybrid 8x (+0.60 dB makeup)
                default: full8Makeup = 1.0901844924; break; // Jaycen Joshua 8x (+0.75 dB makeup)
            }
            if (activeStageCount >= kNumSlots)
                return full8Makeup;
            return std::pow (full8Makeup, static_cast<double> (activeStageCount) / static_cast<double> (kNumSlots));
        }

        inline double computeTransientExpansionGain (double absSample) noexcept
        {
            const double fastCoeff = (absSample > transFastEnv) ? transFastAttackCoeff : transFastReleaseCoeff;
            transFastEnv += fastCoeff * (absSample - transFastEnv);
            transSlowEnv += transSlowCoeff * (absSample - transSlowEnv);

            if (transSlowEnv > 1.0e-4 && transFastEnv > transSlowEnv)
            {
                // Ratio of fast attack envelope over slow body envelope -> up to +1.5 dB (+18.8%) transient punch at -10 Drive
                const double crestExcess = std::clamp ((transFastEnv / (transSlowEnv + 1.0e-5)) - 1.0, 0.0, 1.5);
                return 1.0 + transientExpansionAmount * 0.125 * crestExcess;
            }
            return 1.0;
        }

        inline double nextWhiteNoise() noexcept
        {
            // Fast xorshift64* PRNG in [-1.0, +1.0]
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

            // 1. Cascaded transformer + console linear frequency shaping
            double y = x * st.linearGain;
            stageRef *= st.linearGain;
            for (auto& biq : st.filters)
                y = biq.process (y);

            // Instant-attack / 500ms-release per-stage ceiling tracker:
            // Keeps u = y / normDenom strictly inside [-1.0, +1.0] even at 0 dBFS or +6 dBFS
            // with +6.2 dB @ 27 Hz sub boost, while keeping normDenom constant within each cycle!
            const double absY = std::abs (y);
            if (absY > st.stagePeak)
                st.stagePeak = absY;
            else
                st.stagePeak += peakReleaseCoeff * (std::max (stageRef, absY) - st.stagePeak);

            // 2. Chebyshev T2..T16 staircase harmonic exciter calibrated to THD = -34.3 dB / THD+N = -34.4 dB
            y = applyConsoleHarmonics (y, cfg.model, cfg.drive, st, stageRef);

            // 3. Analog console noise (1/f pink + white floor matching the low-frequency rise in PluginDoctor)
            if (masterNoiseEnabled && cfg.noiseOn)
            {
                const double w = nextWhiteNoise();
                st.pinkB0 = 0.99765 * st.pinkB0 + w * 0.0990460;
                st.pinkB1 = 0.96300 * st.pinkB1 + w * 0.2965164;
                st.pinkB2 = 0.57000 * st.pinkB2 + w * 1.0526913;
                const double pink = (st.pinkB0 + st.pinkB1 + st.pinkB2 + w * 0.1848) * 1.6e-6;
                const double white = w * 6.5e-7;
                y += pink + white + humSample * 0.35;
            }

            // 4. Per-slot Trim / VCA gain
            if (std::abs (cfg.trimDb) > 1.0e-4)
            {
                const double trimLin = std::pow (10.0, cfg.trimDb / 20.0);
                y *= trimLin;
                stageRef *= trimLin;
            }

            return y;
        }

        double applyConsoleHarmonics (double s,
                                      ConsoleModel model,
                                      double drive,
                                      StageChannelState& st,
                                      double stageRef) const noexcept
        {
            // Reference amplitude 0.2182729 (-13.22 dBFS in PluginDoctor HarmonicAnalysis)
            // Calibrated so the 8-stage chain hits THD = -34.3 dB (THD+N = -34.4 dB)
            // with the smooth, strictly monotonic H2 > H3 > H4 > H5 > ... > H16 staircase decay:
            //   H2  = -48.77 dBFS (-35.50 dB rel H1)
            //   H3  = -56.61 dBFS (-43.34 dB rel H1)
            //   H4  = -58.75 dBFS (-45.48 dB rel H1)
            //   H5  = -64.40 dBFS (-51.13 dB rel H1)
            //   H6  = -65.99 dBFS (-52.72 dB rel H1)
            //   H7  = -71.16 dBFS (-57.89 dB rel H1)
            //   H8  = -72.50 dBFS (-59.23 dB rel H1)
            //   H9  = -77.97 dBFS (-64.70 dB rel H1)
            //   H10 = -79.58 dBFS (-66.31 dB rel H1)
            //   H11 = -86.03 dBFS (-72.76 dB rel H1)
            //   H12 = -87.21 dBFS (-73.94 dB rel H1)
            //   H13 = -94.87 dBFS (-81.60 dB rel H1)
            //   H14 = -95.19 dBFS (-81.92 dB rel H1)
            //   H15 = -102.90 dBFS (-89.63 dB rel H1)
            //   H16 = -103.50 dBFS (-90.23 dB rel H1)
            const double drvMult = 1.0 + 0.38 * drive + 0.030 * drive * drive;

            double modelScale = 1.0;
            double evenBias   = 1.0;
            double oddBias    = 1.0;
            switch (model)
            {
                case ConsoleModel::Mike: // EMI TG12345 Mk IV
                    modelScale = 1.04;
                    evenBias   = 1.02;
                    oddBias    = 1.04;
                    break;

                case ConsoleModel::Spike: // SSL 4000G
                    modelScale = 0.86;
                    evenBias   = 0.92;
                    oddBias    = 0.96;
                    break;

                case ConsoleModel::Nevo: // Neve 5116
                default:
                    modelScale = 1.08;
                    evenBias   = 1.10;
                    oddBias    = 0.95;
                    break;
            }

            // Orthogonal Chebyshev weights T2..T16 with exact phase signs so 8 stages sum in-phase to -34.3 dB THD
            static constexpr double kBaseWeights[15] =
            {
                -4.116771e-04, // T2
                -1.706305e-04, // T3
                 1.264902e-04, // T4
                 7.113064e-05, // T5
                -5.272987e-05, // T6
                -3.327030e-05, // T7
                 2.355357e-05, // T8
                 1.556169e-05, // T9
                -9.595260e-06, // T10
                -6.339525e-06, // T11
                 3.404527e-06, // T12
                 2.410220e-06, // T13
                -1.166962e-06, // T14
                -9.595260e-07, // T15
                 2.800000e-07  // T16
            };

            static constexpr double kTkZero[15] =
            {
                -1.0,  0.0,  1.0,  0.0, -1.0,  0.0,  1.0,  0.0,
                -1.0,  0.0,  1.0,  0.0, -1.0,  0.0,  1.0
            };

            const double normDenom = std::max ({ stageRef, st.stagePeak, std::abs (s) });
            const double u = std::clamp (s / normDenom, -1.0, 1.0);
            const double scaleFactor = normDenom / std::max (1.0e-5, stageRef);
            double tPrev2 = 1.0; // T0
            double tPrev1 = u;   // T1
            double harmSum = 0.0;

            for (int k = 2; k <= 16; ++k)
            {
                const double tk = 2.0 * u * tPrev1 - tPrev2;
                tPrev2 = tPrev1;
                tPrev1 = tk;

                const double parityBias = ((k & 1) == 0) ? (evenBias * dynamicEvenBias)
                                                         : (oddBias  * dynamicOddBias);
                harmSum += kBaseWeights[k - 2] * parityBias * (tk - kTkZero[k - 2]);
            }

            // Scale harmonics proportionally to signal envelope so THD stays at -34.3 dB across all levels without clipping
            double out = s + globalHarmonicScale * drvMult * modelScale * scaleFactor * harmSum;

            // Gentle console bus headroom saturation at high signal + drive levels
            if (drive > 1.5)
            {
                const double satDrive = 1.0 + 0.04 * (drive - 1.5);
                out = std::tanh (out * satDrive) / satDrive;
            }

            // 1st-order DC blocker
            const double y = out - st.dcX1 + dcPole * st.dcY1;
            st.dcX1 = out;
            st.dcY1 = y;
            return y;
        }

        void updateAllStageFilters() noexcept
        {
            for (int i = 0; i < kNumSlots; ++i)
                updateStageFilter (i);
        }

        void updateStageFilter (int slotIdx) noexcept
        {
            const auto model = slots[static_cast<size_t> (slotIdx)].model;

            for (int ch = 0; ch < 2; ++ch)
            {
                auto& st = stageStates[static_cast<size_t> (ch)][static_cast<size_t> (slotIdx)];

                // Determine whether Sub (20-90 Hz) and Mid/High (200 Hz-20 kHz) use L=R locked or asymmetric curves
                const bool lockSub = (balanceMode == StereoBalanceMode::SubAndMidLocked
                                   || balanceMode == StereoBalanceMode::SubLockedMidTol);
                const bool lockMid = (balanceMode == StereoBalanceMode::SubAndMidLocked);

                const int subCh = lockSub ? 0 : ch;
                const int midCh = lockMid ? 0 : ch;

                // Sub-bass transformer resonance scaling (default 1.0 = exact NLS Buss sub bump)
                const double subScale = subBumpScale;

                if (model == ConsoleModel::Mike)
                {
                    // EMI TG12345 Mk IV stage
                    const double sub27Db = (lockSub ? 1.12 : (subCh == 0 ? 1.28 : 0.92)) * subScale;
                    const double sub27Hz = lockSub ? 27.0 : (subCh == 0 ? 26.8 : 27.2);
                    const double sub27Q  = lockSub ? 1.72 : (subCh == 0 ? 1.75 : 1.70);

                    const double sub52Db = (lockSub ? 0.36 : (subCh == 0 ? 0.44 : 0.26)) * subScale;
                    const double sub52Hz = lockSub ? 52.8 : (subCh == 0 ? 53.0 : 52.5);
                    const double sub52Q  = lockSub ? 2.15 : (subCh == 0 ? 2.20 : 2.10);

                    const double dip92Db = (lockSub ? -0.30 : (subCh == 0 ? -0.22 : -0.38)) * subScale;
                    const double dip92Hz = lockSub ? 92.0 : (subCh == 0 ? 93.0 : 91.0);

                    const double hsDb    = (lockMid ? -0.36 : (midCh == 0 ? -0.35 : -0.38)) + airShelfOffsetDb;
                    const double hsHz    = lockMid ? 16000.0 : (midCh == 0 ? 16200.0 : 15900.0);
                    const double stageDb = lockMid ? -0.12 : (midCh == 0 ? -0.17 : -0.07);

                    st.filters[0] = BiquadSection::makeHighPass  (7.5,     0.72,    sampleRate);
                    st.filters[1] = BiquadSection::makePeaking   (sub27Hz, sub27Q,  sub27Db, sampleRate);
                    st.filters[2] = BiquadSection::makePeaking   (sub52Hz, sub52Q,  sub52Db, sampleRate);
                    st.filters[3] = BiquadSection::makePeaking   (dip92Hz, 1.45,    dip92Db, sampleRate);
                    st.filters[4] = BiquadSection::makeHighShelf (hsHz,    0.65,    hsDb,    sampleRate);
                    st.linearGain = std::pow (10.0, stageDb / 20.0);
                }
                else if (model == ConsoleModel::Spike)
                {
                    // SSL 4000G stage
                    const double sub27Db = (lockSub ? 0.24 : (subCh == 0 ? 0.28 : 0.20)) * subScale;
                    const double sub52Db = (lockSub ? 0.08 : (subCh == 0 ? 0.10 : 0.06)) * subScale;
                    const double dip92Db = (lockSub ? -0.10 : (subCh == 0 ? -0.08 : -0.12)) * subScale;
                    const double hsDb    = (lockMid ? -0.19 : (midCh == 0 ? -0.18 : -0.20)) + airShelfOffsetDb;
                    const double stageDb = lockMid ? -0.05 : (midCh == 0 ? -0.08 : -0.03);

                    st.filters[0] = BiquadSection::makeHighPass  (6.0,     0.71,   sampleRate);
                    st.filters[1] = BiquadSection::makePeaking   (26.5,    1.40,   sub27Db, sampleRate);
                    st.filters[2] = BiquadSection::makePeaking   (52.0,    1.80,   sub52Db, sampleRate);
                    st.filters[3] = BiquadSection::makePeaking   (92.0,    1.30,   dip92Db, sampleRate);
                    st.filters[4] = BiquadSection::makeHighShelf (17500.0, 0.65,   hsDb,    sampleRate);
                    st.linearGain = std::pow (10.0, stageDb / 20.0);
                }
                else // ConsoleModel::Nevo (Neve 5116)
                {
                    const double sub27Db = (lockSub ? 0.85 : (subCh == 0 ? 0.95 : 0.72)) * subScale;
                    const double sub52Db = (lockSub ? 0.23 : (subCh == 0 ? 0.28 : 0.18)) * subScale;
                    const double dip92Db = (lockSub ? -0.24 : (subCh == 0 ? -0.20 : -0.28)) * subScale;
                    const double hsDb    = (lockMid ? -0.44 : (midCh == 0 ? -0.42 : -0.45)) + airShelfOffsetDb;
                    const double stageDb = lockMid ? -0.10 : (midCh == 0 ? -0.14 : -0.06);

                    st.filters[0] = BiquadSection::makeHighPass  (8.0,     0.74,   sampleRate);
                    st.filters[1] = BiquadSection::makePeaking   (27.5,    1.65,   sub27Db, sampleRate);
                    st.filters[2] = BiquadSection::makePeaking   (54.0,    2.00,   sub52Db, sampleRate);
                    st.filters[3] = BiquadSection::makePeaking   (94.0,    1.40,   dip92Db, sampleRate);
                    st.filters[4] = BiquadSection::makeHighShelf (15000.0, 0.65,   hsDb,    sampleRate);
                    st.linearGain = std::pow (10.0, stageDb / 20.0);
                }
            }
        }

        double sampleRate = 48000.0;
        double dcPole     = 0.9995;
        double rmsCoeff   = 0.0001;
        double peakReleaseCoeff = 0.00004;
        double autoGainRmsCoeff = 0.00017;
        double autoGainSmoothCoeff = 0.00046;
        double transFastAttackCoeff  = 0.0138;
        double transFastReleaseCoeff = 0.0006;
        double transSlowCoeff        = 0.00023;

        StereoBalanceMode balanceMode = StereoBalanceMode::SubAndMidLocked;
        int currentConsoleVariant = 0;
        int activeStageCount     = kNumSlots;
        double lastRawDrive      = 1.0;
        double lastRawTone       = 0.0;
        double subBumpScale      = 1.0;
        double airShelfOffsetDb  = 0.0;
        double globalHarmonicScale = 1.0;
        double dynamicEvenBias   = 1.0;
        double dynamicOddBias    = 1.0;
        double transientExpansionAmount = 0.0;
        bool masterNoiseEnabled  = true;
        bool autoPeakRmsBalance  = true;
        bool autoGainEnabled     = true;
        double manualBalanceDb   = 0.0;
        double outputTrimLinear  = 1.0;

        double humPhase50  = 0.0;
        double humPhase100 = 0.0;
        double rmsEnvL     = 1.0e-8;
        double rmsEnvR     = 1.0e-8;
        double autoTrimL   = 1.0;
        double autoTrimR   = 1.0;
        double inRmsEnv    = 1.0e-8;
        double outRmsEnv   = 1.0e-8;
        double autoGainLinear = 1.0;
        double transFastEnv = 1.0e-6;
        double transSlowEnv = 1.0e-6;
        uint64_t rngState  = 0x853C49E6748FEA9BULL;

        std::array<SlotConfig, kNumSlots> slots;
        std::array<std::array<StageChannelState, kNumSlots>, 2> stageStates;
    };
} // namespace jj8nls
