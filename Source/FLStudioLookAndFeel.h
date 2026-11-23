#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace flheat
{

enum SkinStyle
{
    kSkinFLStudio    = 0, // Skin 0: FL Studio Dark Mixer UI
    kSkinProTools    = 1, // Skin 1: Pro Tools HEAT 1:1 Original Strip (Screenshot clone)
    kSkinAtariPC     = 2, // Skin 2: Atari ST / Retro PC (TOS GEM / Amber & Green CRT aesthetic)
    kSkinAbletonLive = 3, // Skin 3: Ableton Live 12 (Flat vector grey, sharp borders, yellow/cyan macro arcs)
    kSkinLogicProX   = 4, // Skin 4: Apple Logic Pro X (Space-grey brushed metal, cyan neon ring, charcoal caps)
    kSkinCubase      = 5, // Skin 5: Steinberg Cubase 14 (Deep slate MixConsole strip, blue/orange LED ring)
    kNumSkins        = 6
};

/**
 * 6-Skin Vector LookAndFeel for FL HEAT:
 * - 0: FL Studio Dark Mixer UI
 * - 1: Pro Tools HEAT 1:1 Original Strip
 * - 2: Atari ST / Retro PC (GEM Desktop + CRT Phosphor)
 * - 3: Ableton Live 12 (Minimalist flat vector rack device)
 * - 4: Apple Logic Pro X (Modern dark brushed aluminum & cyan glow)
 * - 5: Steinberg Cubase 14 (MixConsole dark strip)
 */
class FLStudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // =========================================================================
    // SKIN 0: FL Studio Dark Mixer Palette
    // =========================================================================
    static inline const juce::Colour kChassisBg       { 0xff262a2e }; // #262A2E
    static inline const juce::Colour kChassisBorder   { 0xff171a1d }; // #171A1D
    static inline const juce::Colour kChassisBevel    { 0xff363b40 }; // #363B40
    static inline const juce::Colour kRecessedPanel   { 0xff1e2124 }; // #1E2124
    static inline const juce::Colour kRecessedBorder  { 0xff141618 }; // #141618
    static inline const juce::Colour kTextPrimary     { 0xffd4d8dc }; // #D4D8DC
    static inline const juce::Colour kTextHeader      { 0xffe2e6ea }; // #E2E6EA
    static inline const juce::Colour kTextSecondary   { 0xff8c939a }; // #8C939A
    static inline const juce::Colour kAccentOrange    { 0xffff9e2c }; // #FF9E2C (+ Tube/Tape)
    static inline const juce::Colour kAccentCyan      { 0xff42a5f5 }; // #42A5F5 (- Transistor)
    static inline const juce::Colour kButtonOff       { 0xff34393e }; // #34393E

    // =========================================================================
    // SKIN 1: Pro Tools Classic HEAT 1:1 Palette (from reference screenshot)
    // =========================================================================
    static inline const juce::Colour kPTOuterFrame    { 0xff6e7072 };
    static inline const juce::Colour kPTDarkBody      { 0xff37393b };
    static inline const juce::Colour kPTBodyBorder    { 0xff1f2123 };
    static inline const juce::Colour kPTInnerCard     { 0xff5a5c5e };
    static inline const juce::Colour kPTInnerBorder   { 0xff2c2e30 };
    static inline const juce::Colour kPTInnerHi       { 0xff707275 };
    static inline const juce::Colour kPTTitleText     { 0xffd6d8da };
    static inline const juce::Colour kPTLabelText     { 0xffd0d2d4 };
    static inline const juce::Colour kPTOmitirTextOff { 0xff888a8c };
    static inline const juce::Colour kPTOmitirTextOn  { 0xff232527 };
    static inline const juce::Colour kPTGreenLED      { 0xff82b83a };
    static inline const juce::Colour kPTAmberBypass   { 0xffe5a024 };

    // =========================================================================
    // SKIN 2: Atari ST / Retro PC (TOS GEM + CRT Phosphor Green/Amber) Palette
    // =========================================================================
    static inline const juce::Colour kAtariBeige      { 0xffd8d4c8 };
    static inline const juce::Colour kAtariLightBevel { 0xfff8f6f0 };
    static inline const juce::Colour kAtariDarkShadow { 0xff6e6a60 };
    static inline const juce::Colour kAtariBlack      { 0xff101410 };
    static inline const juce::Colour kAtariGreenCRT   { 0xff33ff66 };
    static inline const juce::Colour kAtariAmberCRT   { 0xffffb000 };

    // =========================================================================
    // SKIN 3: Ableton Live 12 Palette
    // =========================================================================
    static inline const juce::Colour kLiveChassisBg   { 0xff383838 }; // Ableton device rack grey
    static inline const juce::Colour kLivePanelBg     { 0xff2b2b2b }; // Flat dark module box
    static inline const juce::Colour kLiveBorder      { 0xff161616 }; // 1px sharp black border
    static inline const juce::Colour kLiveHeaderBg    { 0xff464646 }; // Device title bar
    static inline const juce::Colour kLiveYellow      { 0xffffb900 }; // Ableton Active Yellow/Amber
    static inline const juce::Colour kLiveCyan        { 0xff00d2ff }; // Ableton Macro Cyan
    static inline const juce::Colour kLiveText        { 0xffe6e6e6 };

    // =========================================================================
    // SKIN 4: Apple Logic Pro X Palette
    // =========================================================================
    static inline const juce::Colour kLogicTopBg      { 0xff34373d }; // Space-grey anodized top
    static inline const juce::Colour kLogicBotBg      { 0xff25282c }; // Space-grey anodized bottom
    static inline const juce::Colour kLogicCardBg     { 0xff1b1d21 }; // Inset dark glass card
    static inline const juce::Colour kLogicCardBorder { 0xff3d424a }; // Subtle metallic rim
    static inline const juce::Colour kLogicCyan       { 0xff26d6ff }; // Logic signature cyan ring
    static inline const juce::Colour kLogicOrange     { 0xffff9f1c }; // Logic warm tube ring
    static inline const juce::Colour kLogicPowerBlue  { 0xff1997ff }; // Logic blue power button

    // =========================================================================
    // SKIN 5: Steinberg Cubase 14 MixConsole Palette
    // =========================================================================
    static inline const juce::Colour kCubaseChassis   { 0xff1e2228 }; // Cubase deep slate
    static inline const juce::Colour kCubasePanel     { 0xff15181c }; // Recessed rack bay
    static inline const juce::Colour kCubaseBorder    { 0xff2d333c }; // Etched divider
    static inline const juce::Colour kCubaseOrange    { 0xfff37d22 }; // Steinberg warm orange
    static inline const juce::Colour kCubaseBlue      { 0xff32a2ff }; // Cubase MixConsole blue

    FLStudioLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, kTextPrimary);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TooltipWindow::backgroundColourId, kRecessedPanel);
        setColour (juce::TooltipWindow::textColourId, kTextHeader);
        setColour (juce::TooltipWindow::outlineColourId, kAccentOrange.withAlpha (0.6f));
        setColour (juce::PopupMenu::backgroundColourId, kRecessedPanel);
        setColour (juce::PopupMenu::textColourId, kTextHeader);
        setColour (juce::PopupMenu::headerTextColourId, kAccentOrange);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccentOrange.withAlpha (0.28f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    void setSkinStyle (int styleIndex) noexcept
    {
        skinStyle = juce::jlimit (0, static_cast<int> (kNumSkins) - 1, styleIndex);
    }

    int getSkinStyle() const noexcept
    {
        return skinStyle;
    }

    static juce::Font getFLFont (float heightPx, bool bold = false)
    {
        juce::FontOptions opts (juce::Font::getDefaultSansSerifFontName(),
                                std::max (4.5f, heightPx),
                                bold ? juce::Font::bold : juce::Font::plain);
        return juce::Font (opts);
    }

    static juce::Font getAtariFont (float heightPx, bool bold = true)
    {
        juce::FontOptions opts (juce::Font::getDefaultMonospacedFontName(),
                                std::max (5.0f, heightPx),
                                bold ? juce::Font::bold : juce::Font::plain);
        return juce::Font (opts);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        switch (skinStyle)
        {
            case kSkinProTools:
                drawProToolsRotarySlider (g, x, y, width, height,
                                          sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            case kSkinAtariPC:
                drawAtariRotarySlider (g, x, y, width, height,
                                       sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            case kSkinAbletonLive:
                drawAbletonRotarySlider (g, x, y, width, height,
                                         sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            case kSkinLogicProX:
                drawLogicProRotarySlider (g, x, y, width, height,
                                          sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            case kSkinCubase:
                drawCubaseRotarySlider (g, x, y, width, height,
                                        sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            default:
                drawFLStudioRotarySlider (g, x, y, width, height,
                                          sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
                return;
        }
    }

    // =========================================================================
    // SKIN 3: Ableton Live 12 Flat Vector Macro Dial
    // =========================================================================
    void drawAbletonRotarySlider (juce::Graphics& g,
                                  int x, int y, int width, int height,
                                  float sliderPosProportional,
                                  float rotaryStartAngle,
                                  float rotaryEndAngle,
                                  juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center = bounds.getCentre();
        const float scale = diameter / 70.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);
        const float centerAngle  = 0.5f * (rotaryStartAngle + rotaryEndAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.4f;

        const float arcRadius = diameter * 0.39f;
        const float strokeW   = std::max (2.0f, 3.6f * scale);

        // 1. Flat dark background track arc
        juce::Path trackPath;
        trackPath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff181818).withAlpha (alphaMul));
        g.strokePath (trackPath, juce::PathStrokeType (strokeW,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::butt));

        // 2. Bipolar active arc (Ableton Yellow for positive, Ableton Cyan for negative)
        if (std::abs (curVal) > 0.02)
        {
            const juce::Colour activeCol = (curVal > 0.0) ? kLiveYellow : kLiveCyan;
            const float startA = std::min (centerAngle, currentAngle);
            const float endA   = std::max (centerAngle, currentAngle);

            juce::Path activePath;
            activePath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                      0.0f, startA, endA, true);
            g.setColour (activeCol.withAlpha (alphaMul));
            g.strokePath (activePath, juce::PathStrokeType (strokeW,
                                                            juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::butt));
        }

        // 3. Flat minimalist Ableton inner disc
        const float innerR = diameter * 0.28f;
        g.setColour (juce::Colour (0xff343434).withAlpha (alphaMul));
        g.fillEllipse (center.x - innerR, center.y - innerR, innerR * 2.0f, innerR * 2.0f);
        g.setColour (kLiveBorder.withAlpha (alphaMul));
        g.drawEllipse (center.x - innerR, center.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.1f * scale);

        // 4. Crisp straight indicator line from center to outer arc
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);
        const float ptrR = arcRadius + strokeW * 0.35f;

        g.setColour (kLiveText.withAlpha (alphaMul));
        g.drawLine (center.x, center.y,
                    center.x + sinP * ptrR,
                    center.y + cosP * ptrR,
                    2.1f * scale);
    }

    // =========================================================================
    // SKIN 4: Apple Logic Pro X Anodized Rotary Knob
    // =========================================================================
    void drawLogicProRotarySlider (juce::Graphics& g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center = bounds.getCentre();
        const float scale = diameter / 72.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);
        const float centerAngle  = 0.5f * (rotaryStartAngle + rotaryEndAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.4f;

        // 1. Recessed dark halo ring
        const float arcRadius = diameter * 0.405f;
        const float arcW = std::max (1.8f, 3.0f * scale);

        juce::Path trackPath;
        trackPath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff101215).withAlpha (alphaMul));
        g.strokePath (trackPath, juce::PathStrokeType (arcW,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // 2. Logic Pro glowing neon ring (Orange for + Tube, Cyan for - Solid State)
        if (std::abs (curVal) > 0.02)
        {
            const juce::Colour glowCol = (curVal > 0.0) ? kLogicOrange : kLogicCyan;
            const float startA = std::min (centerAngle, currentAngle);
            const float endA   = std::max (centerAngle, currentAngle);

            juce::Path bloomPath;
            bloomPath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                     0.0f, startA, endA, true);
            g.setColour (glowCol.withAlpha (0.28f * alphaMul));
            g.strokePath (bloomPath, juce::PathStrokeType (arcW * 2.4f,
                                                           juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));

            g.setColour (glowCol.withAlpha (alphaMul));
            g.strokePath (bloomPath, juce::PathStrokeType (arcW,
                                                           juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
        }

        // 3. Brushed metallic outer bezel + charcoal cap
        const float bodyR = diameter * 0.32f;
        const juce::Rectangle<float> bodyRect (center.x - bodyR, center.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.6f * alphaMul));
        g.fillEllipse (bodyRect.translated (0.0f, 2.0f * scale));

        juce::ColourGradient rimGrad (juce::Colour (0xff646a73).withAlpha (alphaMul),
                                      bodyRect.getX(), bodyRect.getY(),
                                      juce::Colour (0xff2a2d32).withAlpha (alphaMul),
                                      bodyRect.getX(), bodyRect.getBottom(),
                                      false);
        g.setGradientFill (rimGrad);
        g.fillEllipse (bodyRect);

        const auto capRect = bodyRect.reduced (2.2f * scale);
        juce::ColourGradient capGrad (juce::Colour (0xff3c4047).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getY(),
                                      juce::Colour (0xff22252a).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getBottom(),
                                      false);
        g.setGradientFill (capGrad);
        g.fillEllipse (capRect);

        // 4. Logic Pro Glowing Cyan/Orange Tip Dot + Pointer Line
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);
        const float ptrInner = bodyR * 0.25f;
        const float ptrOuter = bodyR * 0.80f;

        const juce::Colour ptrCol = (std::abs (curVal) < 0.05)
                                      ? juce::Colours::white
                                      : ((curVal > 0.0) ? kLogicOrange : kLogicCyan);
        g.setColour (ptrCol.withAlpha (alphaMul));
        g.drawLine (center.x + sinP * ptrInner, center.y + cosP * ptrInner,
                    center.x + sinP * ptrOuter, center.y + cosP * ptrOuter,
                    2.0f * scale);
    }

    // =========================================================================
    // SKIN 5: Steinberg Cubase 14 MixConsole Rotary Encoder
    // =========================================================================
    void drawCubaseRotarySlider (juce::Graphics& g,
                                 int x, int y, int width, int height,
                                 float sliderPosProportional,
                                 float rotaryStartAngle,
                                 float rotaryEndAngle,
                                 juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center = bounds.getCentre();
        const float scale = diameter / 72.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);
        const float centerAngle  = 0.5f * (rotaryStartAngle + rotaryEndAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.4f;

        // 1. Segmented Cubase MixConsole LED ring (21 discrete rectangular segments)
        constexpr int kNumSegs = 21;
        const float segR = diameter * 0.415f;

        for (int i = 0; i < kNumSegs; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (kNumSegs - 1);
            const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const float sinA = std::sin (a);
            const float cosA = -std::cos (a);

            const bool isCenter = (i == kNumSegs / 2);
            bool isLit = false;
            if (curVal > 0.05 && a >= centerAngle - 0.02f && a <= currentAngle + 0.05f)
                isLit = true;
            else if (curVal < -0.05 && a <= centerAngle + 0.02f && a >= currentAngle - 0.05f)
                isLit = true;
            else if (isCenter)
                isLit = true;

            const juce::Colour litCol = (curVal > 0.05)
                                          ? kCubaseOrange
                                          : ((curVal < -0.05) ? kCubaseBlue : juce::Colour (0xff90a0b0));

            g.setColour (isLit ? litCol.withAlpha (alphaMul)
                               : juce::Colour (0xff262c34).withAlpha (alphaMul));
            g.drawLine (center.x + sinA * (segR - 2.4f * scale),
                        center.y + cosA * (segR - 2.4f * scale),
                        center.x + sinA * (segR + 2.0f * scale),
                        center.y + cosA * (segR + 2.0f * scale),
                        2.1f * scale);
        }

        // 2. Cubase stepped dark gunmetal encoder cap
        const float outerR = diameter * 0.31f;
        g.setColour (juce::Colour (0xff0e1013).withAlpha (alphaMul));
        g.fillEllipse (center.x - outerR, center.y - outerR, outerR * 2.0f, outerR * 2.0f);

        const float innerR = diameter * 0.275f;
        const juce::Rectangle<float> capRect (center.x - innerR, center.y - innerR, innerR * 2.0f, innerR * 2.0f);
        juce::ColourGradient capGrad (juce::Colour (0xff39414c).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getY(),
                                      juce::Colour (0xff1f242b).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getBottom(),
                                      false);
        g.setGradientFill (capGrad);
        g.fillEllipse (capRect);

        g.setColour (juce::Colour (0xff505a68).withAlpha (0.6f * alphaMul));
        g.drawEllipse (capRect, 1.0f * scale);

        // 3. Cubase crisp notch pointer
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);
        g.setColour (juce::Colours::white.withAlpha (alphaMul));
        g.drawLine (center.x + sinP * (innerR * 0.32f),
                    center.y + cosP * (innerR * 0.32f),
                    center.x + sinP * (innerR * 0.88f),
                    center.y + cosP * (innerR * 0.88f),
                    1.9f * scale);
    }

    // =========================================================================
    // SKIN 2: Atari ST / Retro PC Rotary Knob (CRT Phosphor + GEM Pixel Dial)
    // =========================================================================
    void drawAtariRotarySlider (juce::Graphics& g,
                                int x, int y, int width, int height,
                                float sliderPosProportional,
                                float rotaryStartAngle,
                                float rotaryEndAngle,
                                juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center = bounds.getCentre();
        const float scale = diameter / 68.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);
        const float centerAngle  = 0.5f * (rotaryStartAngle + rotaryEndAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.45f;

        constexpr int kNumBlocks = 19;
        const float ledRadius = diameter * 0.43f;
        const float blockSz = std::max (2.0f, 3.2f * scale);

        for (int i = 0; i < kNumBlocks; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (kNumBlocks - 1);
            const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const float bx = center.x + std::sin (a) * ledRadius - blockSz * 0.5f;
            const float by = center.y - std::cos (a) * ledRadius - blockSz * 0.5f;

            const bool isCenter = (i == kNumBlocks / 2);
            bool isLit = false;
            if (curVal > 0.05 && a >= centerAngle - 0.02f && a <= currentAngle + 0.05f)
                isLit = true;
            else if (curVal < -0.05 && a <= centerAngle + 0.02f && a >= currentAngle - 0.05f)
                isLit = true;
            else if (isCenter && std::abs (curVal) <= 0.05)
                isLit = true;

            const juce::Colour litCol = (curVal > 0.05) ? kAtariAmberCRT : kAtariGreenCRT;
            g.setColour (isLit ? litCol.withAlpha (alphaMul)
                               : juce::Colour (0xff1d2b1d).withAlpha (alphaMul));
            g.fillRect (bx, by, blockSz, blockSz);
        }

        const float bodyR = diameter * 0.31f;
        const juce::Rectangle<float> bodyRect (center.x - bodyR, center.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (kAtariBlack.withAlpha (0.85f * alphaMul));
        g.fillEllipse (bodyRect.translated (2.0f * scale, 2.0f * scale));

        g.setColour (kAtariBeige.withAlpha (alphaMul));
        g.fillEllipse (bodyRect);
        g.setColour (kAtariBlack.withAlpha (alphaMul));
        g.drawEllipse (bodyRect, 1.5f * scale);

        const auto innerRect = bodyRect.reduced (3.5f * scale);
        g.setColour (kAtariBlack.withAlpha (alphaMul));
        g.fillEllipse (innerRect);

        g.setColour (kAtariGreenCRT.withAlpha (0.22f * alphaMul));
        g.drawHorizontalLine (static_cast<int> (center.y), innerRect.getX() + 2.0f, innerRect.getRight() - 2.0f);
        g.drawVerticalLine (static_cast<int> (center.x), innerRect.getY() + 2.0f, innerRect.getBottom() - 2.0f);

        const float ptrOuter = bodyR * 0.80f;
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);
        const juce::Colour ptrCol = (curVal > 0.05) ? kAtariAmberCRT : kAtariGreenCRT;

        g.setColour (ptrCol.withAlpha (alphaMul));
        g.drawLine (center.x, center.y,
                    center.x + sinP * ptrOuter,
                    center.y + cosP * ptrOuter,
                    2.2f * scale);

        const float hubSz = 3.6f * scale;
        g.fillRect (center.x - hubSz * 0.5f, center.y - hubSz * 0.5f, hubSz, hubSz);
    }

    // =========================================================================
    // SKIN 1: Pro Tools HEAT 1:1 Rotary Knob (Exact match to screenshot)
    // =========================================================================
    void drawProToolsRotarySlider (juce::Graphics& g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center (bounds.getCentreX(), bounds.getCentreY() + diameter * 0.04f);
        const float scale = diameter / 62.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.55f;

        const float outerRadius = diameter * 0.39f;
        const float grooveThickness = std::max (2.0f, 4.2f * scale);

        {
            juce::Path darkWellPath;
            darkWellPath.addCentredArc (center.x, center.y, outerRadius, outerRadius,
                                        0.0f, rotaryStartAngle - 0.08f, rotaryEndAngle + 0.08f, true);
            g.setColour (juce::Colour (0xff2d2f31).withAlpha (0.9f * alphaMul));
            g.strokePath (darkWellPath, juce::PathStrokeType (grooveThickness * 1.45f,
                                                              juce::PathStrokeType::curved,
                                                              juce::PathStrokeType::rounded));
        }

        const float collarRadius = diameter * 0.355f;
        const float collarThickness = std::max (1.8f, 3.3f * scale);
        {
            juce::Path collarPath;
            collarPath.addCentredArc (center.x, center.y, collarRadius, collarRadius,
                                      0.0f, rotaryStartAngle, rotaryEndAngle, true);

            juce::ColourGradient collarGrad (juce::Colour (0xff8a8c8f).withAlpha (alphaMul),
                                             center.x, center.y - collarRadius,
                                             juce::Colour (0xff484a4d).withAlpha (alphaMul),
                                             center.x, center.y + collarRadius,
                                             false);
            g.setGradientFill (collarGrad);
            g.strokePath (collarPath, juce::PathStrokeType (collarThickness,
                                                            juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));

            if (std::abs (curVal) > 0.05)
            {
                const float centerAngle = 0.5f * (rotaryStartAngle + rotaryEndAngle);
                const float startA = std::min (centerAngle, currentAngle);
                const float endA   = std::max (centerAngle, currentAngle);

                juce::Path activeArc;
                activeArc.addCentredArc (center.x, center.y, collarRadius, collarRadius,
                                         0.0f, startA, endA, true);
                const juce::Colour arcCol = (curVal >= 0.0)
                                              ? juce::Colour (0xff97d844)
                                              : juce::Colour (0xff64b5f6);
                g.setColour (arcCol.withAlpha (0.55f * alphaMul));
                g.strokePath (activeArc, juce::PathStrokeType (collarThickness * 0.75f,
                                                               juce::PathStrokeType::curved,
                                                               juce::PathStrokeType::rounded));
            }
        }

        {
            const float ledY = center.y - outerRadius - 2.2f * scale;
            const float ledW = 5.2f * scale;
            const float ledH = 6.2f * scale;
            const juce::Rectangle<float> ledRect (center.x - ledW * 0.5f,
                                                  ledY - ledH * 0.45f,
                                                  ledW,
                                                  ledH);

            const float glowR = 8.0f * scale;
            juce::ColourGradient halo (kPTGreenLED.withAlpha (0.55f * alphaMul),
                                       ledRect.getCentreX(), ledRect.getCentreY(),
                                       kPTGreenLED.withAlpha (0.0f),
                                       ledRect.getCentreX() + glowR, ledRect.getCentreY(),
                                       true);
            g.setGradientFill (halo);
            g.fillEllipse (ledRect.getCentreX() - glowR, ledRect.getCentreY() - glowR, glowR * 2.0f, glowR * 2.0f);

            g.setColour (kPTGreenLED.withAlpha (alphaMul));
            g.fillRoundedRectangle (ledRect, 1.8f * scale);

            g.setColour (juce::Colour (0xffaee65c).withAlpha (0.85f * alphaMul));
            g.fillRoundedRectangle (ledRect.reduced (1.1f * scale), 1.2f * scale);
        }

        const float gapRadius = diameter * 0.295f;
        g.setColour (juce::Colour (0xff232527).withAlpha (alphaMul));
        g.fillEllipse (center.x - gapRadius, center.y - gapRadius, gapRadius * 2.0f, gapRadius * 2.0f);

        const float capRadius = diameter * 0.255f;
        const juce::Rectangle<float> capRect (center.x - capRadius,
                                              center.y - capRadius,
                                              capRadius * 2.0f,
                                              capRadius * 2.0f);

        juce::ColourGradient capRimGrad (juce::Colour (0xff8c8e91).withAlpha (alphaMul),
                                         capRect.getX(), capRect.getY(),
                                         juce::Colour (0xff4e5053).withAlpha (alphaMul),
                                         capRect.getX(), capRect.getBottom(),
                                         false);
        g.setGradientFill (capRimGrad);
        g.fillEllipse (capRect);

        const juce::Rectangle<float> faceRect = capRect.reduced (2.1f * scale);
        juce::ColourGradient faceGrad (juce::Colour (0xff47494c).withAlpha (alphaMul),
                                       faceRect.getX(), faceRect.getY(),
                                       juce::Colour (0xff343638).withAlpha (alphaMul),
                                       faceRect.getX(), faceRect.getBottom(),
                                       false);
        g.setGradientFill (faceGrad);
        g.fillEllipse (faceRect);

        const float ptrInner = 0.0f;
        const float ptrOuter = capRadius * 0.92f;
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);

        const juce::Point<float> p1 (center.x + sinP * ptrInner, center.y + cosP * ptrInner);
        const juce::Point<float> p2 (center.x + sinP * ptrOuter, center.y + cosP * ptrOuter);

        g.setColour (juce::Colour (0xff222426).withAlpha (0.85f * alphaMul));
        g.drawLine (p1.x + 0.7f * scale, p1.y, p2.x + 0.7f * scale, p2.y, 2.2f * scale);

        g.setColour (juce::Colour (0xff9fa2a6).withAlpha (alphaMul));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.75f * scale);
    }

    // =========================================================================
    // SKIN 0: FL Studio Dark Mixer Bipolar Rotary Knob
    // =========================================================================
    void drawFLStudioRotarySlider (juce::Graphics& g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider& slider)
    {
        juce::ignoreUnused (sliderPosProportional);

        const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                    static_cast<float> (y),
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 8.0f)
            return;

        const juce::Point<float> center = bounds.getCentre();
        const float scale = diameter / 74.0f;

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const double curVal = slider.getValue();
        const double normVal = (maxVal > minVal) ? ((curVal - minVal) / (maxVal - minVal)) : 0.5;
        const float currentAngle = rotaryStartAngle + static_cast<float> (normVal) * (rotaryEndAngle - rotaryStartAngle);
        const float centerAngle  = 0.5f * (rotaryStartAngle + rotaryEndAngle);

        const bool isEnabled = slider.isEnabled();
        const float alphaMul = isEnabled ? 1.0f : 0.4f;

        const float tickInnerRadius = diameter * 0.425f;
        const float tickOuterRadius = diameter * 0.468f;
        constexpr int kNumTicks = 27;

        for (int i = 0; i < kNumTicks; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (kNumTicks - 1);
            const float angle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const bool isCenterTick = (i == kNumTicks / 2);
            const bool isMajorTick  = (i == 0 || i == kNumTicks - 1 || isCenterTick);

            const float r1 = isCenterTick ? (tickInnerRadius - 1.2f * scale) : tickInnerRadius;
            const float r2 = isMajorTick  ? (tickOuterRadius + 1.0f * scale) : tickOuterRadius;

            const float sinA = std::sin (angle);
            const float cosA = -std::cos (angle);

            juce::Colour tickCol = isCenterTick ? kTextPrimary.withAlpha (0.85f * alphaMul)
                                                : kTextSecondary.withAlpha ((isMajorTick ? 0.65f : 0.35f) * alphaMul);

            g.setColour (tickCol);
            g.drawLine (center.x + sinA * r1,
                        center.y + cosA * r1,
                        center.x + sinA * r2,
                        center.y + cosA * r2,
                        (isCenterTick ? 1.5f : 1.0f) * scale);
        }

        const float arcRadius = diameter * 0.375f;
        const float arcThickness = std::max (1.5f, 2.7f * scale);

        juce::Path trackPath;
        trackPath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff121417).withAlpha (alphaMul));
        g.strokePath (trackPath, juce::PathStrokeType (arcThickness,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        if (std::abs (curVal) > 0.02)
        {
            const juce::Colour activeCol = (curVal > 0.0) ? kAccentOrange : kAccentCyan;
            const float startA = std::min (centerAngle, currentAngle);
            const float endA   = std::max (centerAngle, currentAngle);

            juce::Path glowPath;
            glowPath.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                    0.0f, startA, endA, true);
            g.setColour (activeCol.withAlpha (0.24f * alphaMul));
            g.strokePath (glowPath, juce::PathStrokeType (arcThickness * 2.2f,
                                                          juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

            juce::Path activeArc;
            activeArc.addCentredArc (center.x, center.y, arcRadius, arcRadius,
                                     0.0f, startA, endA, true);
            g.setColour (activeCol.withAlpha (alphaMul));
            g.strokePath (activeArc, juce::PathStrokeType (arcThickness,
                                                           juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
        }

        const float bodyRadius = diameter * 0.305f;
        const juce::Rectangle<float> bodyRect (center.x - bodyRadius,
                                               center.y - bodyRadius,
                                               bodyRadius * 2.0f,
                                               bodyRadius * 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.55f * alphaMul));
        g.fillEllipse (bodyRect.translated (0.0f, 1.8f * scale));

        g.setColour (juce::Colour (0xff14171a).withAlpha (alphaMul));
        g.fillEllipse (bodyRect);

        const juce::Rectangle<float> capRect = bodyRect.reduced (1.5f * scale);
        juce::ColourGradient capGrad (juce::Colour (0xff383e44).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getY(),
                                      juce::Colour (0xff202428).withAlpha (alphaMul),
                                      capRect.getX(), capRect.getBottom(),
                                      false);
        g.setGradientFill (capGrad);
        g.fillEllipse (capRect);

        g.setColour (juce::Colour (0xff4a525a).withAlpha (0.55f * alphaMul));
        g.drawEllipse (capRect.reduced (0.6f * scale), 0.9f * scale);

        const float ptrInner = bodyRadius * 0.28f;
        const float ptrOuter = bodyRadius * 0.84f;
        const float sinP = std::sin (currentAngle);
        const float cosP = -std::cos (currentAngle);

        const juce::Point<float> p1 (center.x + sinP * ptrInner, center.y + cosP * ptrInner);
        const juce::Point<float> p2 (center.x + sinP * ptrOuter, center.y + cosP * ptrOuter);

        g.setColour (juce::Colours::black.withAlpha (0.5f * alphaMul));
        g.drawLine (p1.x, p1.y + 0.7f * scale, p2.x, p2.y + 0.7f * scale, 2.2f * scale);

        g.setColour (juce::Colour (0xfff4f7fa).withAlpha (alphaMul));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.85f * scale);

        const float centerDotR = 2.0f * scale;
        g.setColour (juce::Colour (0xff181b1e).withAlpha (alphaMul));
        g.fillEllipse (center.x - centerDotR, center.y - centerDotR, centerDotR * 2.0f, centerDotR * 2.0f);
    }

    static void drawRecessedPanel (juce::Graphics& g, juce::Rectangle<float> rect, float scale)
    {
        const float corner = 4.0f * scale;
        g.setColour (kRecessedPanel);
        g.fillRoundedRectangle (rect, corner);

        g.setColour (kRecessedBorder);
        g.drawRoundedRectangle (rect, corner, std::max (1.0f, 1.0f * scale));

        g.setColour (kChassisBevel.withAlpha (0.35f));
        g.drawLine (rect.getX() + corner, rect.getBottom() + 0.5f * scale,
                    rect.getRight() - corner, rect.getBottom() + 0.5f * scale,
                    std::max (0.8f, 0.9f * scale));
    }

    static void drawHarmonicLED (juce::Graphics& g,
                                 juce::Point<float> center,
                                 float radius,
                                 float activity,
                                 float drivePolarity,
                                 bool isBypassed)
    {
        const float clampedAct = isBypassed ? 0.0f : juce::jlimit (0.0f, 1.0f, activity);
        const juce::Colour baseLedCol = (drivePolarity >= 0.0f) ? kAccentOrange : kAccentCyan;

        g.setColour (kRecessedBorder);
        g.fillEllipse (center.x - radius * 1.25f, center.y - radius * 1.25f, radius * 2.5f, radius * 2.5f);

        g.setColour (juce::Colour (0xff1a1d20));
        g.fillEllipse (center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

        if (clampedAct > 0.01f)
        {
            const float glowR = radius * (1.5f + 1.2f * clampedAct);
            juce::ColourGradient glowGrad (baseLedCol.withAlpha (0.55f * clampedAct),
                                           center.x, center.y,
                                           baseLedCol.withAlpha (0.0f),
                                           center.x + glowR, center.y,
                                           true);
            g.setGradientFill (glowGrad);
            g.fillEllipse (center.x - glowR, center.y - glowR, glowR * 2.0f, glowR * 2.0f);

            g.setColour (baseLedCol.interpolatedWith (juce::Colours::white, 0.25f * clampedAct)
                                   .withAlpha (0.35f + 0.65f * clampedAct));
            g.fillEllipse (center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
        }
        else if (! isBypassed && std::abs (drivePolarity) > 0.05f)
        {
            g.setColour (baseLedCol.withAlpha (0.28f));
            g.fillEllipse (center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (center.x - radius * 0.45f, center.y - radius * 0.55f, radius * 0.65f, radius * 0.5f);
    }

private:
    int skinStyle = kSkinFLStudio;
};

} // namespace flheat
