#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "FLStudioLookAndFeel.h"
#include <functional>

/**
 * Custom Rotary Knob supporting all 6 skins (FL Studio, Pro Tools 1:1, Atari ST / Retro PC,
 * Ableton Live 12, Apple Logic Pro X, Steinberg Cubase 14):
 * - Right-click opens context menu (Skin selector, Oversampling 1x/2x/4x/8x, Window zoom)
 * - Double-click resets to 0.0
 * - Mouse wheel fine increments
 * - Ctrl/Shift fine velocity dragging
 */
class FLHeatKnob : public juce::Slider
{
public:
    FLHeatKnob (const juce::String& paramName, bool isDriveKnob)
        : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
          name (paramName),
          driveKnob (isDriveKnob)
    {
        setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f,
                             true);
        setDoubleClickReturnValue (true, isDriveKnob ? 1.0 : 0.0);
        setVelocityBasedMode (false);
        setSliderSnapsToMousePosition (false);
        setMouseDragSensitivity (135);
        setScrollWheelEnabled (true);
    }

    std::function<void()> onRightClickMenu;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (onRightClickMenu)
                onRightClickMenu();
            return;
        }

        updateDragSensitivity (e.mods);
        juce::Slider::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
            return;

        updateDragSensitivity (e.mods);
        juce::Slider::mouseDrag (e);
    }

    juce::String getTooltip() override
    {
        const double v = getValue();
        juce::String valStr = (std::abs (v) < 0.005)
                                ? "0.0"
                                : ((v > 0.0 ? "+" : "") + juce::String (v, 2));

        if (driveKnob)
        {
            juce::String character = "Jaycen Joshua 8x Default (+1.0)";
            if (v > 1.05)
                character = "Mas saturacion armonica H2..H16 en las 8 consolas";
            else if (v >= 0.0)
                character = "Menos empuje en Slot 1 (hacia base 8x NLS)";
            else
                character = "Transformador Limpio (reduce THD en las 8 consolas) + Expansion de Transitorios (+Punch)";

            return name + " (Saturacion / Clean Punch 8x NLS): " + valStr
                 + " [" + character + "] — Doble clic: +1.0 | Clic derecho: Menu";
        }

        juce::String subInfo = "1:1 Jaycen Joshua (+6.2 dB @ 27 Hz, L=R 0.00 dB)";
        if (v > 0.05)
            subInfo = "Full Phat & Air: Mas Subgrave 27 Hz (hasta +9.3 dB) + Abre Aire 16 kHz";
        else if (v < -0.05)
            subInfo = "Tight Sub: Controla/recorta el bulto de 27 Hz hacia respuesta plana";

        return name + " (Sub 27 Hz + Air 16 kHz L=R): " + valStr
             + " [" + subInfo + "] — Doble clic: 0.0 | Clic derecho: Menu";
    }

private:
    void updateDragSensitivity (const juce::ModifierKeys& mods)
    {
        const bool fineMode = mods.isCtrlDown() || mods.isShiftDown() || mods.isCommandDown();
        setMouseDragSensitivity (fineMode ? 550 : 135);
    }

    juce::String name;
    bool driveKnob = false;
};

/**
 * 6-Skin Mode / Power Button:
 * - Skin 0 (FL Studio): "IND" / "LNK" pill button
 * - Skin 1 (Pro Tools 1:1): Square beveled Power Icon [⏻] button
 * - Skin 2 (Atari ST / Retro PC): GEM 3D beveled "[LNK]" / "[IND]" retro button
 * - Skin 3 (Ableton Live 12): Flat sharp rectangular "LNK" / "IND" button (Yellow active)
 * - Skin 4 (Apple Logic Pro X): Glowing metallic pill "LNK" / "IND" (Logic Blue active)
 * - Skin 5 (Steinberg Cubase 14): MixConsole rectangular LED slot "LNK" / "IND"
 */
class FLModePillButton : public juce::ToggleButton
{
public:
    FLModePillButton()
    {
        setClickingTogglesState (true);
        setTooltip ("AutoGain Estereo (LNK / Luz Verde = ON | IND = OFF): Iguala el volumen RMS de salida al de entrada tras las 8 consolas manteniendo L=R clavado a 0.00 dB (Clic derecho: Skins)");
    }

    std::function<void()> onRightClickMenu;
    int skinStyle = flheat::kSkinFLStudio;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (onRightClickMenu)
                onRightClickMenu();
            return;
        }
        juce::ToggleButton::mouseDown (e);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (shouldDrawButtonAsDown);

        switch (skinStyle)
        {
            case flheat::kSkinProTools:
                paintProToolsPowerButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinAtariPC:
                paintAtariModeButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinAbletonLive:
                paintAbletonModeButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinLogicProX:
                paintLogicModeButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinCubase:
                paintCubaseModeButton (g, shouldDrawButtonAsHighlighted);
                return;
            default:
                break;
        }

        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float h = b.getHeight();
        const float scale = h / 16.0f;
        const float corner = h * 0.45f;
        const bool isLinked = getToggleState();

        const juce::Colour bgCol = isLinked
                                    ? flheat::FLStudioLookAndFeel::kAccentOrange.withAlpha (0.22f)
                                    : flheat::FLStudioLookAndFeel::kRecessedPanel;
        const juce::Colour borderCol = isLinked
                                        ? flheat::FLStudioLookAndFeel::kAccentOrange
                                        : (shouldDrawButtonAsHighlighted
                                            ? flheat::FLStudioLookAndFeel::kTextSecondary
                                            : flheat::FLStudioLookAndFeel::kRecessedBorder);

        g.setColour (bgCol);
        g.fillRoundedRectangle (b, corner);

        g.setColour (borderCol);
        g.drawRoundedRectangle (b, corner, std::max (1.0f, 1.0f * scale));

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (std::max (5.0f, h * 0.58f), true));
        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kAccentOrange
                              : flheat::FLStudioLookAndFeel::kTextSecondary);
        g.drawFittedText (isLinked ? "LNK" : "IND", getLocalBounds(), juce::Justification::centred, 1);
    }

private:
    void paintAbletonModeButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds();
        const bool isLinked = getToggleState();

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kLiveYellow
                              : juce::Colour (0xff262626));
        g.fillRect (b);

        g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
        g.drawRect (b, 1);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.58f, true));
        g.setColour (isLinked ? juce::Colour (0xff161616) : flheat::FLStudioLookAndFeel::kLiveText);
        g.drawFittedText (isLinked ? "LNK" : "IND", b, juce::Justification::centred, 1);
    }

    void paintLogicModeButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const bool isLinked = getToggleState();
        const float corner = 3.0f;

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kLogicPowerBlue
                              : juce::Colour (0xff1a1c20));
        g.fillRoundedRectangle (b, corner);

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kLogicCyan
                              : flheat::FLStudioLookAndFeel::kLogicCardBorder);
        g.drawRoundedRectangle (b, corner, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.58f, true));
        g.setColour (isLinked ? juce::Colours::white : flheat::FLStudioLookAndFeel::kTextSecondary);
        g.drawFittedText (isLinked ? "LNK" : "IND", getLocalBounds(), juce::Justification::centred, 1);
    }

    void paintCubaseModeButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const bool isLinked = getToggleState();

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kCubaseBlue.withAlpha (0.28f)
                              : flheat::FLStudioLookAndFeel::kCubasePanel);
        g.fillRoundedRectangle (b, 2.0f);

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kCubaseBlue
                              : flheat::FLStudioLookAndFeel::kCubaseBorder);
        g.drawRoundedRectangle (b, 2.0f, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.58f, true));
        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kCubaseBlue
                              : flheat::FLStudioLookAndFeel::kTextSecondary);
        g.drawFittedText (isLinked ? "LNK" : "IND", getLocalBounds(), juce::Justification::centred, 1);
    }

    void paintAtariModeButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds();
        const bool isLinked = getToggleState();

        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kAtariBlack
                              : flheat::FLStudioLookAndFeel::kAtariBeige);
        g.fillRect (b);

        g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
        g.drawRect (b, 1);

        g.setFont (flheat::FLStudioLookAndFeel::getAtariFont (b.getHeight() * 0.62f, true));
        g.setColour (isLinked ? flheat::FLStudioLookAndFeel::kAtariGreenCRT
                              : flheat::FLStudioLookAndFeel::kAtariBlack);
        g.drawFittedText (isLinked ? "LNK" : "IND", b, juce::Justification::centred, 1);
    }

    void paintProToolsPowerButton (juce::Graphics& g, bool highlighted)
    {
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float scale = b.getHeight() / 22.0f;
        const float corner = 3.2f * scale;
        const bool isLinked = getToggleState();

        g.setColour (juce::Colour (0xff232527));
        g.fillRoundedRectangle (b, corner);

        g.setColour (juce::Colour (0xff4e5052));
        g.drawRoundedRectangle (b, corner, std::max (1.0f, 1.0f * scale));

        const auto inner = b.reduced (2.0f * scale);
        juce::ColourGradient grad (juce::Colour (highlighted ? 0xff626568 : 0xff57595c),
                                   inner.getX(), inner.getY(),
                                   juce::Colour (0xff3c3e40),
                                   inner.getX(), inner.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (inner, corner * 0.75f);

        g.setColour (juce::Colour (0xff252729));
        g.drawRoundedRectangle (inner, corner * 0.75f, std::max (0.8f, 0.9f * scale));

        const juce::Point<float> c = inner.getCentre();
        const float r = std::min (inner.getWidth(), inner.getHeight()) * 0.27f;
        const float strokeW = std::max (1.3f, 1.85f * scale);

        const juce::Colour iconCol = isLinked
                                      ? flheat::FLStudioLookAndFeel::kPTGreenLED
                                      : juce::Colour (0xffc4c6c8);

        juce::Path arcPath;
        arcPath.addCentredArc (c.x, c.y + 0.5f * scale, r, r,
                               0.0f,
                               juce::MathConstants<float>::pi * 0.24f,
                               juce::MathConstants<float>::pi * 1.76f,
                               true);
        g.setColour (iconCol);
        g.strokePath (arcPath, juce::PathStrokeType (strokeW,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        g.drawLine (c.x, c.y - r * 1.15f, c.x, c.y + 0.4f * scale, strokeW);
    }
};

/**
 * 6-Skin Bypass Switch Button:
 * - Skin 0 (FL Studio): Bottom mixer bar "ACTIVE" / "BYPASS" switch.
 * - Skin 1 (Pro Tools 1:1): Top-right "OMITIR" beveled button.
 * - Skin 2 (Atari ST / Retro PC): GEM 3D beveled CRT "* ONLINE *" / "- BYPASS -" switch.
 * - Skin 3 (Ableton Live 12): Flat yellow Ableton Device Activator switch.
 * - Skin 4 (Apple Logic Pro X): Glowing Logic blue power switch.
 * - Skin 5 (Steinberg Cubase 14): Cubase MixConsole Insert Activate/Bypass button.
 */
class FLBypassSwitchButton : public juce::ToggleButton
{
public:
    FLBypassSwitchButton()
    {
        setClickingTogglesState (true);
        setTooltip ("ACTIVE / OMITIR (Bypass): Activa o desactiva la cadena de 8 consolas Summing Console de Jaycen Joshua (Clic derecho: Skins)");
    }

    std::function<void()> onRightClickMenu;
    int skinStyle = flheat::kSkinFLStudio;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (onRightClickMenu)
                onRightClickMenu();
            return;
        }
        juce::ToggleButton::mouseDown (e);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (shouldDrawButtonAsDown);

        switch (skinStyle)
        {
            case flheat::kSkinProTools:
                paintProToolsOmitirButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinAtariPC:
                paintAtariBypassButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinAbletonLive:
                paintAbletonBypassButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinLogicProX:
                paintLogicBypassButton (g, shouldDrawButtonAsHighlighted);
                return;
            case flheat::kSkinCubase:
                paintCubaseBypassButton (g, shouldDrawButtonAsHighlighted);
                return;
            default:
                break;
        }

        const auto b = getLocalBounds().toFloat().reduced (0.8f);
        const float h = b.getHeight();
        const float scale = h / 22.0f;
        const float corner = 3.5f * scale;

        const bool isBypassed = getToggleState();
        const bool isActive   = ! isBypassed;

        juce::Colour fillCol = isActive
                                ? juce::Colour (0xff2b332c)
                                : flheat::FLStudioLookAndFeel::kButtonOff;
        if (shouldDrawButtonAsHighlighted)
            fillCol = fillCol.brighter (0.08f);

        g.setColour (fillCol);
        g.fillRoundedRectangle (b, corner);

        const juce::Colour borderCol = isActive
                                        ? flheat::FLStudioLookAndFeel::kAccentOrange.withAlpha (0.85f)
                                        : flheat::FLStudioLookAndFeel::kRecessedBorder;
        g.setColour (borderCol);
        g.drawRoundedRectangle (b, corner, std::max (1.0f, 1.1f * scale));

        const float ledW = std::max (3.0f, 3.8f * scale);
        const float ledH = b.getHeight() * 0.48f;
        const juce::Rectangle<float> ledRect (b.getX() + 6.0f * scale,
                                              b.getCentreY() - ledH * 0.5f,
                                              ledW,
                                              ledH);

        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kAccentOrange
                              : juce::Colour (0xff1c1f22));
        g.fillRoundedRectangle (ledRect, ledW * 0.4f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (std::max (5.0f, h * 0.48f), true));
        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kTextHeader
                              : flheat::FLStudioLookAndFeel::kTextSecondary);

        auto textRect = b.withLeft (ledRect.getRight() + 2.0f * scale).toNearestInt();
        g.drawFittedText (isActive ? "ACTIVE" : "BYPASS", textRect, juce::Justification::centred, 1);
    }

private:
    void paintAbletonBypassButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds();
        const bool isActive = ! getToggleState();

        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kLiveYellow : juce::Colour (0xff262626));
        g.fillRect (b);
        g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
        g.drawRect (b, 1);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.50f, true));
        g.setColour (isActive ? juce::Colour (0xff161616) : juce::Colour (0xff8c8c8c));
        g.drawFittedText (isActive ? "DEVICE ON" : "BYPASSED", b, juce::Justification::centred, 1);
    }

    void paintLogicBypassButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds().toFloat().reduced (0.8f);
        const bool isActive = ! getToggleState();

        juce::ColourGradient grad (isActive ? juce::Colour (0xff249aff) : juce::Colour (0xff282b30),
                                   b.getX(), b.getY(),
                                   isActive ? juce::Colour (0xff0c68c8) : juce::Colour (0xff1c1e22),
                                   b.getX(), b.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 4.0f);

        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kLogicCyan : flheat::FLStudioLookAndFeel::kLogicCardBorder);
        g.drawRoundedRectangle (b, 4.0f, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.48f, true));
        g.setColour (isActive ? juce::Colours::white : flheat::FLStudioLookAndFeel::kTextSecondary);
        g.drawFittedText (isActive ? "ACTIVE" : "BYPASS", getLocalBounds(), juce::Justification::centred, 1);
    }

    void paintCubaseBypassButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds().toFloat().reduced (0.8f);
        const bool isActive = ! getToggleState();

        g.setColour (flheat::FLStudioLookAndFeel::kCubasePanel);
        g.fillRoundedRectangle (b, 2.0f);

        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kCubaseOrange : flheat::FLStudioLookAndFeel::kCubaseBorder);
        g.drawRoundedRectangle (b, 2.0f, 1.0f);

        // Cubase top horizontal LED bar inside switch
        const juce::Rectangle<float> topLed (b.getX() + 4.0f, b.getY() + 2.5f, b.getWidth() - 8.0f, 2.5f);
        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kCubaseOrange : juce::Colour (0xff262b32));
        g.fillRect (topLed);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (b.getHeight() * 0.46f, true));
        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kTextHeader : flheat::FLStudioLookAndFeel::kTextSecondary);
        g.drawFittedText (isActive ? "ACTIVE" : "BYPASS",
                          getLocalBounds().withTrimmedTop (3),
                          juce::Justification::centred,
                          1);
    }

    void paintAtariBypassButton (juce::Graphics& g, bool highlighted)
    {
        juce::ignoreUnused (highlighted);
        const auto b = getLocalBounds();
        const bool isBypassed = getToggleState();
        const bool isActive = ! isBypassed;

        g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
        g.fillRect (b);

        g.setColour (isActive ? flheat::FLStudioLookAndFeel::kAtariGreenCRT
                              : flheat::FLStudioLookAndFeel::kAtariAmberCRT);
        g.drawRect (b.reduced (1), 1);

        g.setFont (flheat::FLStudioLookAndFeel::getAtariFont (b.getHeight() * 0.54f, true));
        g.drawFittedText (isActive ? "* ONLINE *" : "- BYPASS -", b, juce::Justification::centred, 1);
    }

    void paintProToolsOmitirButton (juce::Graphics& g, bool highlighted)
    {
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        const float scale = b.getHeight() / 22.0f;
        const float corner = 3.2f * scale;
        const bool isBypassed = getToggleState();

        g.setColour (juce::Colour (0xff232527));
        g.fillRoundedRectangle (b, corner);

        g.setColour (juce::Colour (0xff4e5052));
        g.drawRoundedRectangle (b, corner, std::max (1.0f, 1.0f * scale));

        const auto inner = b.reduced (2.0f * scale);
        if (isBypassed)
        {
            juce::ColourGradient litGrad (juce::Colour (0xfff0b23a),
                                          inner.getX(), inner.getY(),
                                          juce::Colour (0xffc88718),
                                          inner.getX(), inner.getBottom(),
                                          false);
            g.setGradientFill (litGrad);
        }
        else
        {
            juce::ColourGradient offGrad (juce::Colour (highlighted ? 0xff5c5f62 : 0xff525558),
                                          inner.getX(), inner.getY(),
                                          juce::Colour (0xff3a3c3e),
                                          inner.getX(), inner.getBottom(),
                                          false);
            g.setGradientFill (offGrad);
        }
        g.fillRoundedRectangle (inner, corner * 0.75f);

        g.setColour (juce::Colour (0xff242628));
        g.drawRoundedRectangle (inner, corner * 0.75f, std::max (0.8f, 0.9f * scale));

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (std::max (5.0f, inner.getHeight() * 0.62f), false));
        g.setColour (isBypassed ? flheat::FLStudioLookAndFeel::kPTOmitirTextOn
                                : flheat::FLStudioLookAndFeel::kPTOmitirTextOff);
        g.drawFittedText ("OMITIR", inner.toNearestInt(), juce::Justification::centred, 1);
    }
};

class FLHeatAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    // Logical internal design canvas coordinates remain 130x290 for crisp vector layout
    static constexpr int kDesignWidth  = 130;
    static constexpr int kDesignHeight = 290;

    // Compact default size safe for FL Studio Fruity Wrapper (130 x 240 px)
    static constexpr int kBaseWidth  = 130;
    static constexpr int kBaseHeight = 240;
    static constexpr int kMinWidth   = 118;
    static constexpr int kMinHeight  = 218;
    static constexpr int kMaxWidth   = 210;
    static constexpr int kMaxHeight  = 388;

    explicit FLHeatAudioProcessorEditor (FLHeatAudioProcessor&);
    ~FLHeatAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void syncSkinState();
    void showContextMenu();
    void paintFLStudioSkin (juce::Graphics& g);
    void paintProToolsSkin (juce::Graphics& g);
    void paintAtariPCSkin (juce::Graphics& g);
    void paintAbletonLiveSkin (juce::Graphics& g);
    void paintLogicProSkin (juce::Graphics& g);
    void paintCubaseSkin (juce::Graphics& g);
    static juce::String formatSignedValue (double value);
    static const char* getOversamplingShortTag (int osMode) noexcept;

    FLHeatAudioProcessor& audioProcessor;
    flheat::FLStudioLookAndFeel flLookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    juce::Component contentCanvas;

    FLHeatKnob driveKnob { "DRIVE", true };
    FLHeatKnob toneKnob  { "TONE",  false };
    FLModePillButton linkButton;
    FLBypassSwitchButton bypassButton;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<ButtonAttachment> linkAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    // Logical 130x290 layout rectangles
    juce::Rectangle<float> headerBounds;
    juce::Rectangle<float> drivePanelBounds;
    juce::Rectangle<float> tonePanelBounds;
    juce::Rectangle<float> footerBounds;
    juce::Rectangle<float> driveReadoutBounds;
    juce::Rectangle<float> toneReadoutBounds;

    // Pro Tools 1:1 Skin layout rectangles
    juce::Rectangle<float> ptOuterTabBounds;
    juce::Rectangle<float> ptDarkStripBounds;
    juce::Rectangle<float> ptTitleBounds;
    juce::Rectangle<float> ptMeterSlotBounds;
    juce::Rectangle<float> ptGreyCardBounds;
    juce::Rectangle<float> ptDriveLabelBounds;
    juce::Rectangle<float> ptToneLabelBounds;

    float displayedActivity = 0.0f;
    float displayedPolarity = 0.0f;
    int   currentSkinStyle  = flheat::kSkinFLStudio;
    int   displayedOSMode   = 2;
    int   displayedStages   = 8;

    float logicalScale = 1.0f;
    float logicalW     = 130.0f;
    float logicalH     = 290.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FLHeatAudioProcessorEditor)
};
