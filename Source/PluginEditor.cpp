#include "PluginProcessor.h"
#include "PluginEditor.h"

FLHeatAudioProcessorEditor::FLHeatAudioProcessorEditor (FLHeatAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&flLookAndFeel);

    contentCanvas.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (contentCanvas);

    contentCanvas.addAndMakeVisible (driveKnob);
    contentCanvas.addAndMakeVisible (toneKnob);
    contentCanvas.addAndMakeVisible (linkButton);
    contentCanvas.addAndMakeVisible (bypassButton);

    auto& apvts = audioProcessor.getAPVTS();
    driveAttachment  = std::make_unique<SliderAttachment> (apvts, FLHeatAudioProcessor::kParamDrive,  driveKnob);
    toneAttachment   = std::make_unique<SliderAttachment> (apvts, FLHeatAudioProcessor::kParamTone,   toneKnob);
    linkAttachment   = std::make_unique<ButtonAttachment> (apvts, FLHeatAudioProcessor::kParamLink,   linkButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (apvts, FLHeatAudioProcessor::kParamBypass, bypassButton);

    auto openContextMenuCallback = [this]()
    {
        showContextMenu();
    };

    driveKnob.onRightClickMenu    = openContextMenuCallback;
    toneKnob.onRightClickMenu     = openContextMenuCallback;
    linkButton.onRightClickMenu   = openContextMenuCallback;
    bypassButton.onRightClickMenu = openContextMenuCallback;

    driveKnob.onValueChange = [this]() { repaint(); };
    toneKnob.onValueChange  = [this]() { repaint(); };
    linkButton.onClick      = [this]() { repaint(); };
    bypassButton.onClick    = [this]()
    {
        const bool isActive = ! bypassButton.getToggleState();
        driveKnob.setEnabled (isActive);
        toneKnob.setEnabled (isActive);
        repaint();
    };

    const bool initialActive = ! bypassButton.getToggleState();
    driveKnob.setEnabled (initialActive);
    toneKnob.setEnabled (initialActive);

    currentSkinStyle = audioProcessor.getSkinIndex();
    displayedOSMode  = audioProcessor.getOversamplingMode();
    flLookAndFeel.setSkinStyle (currentSkinStyle);
    linkButton.skinStyle   = currentSkinStyle;
    bypassButton.skinStyle = currentSkinStyle;

    // Compact default window size safe for FL Studio Wrapper (130 x 240 px), restorable from state
    setResizable (false, false);
    const int initW = juce::jlimit (kMinWidth,  kMaxWidth,  audioProcessor.getSavedEditorWidth());
    const int initH = juce::jlimit (kMinHeight, kMaxHeight, audioProcessor.getSavedEditorHeight());
    setSize (initW, initH);

    startTimerHz (60);
}

FLHeatAudioProcessorEditor::~FLHeatAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

const char* FLHeatAudioProcessorEditor::getOversamplingShortTag (int osMode) noexcept
{
    switch (juce::jlimit (0, 3, osMode))
    {
        case 0:  return "1x";
        case 1:  return "2x";
        case 2:  return "4x";
        default: return "8x";
    }
}

void FLHeatAudioProcessorEditor::showContextMenu()
{
    juce::PopupMenu menu;

    const int curSkin = audioProcessor.getSkinIndex();
    const int curOS   = audioProcessor.getOversamplingMode();
    const bool isLnk  = audioProcessor.isLinkedMode();

    menu.addSectionHeader ("FL HEAT - SKIN / THEME");
    menu.addItem (1, "1. FL Studio (Dark Mixer UI)",        true, curSkin == flheat::kSkinFLStudio);
    menu.addItem (2, "2. Pro Tools HEAT 1:1 (Original)",    true, curSkin == flheat::kSkinProTools);
    menu.addItem (3, "3. Atari ST / Retro PC (GEM CRT)",    true, curSkin == flheat::kSkinAtariPC);
    menu.addItem (4, "4. Ableton Live 12 (Rack Device)",    true, curSkin == flheat::kSkinAbletonLive);
    menu.addItem (5, "5. Apple Logic Pro X (Space Grey)",   true, curSkin == flheat::kSkinLogicProX);
    menu.addItem (6, "6. Steinberg Cubase 14 (MixConsole)", true, curSkin == flheat::kSkinCubase);

    menu.addSeparator();
    menu.addSectionHeader ("OVERSAMPLING (ENGINE QUALITY)");
    menu.addItem (10, "1x Native (0 Latency - Pro Tools Bit-Exact)", true, curOS == 0);
    menu.addItem (11, "2x Oversampling (Low Latency IIR)",           true, curOS == 1);
    menu.addItem (12, "4x Oversampling (Default Studio HQ)",         true, curOS == 2);
    menu.addItem (13, "8x Oversampling (Ultra Mastering)",           true, curOS == 3);

    menu.addSeparator();
    menu.addSectionHeader ("CONSOLE LINK (FL STUDIO / ABLETON)");
    menu.addItem (20, "LNK: Master Linked Console (All Tracks)", true,  isLnk);
    menu.addItem (21, "IND: Individual Track Controls",          true, !isLnk);

    menu.addSeparator();
    menu.addSectionHeader ("UI SIZE (FL STUDIO SAFE)");
    const int curW = getWidth();
    menu.addItem (30, "Mini 70% (118 x 218 px)",            true, curW <= 122);
    menu.addItem (31, "Compact 80% (130 x 240 px Default)", true, curW > 122 && curW <= 140);
    menu.addItem (32, "Medium 90% (150 x 277 px)",          true, curW > 140 && curW <= 158);
    menu.addItem (33, "Classic 100% (168 x 310 px)",        true, curW > 158);

    juce::Component::SafePointer<FLHeatAudioProcessorEditor> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safeThis] (int result)
    {
        if (safeThis == nullptr || result == 0)
            return;

        auto& proc = safeThis->audioProcessor;

        if (result >= 1 && result <= 6)
        {
            proc.setSkinIndex (result - 1);
            safeThis->syncSkinState();
        }
        else if (result >= 10 && result <= 13)
        {
            proc.setOversamplingMode (result - 10);
            safeThis->displayedOSMode = proc.getOversamplingMode();
            safeThis->repaint();
        }
        else if (result == 20 || result == 21)
        {
            if (auto* p = proc.getAPVTS().getParameter (FLHeatAudioProcessor::kParamLink))
                p->setValueNotifyingHost (result == 20 ? 1.0f : 0.0f);
            safeThis->repaint();
        }
        else if (result >= 30 && result <= 33)
        {
            static constexpr int kWidths[4]  = { 118, 130, 150, 168 };
            static constexpr int kHeights[4] = { 218, 240, 277, 310 };
            const int idx = result - 30;
            proc.setSavedEditorSize (kWidths[idx], kHeights[idx]);
            safeThis->setSize (kWidths[idx], kHeights[idx]);
            safeThis->resized();
            safeThis->repaint();
        }
    });
}

void FLHeatAudioProcessorEditor::syncSkinState()
{
    const int wantSkin = audioProcessor.getSkinIndex();
    const int wantOS   = audioProcessor.getOversamplingMode();

    if (currentSkinStyle != wantSkin || displayedOSMode != wantOS)
    {
        currentSkinStyle = wantSkin;
        displayedOSMode  = wantOS;
        flLookAndFeel.setSkinStyle (currentSkinStyle);
        linkButton.skinStyle   = currentSkinStyle;
        bypassButton.skinStyle = currentSkinStyle;
        resized();
        contentCanvas.repaint();
        repaint();
    }
}

juce::String FLHeatAudioProcessorEditor::formatSignedValue (double value)
{
    if (std::abs (value) < 0.05)
        return "0.0";

    return (value > 0.0 ? "+" : "") + juce::String (value, 1);
}

void FLHeatAudioProcessorEditor::timerCallback()
{
    audioProcessor.pullSharedBusIfNeeded();
    syncSkinState();

    const bool isActive = ! bypassButton.getToggleState();
    if (driveKnob.isEnabled() != isActive)
    {
        driveKnob.setEnabled (isActive);
        toneKnob.setEnabled (isActive);
    }

    const float newAct = audioProcessor.getHarmonicActivity();
    const float newPol = audioProcessor.getHarmonicPolarity();

    if (std::abs (newAct - displayedActivity) > 0.004f
        || std::abs (newPol - displayedPolarity) > 0.01f)
    {
        displayedActivity = newAct;
        displayedPolarity = newPol;
        repaint();
    }
}

void FLHeatAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    juce::AudioProcessorEditor::mouseDown (event);
}

void FLHeatAudioProcessorEditor::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        return;

    const juce::Point<float> logicalPos (event.position.x / std::max (0.1f, logicalScale),
                                         event.position.y / std::max (0.1f, logicalScale));

    if (currentSkinStyle == flheat::kSkinProTools)
    {
        if (ptDriveLabelBounds.contains (logicalPos))
            driveKnob.setValue (1.0, juce::sendNotificationSync);
        else if (ptToneLabelBounds.contains (logicalPos))
            toneKnob.setValue (0.0, juce::sendNotificationSync);
    }
    else
    {
        if (driveReadoutBounds.contains (logicalPos))
            driveKnob.setValue (1.0, juce::sendNotificationSync);
        else if (toneReadoutBounds.contains (logicalPos))
            toneKnob.setValue (0.0, juce::sendNotificationSync);
    }
}

void FLHeatAudioProcessorEditor::resized()
{
    if (getWidth() > 0 && getHeight() > 0)
        audioProcessor.setSavedEditorSize (getWidth(), getHeight());

    const float scaleX = static_cast<float> (getWidth())  / static_cast<float> (kDesignWidth);
    const float scaleY = static_cast<float> (getHeight()) / static_cast<float> (kDesignHeight);
    logicalScale = std::max (0.25f, std::min (scaleX, scaleY));
    logicalW     = static_cast<float> (getWidth())  / logicalScale;
    logicalH     = static_cast<float> (getHeight()) / logicalScale;

    contentCanvas.setBounds (0, 0,
                             static_cast<int> (std::ceil (logicalW)),
                             static_cast<int> (std::ceil (logicalH)));
    contentCanvas.setTransform (juce::AffineTransform::scale (logicalScale, logicalScale));

    const float w = logicalW;
    const float h = logicalH;

    if (currentSkinStyle == flheat::kSkinProTools)
    {
        // =====================================================================
        // SKIN 1: Pro Tools HEAT 1:1 Original Layout (130x290 logical space)
        // =====================================================================
        const float outerMarginX = 4.5f;
        const float topCapH      = 16.0f;

        ptOuterTabBounds  = juce::Rectangle<float> (outerMarginX, 2.0f,
                                                    w - outerMarginX * 2.0f, h - 4.0f);
        ptDarkStripBounds = juce::Rectangle<float> (outerMarginX + 1.0f,
                                                    topCapH,
                                                    w - (outerMarginX + 1.0f) * 2.0f,
                                                    h - topCapH - 3.0f);

        const float innerX = ptDarkStripBounds.getX() + 5.0f;
        const float innerW = ptDarkStripBounds.getWidth() - 10.0f;

        ptTitleBounds = juce::Rectangle<float> (innerX,
                                                ptDarkStripBounds.getY() + 3.0f,
                                                innerW,
                                                18.0f);

        const float btnRowY = ptTitleBounds.getBottom() + 2.0f;
        const float btnRowH = 25.0f;
        const float powerW  = 26.0f;
        const float gapW    = 3.5f;
        const float omitirW = innerW - powerW - gapW;

        linkButton.setBounds (juce::Rectangle<float> (innerX, btnRowY, powerW, btnRowH).toNearestInt());
        bypassButton.setBounds (juce::Rectangle<float> (innerX + powerW + gapW, btnRowY, omitirW, btnRowH).toNearestInt());

        const float slotY = btnRowY + btnRowH + 5.5f;
        const float slotH = 12.0f;
        ptMeterSlotBounds = juce::Rectangle<float> (innerX + 1.0f, slotY, innerW - 2.0f, slotH);

        const float cardY = ptMeterSlotBounds.getBottom() + 6.0f;
        const float cardH = ptDarkStripBounds.getBottom() - cardY - 5.0f;
        ptGreyCardBounds = juce::Rectangle<float> (innerX, cardY, innerW, cardH);

        const float halfCardH = ptGreyCardBounds.getHeight() * 0.5f;
        const float labelH    = 17.5f;
        const float knobBoxH  = halfCardH - labelH - 2.0f;
        const float knobDiam  = std::min (ptGreyCardBounds.getWidth() * 0.76f, knobBoxH * 0.96f);

        const float k1Y = ptGreyCardBounds.getY() + 4.0f + (knobBoxH - knobDiam) * 0.5f;
        driveKnob.setBounds (juce::Rectangle<float> (ptGreyCardBounds.getCentreX() - knobDiam * 0.5f,
                                                     k1Y,
                                                     knobDiam,
                                                     knobDiam).toNearestInt());
        ptDriveLabelBounds = juce::Rectangle<float> (ptGreyCardBounds.getX() + 2.0f,
                                                     ptGreyCardBounds.getY() + halfCardH - labelH - 1.0f,
                                                     ptGreyCardBounds.getWidth() - 4.0f,
                                                     labelH);

        const float k2Top = ptGreyCardBounds.getY() + halfCardH;
        const float k2Y   = k2Top + 2.0f + (knobBoxH - knobDiam) * 0.5f;
        toneKnob.setBounds (juce::Rectangle<float> (ptGreyCardBounds.getCentreX() - knobDiam * 0.5f,
                                                    k2Y,
                                                    knobDiam,
                                                    knobDiam).toNearestInt());
        ptToneLabelBounds = juce::Rectangle<float> (ptGreyCardBounds.getX() + 2.0f,
                                                    ptGreyCardBounds.getBottom() - labelH - 3.0f,
                                                    ptGreyCardBounds.getWidth() - 4.0f,
                                                    labelH);
        return;
    }

    // =========================================================================
    // Vertical Strip Layout for FL Studio, Atari PC, Ableton, Logic Pro & Cubase
    // =========================================================================
    const float padX = 7.0f;

    const float headerH  = h * 0.14f;
    const float knobSecH = h * 0.35f;
    const float footerH  = h - (headerH + knobSecH * 2.0f);

    headerBounds     = juce::Rectangle<float> (0.0f, 0.0f, w, headerH);
    drivePanelBounds = juce::Rectangle<float> (padX, headerH, w - padX * 2.0f, knobSecH - 3.5f);
    tonePanelBounds  = juce::Rectangle<float> (padX, headerH + knobSecH, w - padX * 2.0f, knobSecH - 3.5f);
    footerBounds     = juce::Rectangle<float> (0.0f, headerH + knobSecH * 2.0f, w, footerH);

    const float pillW = 32.0f;
    const float pillH = 16.0f;
    linkButton.setBounds (juce::Rectangle<float> (w - padX - pillW,
                                                  headerBounds.getCentreY() - pillH * 0.5f,
                                                  pillW,
                                                  pillH).toNearestInt());

    auto layoutKnobSection = [] (const juce::Rectangle<float>& panel,
                                 FLHeatKnob& knob,
                                 juce::Rectangle<float>& readoutBox)
    {
        const float labelH   = panel.getHeight() * 0.19f;
        const float readoutH = panel.getHeight() * 0.20f;
        const float knobAreaH = panel.getHeight() - labelH - readoutH;
        const float knobDiameter = std::min (panel.getWidth() * 0.82f, knobAreaH * 0.98f);

        const juce::Rectangle<float> knobRect (panel.getCentreX() - knobDiameter * 0.5f,
                                               panel.getY() + labelH + (knobAreaH - knobDiameter) * 0.5f,
                                               knobDiameter,
                                               knobDiameter);
        knob.setBounds (knobRect.toNearestInt());

        readoutBox = juce::Rectangle<float> (panel.getX() + 6.0f,
                                             panel.getBottom() - readoutH - 2.0f,
                                             panel.getWidth() - 12.0f,
                                             readoutH);
    };

    layoutKnobSection (drivePanelBounds, driveKnob, driveReadoutBounds);
    layoutKnobSection (tonePanelBounds,  toneKnob,  toneReadoutBounds);

    const float btnH = std::min (footerBounds.getHeight() * 0.56f, 24.0f);
    const float btnW = w - padX * 2.0f;
    bypassButton.setBounds (juce::Rectangle<float> (padX,
                                                    footerBounds.getCentreY() - btnH * 0.54f,
                                                    btnW,
                                                    btnH).toNearestInt());
}

void FLHeatAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (logicalScale, logicalScale));

    switch (currentSkinStyle)
    {
        case flheat::kSkinProTools:
            paintProToolsSkin (g);
            break;
        case flheat::kSkinAtariPC:
            paintAtariPCSkin (g);
            break;
        case flheat::kSkinAbletonLive:
            paintAbletonLiveSkin (g);
            break;
        case flheat::kSkinLogicProX:
            paintLogicProSkin (g);
            break;
        case flheat::kSkinCubase:
            paintCubaseSkin (g);
            break;
        default:
            paintFLStudioSkin (g);
            break;
    }
}

void FLHeatAudioProcessorEditor::paintAbletonLiveSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;
    const float padX = 7.0f;

    // 1. Ableton Live 12 Flat Device Rack Background (#383838)
    g.fillAll (flheat::FLStudioLookAndFeel::kLiveChassisBg);

    // 2. Top Ableton Device Title Bar (#464646) with Yellow/Cyan status circle
    const auto hdrInt = headerBounds.withTrimmedBottom (4.0f).toNearestInt();
    g.setColour (flheat::FLStudioLookAndFeel::kLiveHeaderBg);
    g.fillRect (hdrInt);
    g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
    g.drawHorizontalLine (hdrInt.getBottom(), 0.0f, w);

    const bool isBypassed = bypassButton.getToggleState();
    const float act = isBypassed ? 0.0f : juce::jlimit (0.0f, 1.0f, displayedActivity);
    const juce::Colour circleCol = isBypassed
                                    ? juce::Colour (0xff222222)
                                    : (driveKnob.getValue() < -0.05
                                        ? flheat::FLStudioLookAndFeel::kLiveCyan
                                        : flheat::FLStudioLookAndFeel::kLiveYellow);

    g.setColour (circleCol.withAlpha (0.45f + 0.55f * act));
    g.fillEllipse (padX, headerBounds.getCentreY() - 5.5f, 9.0f, 9.0f);
    g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
    g.drawEllipse (padX, headerBounds.getCentreY() - 5.5f, 9.0f, 9.0f, 1.0f);

    g.setFont (flheat::FLStudioLookAndFeel::getFLFont (11.5f, true));
    g.setColour (flheat::FLStudioLookAndFeel::kLiveText);
    const juce::String titleStr = juce::String ("HEAT ") + getOversamplingShortTag (displayedOSMode);
    g.drawText (titleStr,
                juce::Rectangle<float> (padX + 13.0f, 0.0f, 68.0f, headerBounds.getHeight() - 4.0f),
                juce::Justification::centredLeft,
                false);

    // 3. Flat Rectangular Ableton Macro Sections
    auto drawLiveCard = [&g] (const juce::Rectangle<float>& panel,
                              const juce::String& title,
                              const juce::Rectangle<float>& readoutBox,
                              double currentVal,
                              bool isEnabled)
    {
        const auto pInt = panel.toNearestInt();
        g.setColour (flheat::FLStudioLookAndFeel::kLivePanelBg);
        g.fillRect (pInt);
        g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
        g.drawRect (pInt, 1);

        const float alpha = isEnabled ? 1.0f : 0.45f;
        const float labelH = panel.getHeight() * 0.19f;

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kLiveText.withAlpha (alpha));
        g.drawText (title,
                    juce::Rectangle<float> (panel.getX(), panel.getY() + 1.5f, panel.getWidth(), labelH),
                    juce::Justification::centred,
                    false);

        const auto pill = readoutBox.reduced (14.0f, 1.0f).toNearestInt();
        g.setColour (juce::Colour (0xff1c1c1c));
        g.fillRect (pill);
        g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
        g.drawRect (pill, 1);

        const juce::Colour valCol = (currentVal > 0.05)
                                      ? flheat::FLStudioLookAndFeel::kLiveYellow
                                      : ((currentVal < -0.05)
                                          ? flheat::FLStudioLookAndFeel::kLiveCyan
                                          : flheat::FLStudioLookAndFeel::kLiveText);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.5f, true));
        g.setColour (valCol.withAlpha (alpha));
        g.drawText (formatSignedValue (currentVal), pill, juce::Justification::centred, false);
    };

    const bool controlsActive = ! isBypassed;
    drawLiveCard (drivePanelBounds, "Drive", driveReadoutBounds, driveKnob.getValue(), controlsActive);
    drawLiveCard (tonePanelBounds,  "Tone",  toneReadoutBounds,  toneKnob.getValue(),  controlsActive);

    // Outer 1px sharp Ableton border
    g.setColour (flheat::FLStudioLookAndFeel::kLiveBorder);
    g.drawRect (0.0f, 0.0f, w, h, 1.0f);
}

void FLHeatAudioProcessorEditor::paintLogicProSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;
    const float padX = 7.0f;

    // 1. Apple Logic Pro X Space-Grey Anodized Aluminum Gradient Chassis
    juce::ColourGradient bgGrad (flheat::FLStudioLookAndFeel::kLogicTopBg, 0.0f, 0.0f,
                                 flheat::FLStudioLookAndFeel::kLogicBotBg, 0.0f, h, false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    // 2. Top Header with Glowing Cyan/Orange LED & OS Badge
    {
        g.setColour (juce::Colour (0xff181a1e));
        g.drawLine (padX, headerBounds.getBottom() - 2.0f, w - padX, headerBounds.getBottom() - 2.0f, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (12.5f, true));
        g.setColour (juce::Colours::white);
        g.drawText ("HEAT",
                    juce::Rectangle<float> (padX + 1.0f, headerBounds.getY(), 38.0f, headerBounds.getHeight()),
                    juce::Justification::centredLeft,
                    false);

        const juce::Point<float> ledCenter (padX + 43.0f, headerBounds.getCentreY());
        const bool isBypassed = bypassButton.getToggleState();
        const float polarity  = (std::abs (displayedPolarity) > 0.01f)
                                  ? displayedPolarity
                                  : static_cast<float> (driveKnob.getValue());

        flheat::FLStudioLookAndFeel::drawHarmonicLED (g, ledCenter, 3.2f, displayedActivity, polarity, isBypassed);

        const juce::Rectangle<float> osBadge (ledCenter.x + 6.5f, headerBounds.getCentreY() - 6.5f, 20.0f, 13.0f);
        g.setColour (flheat::FLStudioLookAndFeel::kLogicCardBg);
        g.fillRoundedRectangle (osBadge, 3.0f);
        g.setColour (flheat::FLStudioLookAndFeel::kLogicCardBorder);
        g.drawRoundedRectangle (osBadge, 3.0f, 0.9f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (8.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kLogicCyan);
        g.drawText (getOversamplingShortTag (displayedOSMode), osBadge, juce::Justification::centred, false);
    }

    // 3. Dark Glass Cards for DRIVE & TONE
    auto drawLogicCard = [&g] (const juce::Rectangle<float>& panel,
                               const juce::String& title,
                               const juce::Rectangle<float>& readoutBox,
                               double currentVal,
                               bool isEnabled)
    {
        g.setColour (flheat::FLStudioLookAndFeel::kLogicCardBg);
        g.fillRoundedRectangle (panel, 6.0f);
        g.setColour (flheat::FLStudioLookAndFeel::kLogicCardBorder);
        g.drawRoundedRectangle (panel, 6.0f, 1.0f);

        const float alpha = isEnabled ? 1.0f : 0.45f;
        const float labelH = panel.getHeight() * 0.19f;

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (9.8f, true));
        g.setColour (juce::Colour (0xffb0b8c4).withAlpha (alpha));
        g.drawText (title,
                    juce::Rectangle<float> (panel.getX(), panel.getY() + 1.5f, panel.getWidth(), labelH),
                    juce::Justification::centred,
                    false);

        const juce::Rectangle<float> pill = readoutBox.reduced (13.0f, 1.0f);
        g.setColour (juce::Colour (0xff101215).withAlpha (alpha));
        g.fillRoundedRectangle (pill, 3.5f);

        const juce::Colour valCol = (currentVal > 0.05)
                                      ? flheat::FLStudioLookAndFeel::kLogicOrange
                                      : ((currentVal < -0.05)
                                          ? flheat::FLStudioLookAndFeel::kLogicCyan
                                          : juce::Colours::white);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.5f, true));
        g.setColour (valCol.withAlpha (alpha));
        g.drawText (formatSignedValue (currentVal), pill, juce::Justification::centred, false);
    };

    const bool controlsActive = ! bypassButton.getToggleState();
    drawLogicCard (drivePanelBounds, "DRIVE", driveReadoutBounds, driveKnob.getValue(), controlsActive);
    drawLogicCard (tonePanelBounds,  "TONE",  toneReadoutBounds,  toneKnob.getValue(),  controlsActive);

    g.setColour (juce::Colour (0xff141619));
    g.drawRect (0.0f, 0.0f, w, h, 1.0f);
}

void FLHeatAudioProcessorEditor::paintCubaseSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;
    const float padX = 7.0f;

    // 1. Steinberg Cubase 14 MixConsole Deep Slate Chassis (#1E2228)
    g.fillAll (flheat::FLStudioLookAndFeel::kCubaseChassis);

    // Top orange/blue Cubase strip accent line
    g.setColour (flheat::FLStudioLookAndFeel::kCubaseOrange);
    g.fillRect (0.0f, 0.0f, w, 2.0f);

    // 2. Header Bar
    {
        g.setColour (flheat::FLStudioLookAndFeel::kCubaseBorder);
        g.drawLine (padX, headerBounds.getBottom() - 2.0f, w - padX, headerBounds.getBottom() - 2.0f, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (12.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kTextHeader);
        const juce::String hdrStr = juce::String ("HEAT ") + getOversamplingShortTag (displayedOSMode);
        g.drawText (hdrStr,
                    juce::Rectangle<float> (padX + 1.0f, headerBounds.getY(), 66.0f, headerBounds.getHeight()),
                    juce::Justification::centredLeft,
                    false);
    }

    // 3. Cubase MixConsole Rack Bays
    auto drawCubaseBay = [&g] (const juce::Rectangle<float>& panel,
                               const juce::String& title,
                               const juce::Rectangle<float>& readoutBox,
                               double currentVal,
                               bool isEnabled)
    {
        g.setColour (flheat::FLStudioLookAndFeel::kCubasePanel);
        g.fillRoundedRectangle (panel, 2.0f);
        g.setColour (flheat::FLStudioLookAndFeel::kCubaseBorder);
        g.drawRoundedRectangle (panel, 2.0f, 1.0f);

        // Cubase section header bar with left neon accent
        const juce::Rectangle<float> bayHdr (panel.getX() + 1.0f, panel.getY() + 1.0f, panel.getWidth() - 2.0f, 15.0f);
        g.setColour (juce::Colour (0xff1b2026));
        g.fillRect (bayHdr);
        g.setColour (flheat::FLStudioLookAndFeel::kCubaseOrange);
        g.fillRect (bayHdr.getX() + 2.0f, bayHdr.getY() + 3.0f, 2.5f, 9.0f);

        const float alpha = isEnabled ? 1.0f : 0.45f;
        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (9.5f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kTextHeader.withAlpha (alpha));
        g.drawText (title, bayHdr, juce::Justification::centred, false);

        const juce::Rectangle<float> pill = readoutBox.reduced (13.0f, 1.0f);
        g.setColour (juce::Colour (0xff0d0f12).withAlpha (alpha));
        g.fillRoundedRectangle (pill, 2.0f);
        g.setColour (flheat::FLStudioLookAndFeel::kCubaseBorder.withAlpha (alpha));
        g.drawRoundedRectangle (pill, 2.0f, 0.9f);

        const juce::Colour valCol = (currentVal > 0.05)
                                      ? flheat::FLStudioLookAndFeel::kCubaseOrange
                                      : ((currentVal < -0.05)
                                          ? flheat::FLStudioLookAndFeel::kCubaseBlue
                                          : flheat::FLStudioLookAndFeel::kTextPrimary);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.2f, true));
        g.setColour (valCol.withAlpha (alpha));
        g.drawText (formatSignedValue (currentVal), pill, juce::Justification::centred, false);
    };

    const bool controlsActive = ! bypassButton.getToggleState();
    drawCubaseBay (drivePanelBounds, "DRIVE", driveReadoutBounds, driveKnob.getValue(), controlsActive);
    drawCubaseBay (tonePanelBounds,  "TONE",  toneReadoutBounds,  toneKnob.getValue(),  controlsActive);

    g.setColour (juce::Colour (0xff0e1013));
    g.drawRect (0.0f, 0.0f, w, h, 1.0f);
    juce::ignoreUnused (h);
}

void FLHeatAudioProcessorEditor::paintAtariPCSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;
    const float padX = 7.0f;

    g.fillAll (flheat::FLStudioLookAndFeel::kAtariBeige);

    g.setColour (flheat::FLStudioLookAndFeel::kAtariLightBevel);
    g.drawHorizontalLine (1, 1.0f, w - 2.0f);
    g.drawVerticalLine (1, 1.0f, h - 2.0f);

    g.setColour (flheat::FLStudioLookAndFeel::kAtariDarkShadow);
    g.drawHorizontalLine (static_cast<int> (h) - 2, 1.0f, w - 1.0f);
    g.drawVerticalLine (static_cast<int> (w) - 2, 1.0f, h - 1.0f);

    {
        const juce::Rectangle<int> titleBar (4, 4, static_cast<int> (w) - 8, static_cast<int> (headerBounds.getHeight()) - 6);
        g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
        g.fillRect (titleBar);

        g.setColour (flheat::FLStudioLookAndFeel::kAtariGreenCRT.withAlpha (0.28f));
        for (int ly = titleBar.getY() + 4; ly < titleBar.getBottom() - 3; ly += 3)
            g.drawHorizontalLine (ly, static_cast<float> (titleBar.getX() + 3), static_cast<float> (titleBar.getRight() - 38));

        g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
        g.fillRect (8, titleBar.getY() + 2, 68, titleBar.getHeight() - 4);

        g.setFont (flheat::FLStudioLookAndFeel::getAtariFont (10.5f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kAtariGreenCRT);
        const juce::String hdrTitle = juce::String ("HEAT ") + getOversamplingShortTag (displayedOSMode);
        g.drawText (hdrTitle, 10, titleBar.getY(), 66, titleBar.getHeight(), juce::Justification::centredLeft, false);
    }

    auto drawAtariCard = [&g] (const juce::Rectangle<float>& panel,
                               const juce::String& title,
                               const juce::Rectangle<float>& readoutBox,
                               double currentVal,
                               bool isEnabled)
    {
        const auto pInt = panel.toNearestInt();

        g.setColour (flheat::FLStudioLookAndFeel::kAtariDarkShadow);
        g.drawHorizontalLine (pInt.getY(), static_cast<float> (pInt.getX()), static_cast<float> (pInt.getRight()));
        g.drawVerticalLine (pInt.getX(), static_cast<float> (pInt.getY()), static_cast<float> (pInt.getBottom()));

        g.setColour (flheat::FLStudioLookAndFeel::kAtariLightBevel);
        g.drawHorizontalLine (pInt.getBottom(), static_cast<float> (pInt.getX()), static_cast<float> (pInt.getRight()));
        g.drawVerticalLine (pInt.getRight(), static_cast<float> (pInt.getY()), static_cast<float> (pInt.getBottom()));

        const auto crtRect = pInt.reduced (2);
        g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
        g.fillRect (crtRect);

        g.setColour (flheat::FLStudioLookAndFeel::kAtariGreenCRT.withAlpha (0.06f));
        for (int sy = crtRect.getY() + 1; sy < crtRect.getBottom(); sy += 2)
            g.drawHorizontalLine (sy, static_cast<float> (crtRect.getX() + 1), static_cast<float> (crtRect.getRight() - 1));

        const float alpha = isEnabled ? 1.0f : 0.45f;
        const float labelH = panel.getHeight() * 0.19f;

        g.setFont (flheat::FLStudioLookAndFeel::getAtariFont (10.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kAtariGreenCRT.withAlpha (alpha));
        g.drawText ("[" + title + "]",
                    juce::Rectangle<float> (panel.getX(), panel.getY() + 1.5f, panel.getWidth(), labelH),
                    juce::Justification::centred,
                    false);

        const juce::Rectangle<float> pill = readoutBox.reduced (12.0f, 1.0f);
        g.setColour (juce::Colour (0xff070a07));
        g.fillRect (pill);

        const juce::Colour valCol = (currentVal > 0.05)
                                      ? flheat::FLStudioLookAndFeel::kAtariAmberCRT
                                      : flheat::FLStudioLookAndFeel::kAtariGreenCRT;
        g.setColour (valCol.withAlpha (0.65f * alpha));
        g.drawRect (pill, 1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getAtariFont (10.5f, true));
        g.setColour (valCol.withAlpha (alpha));
        g.drawText (formatSignedValue (currentVal), pill, juce::Justification::centred, false);
    };

    const bool controlsActive = ! bypassButton.getToggleState();
    drawAtariCard (drivePanelBounds, "DRIVE", driveReadoutBounds, driveKnob.getValue(), controlsActive);
    drawAtariCard (tonePanelBounds,  "TONE",  toneReadoutBounds,  toneKnob.getValue(),  controlsActive);

    g.setColour (flheat::FLStudioLookAndFeel::kAtariBlack);
    g.drawRect (0.0f, 0.0f, w, h, 1.0f);

    juce::ignoreUnused (padX);
}

void FLHeatAudioProcessorEditor::paintProToolsSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;

    g.fillAll (juce::Colour (0xff232527));

    const float outerCorner = 6.5f;
    g.setColour (flheat::FLStudioLookAndFeel::kPTOuterFrame);
    g.fillRoundedRectangle (ptOuterTabBounds, outerCorner);

    g.setColour (juce::Colour (0xff18191b));
    g.drawRoundedRectangle (ptOuterTabBounds, outerCorner, 1.0f);

    const float bodyCorner = 5.0f;
    g.setColour (flheat::FLStudioLookAndFeel::kPTDarkBody);
    g.fillRoundedRectangle (ptDarkStripBounds, bodyCorner);

    g.setColour (flheat::FLStudioLookAndFeel::kPTBodyBorder);
    g.drawRoundedRectangle (ptDarkStripBounds, bodyCorner, 1.1f);

    g.setFont (flheat::FLStudioLookAndFeel::getFLFont (13.0f, false));
    g.setColour (flheat::FLStudioLookAndFeel::kPTTitleText);
    g.drawText ("HEAT", ptTitleBounds, juce::Justification::centred, false);

    g.setFont (flheat::FLStudioLookAndFeel::getFLFont (7.8f, true));
    g.setColour (flheat::FLStudioLookAndFeel::kPTTitleText.withAlpha (0.55f));
    g.drawText (getOversamplingShortTag (displayedOSMode),
                ptTitleBounds.withTrimmedRight (2.0f),
                juce::Justification::centredRight,
                false);

    {
        const float slotCorner = 2.8f;
        g.setColour (juce::Colour (0xff2b2d2f));
        g.fillRoundedRectangle (ptMeterSlotBounds, slotCorner);

        g.setColour (juce::Colour (0xff202224));
        g.drawRoundedRectangle (ptMeterSlotBounds, slotCorner, 1.0f);

        const bool isBypassed = bypassButton.getToggleState();
        const float act = isBypassed ? 0.0f : juce::jlimit (0.0f, 1.0f, displayedActivity);
        if (act > 0.01f)
        {
            const auto innerBar = ptMeterSlotBounds.reduced (2.0f);
            const float fillW = innerBar.getWidth() * act;
            const float pol = (std::abs (displayedPolarity) > 0.01f)
                                ? displayedPolarity
                                : static_cast<float> (driveKnob.getValue());
            const juce::Colour barCol = (pol >= 0.0f)
                                          ? flheat::FLStudioLookAndFeel::kPTAmberBypass
                                          : flheat::FLStudioLookAndFeel::kPTGreenLED;
            g.setColour (barCol.withAlpha (0.65f));
            g.fillRoundedRectangle (innerBar.withWidth (fillW), slotCorner * 0.6f);
        }
    }

    {
        const float cardCorner = 4.0f;
        g.setColour (flheat::FLStudioLookAndFeel::kPTInnerCard);
        g.fillRoundedRectangle (ptGreyCardBounds, cardCorner);

        g.setColour (flheat::FLStudioLookAndFeel::kPTInnerHi.withAlpha (0.55f));
        g.drawLine (ptGreyCardBounds.getX() + cardCorner,
                    ptGreyCardBounds.getY() + 0.8f,
                    ptGreyCardBounds.getRight() - cardCorner,
                    ptGreyCardBounds.getY() + 0.8f,
                    1.0f);

        g.setColour (flheat::FLStudioLookAndFeel::kPTInnerBorder);
        g.drawRoundedRectangle (ptGreyCardBounds, cardCorner, 1.0f);
    }

    const bool controlsActive = ! bypassButton.getToggleState();
    const float textAlpha = controlsActive ? 1.0f : 0.55f;

    g.setFont (flheat::FLStudioLookAndFeel::getFLFont (13.2f, false));
    g.setColour (flheat::FLStudioLookAndFeel::kPTLabelText.withAlpha (textAlpha));

    const auto satText = juce::String (juce::CharPointer_UTF8 ("Saturaci\xc3\xb3n"));
    g.setColour (juce::Colours::black.withAlpha (0.35f * textAlpha));
    g.drawText (satText, ptDriveLabelBounds.translated (0.0f, 0.8f), juce::Justification::centred, false);
    g.drawText ("Tono",  ptToneLabelBounds.translated (0.0f, 0.8f),  juce::Justification::centred, false);

    g.setColour (flheat::FLStudioLookAndFeel::kPTLabelText.withAlpha (textAlpha));
    g.drawText (satText, ptDriveLabelBounds, juce::Justification::centred, false);
    g.drawText ("Tono",  ptToneLabelBounds,  juce::Justification::centred, false);
    juce::ignoreUnused (w, h);
}

void FLHeatAudioProcessorEditor::paintFLStudioSkin (juce::Graphics& g)
{
    const float w = logicalW;
    const float h = logicalH;
    const float padX = 7.0f;

    g.fillAll (flheat::FLStudioLookAndFeel::kChassisBg);

    g.setColour (flheat::FLStudioLookAndFeel::kChassisBevel);
    g.drawHorizontalLine (1, 1.0f, w - 1.0f);

    // Top Header Bar (14% height)
    {
        g.setColour (flheat::FLStudioLookAndFeel::kRecessedBorder);
        g.drawLine (padX, headerBounds.getBottom() - 2.0f,
                    w - padX, headerBounds.getBottom() - 2.0f,
                    1.0f);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (13.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kTextHeader);

        const float titleX = padX + 1.0f;
        const float titleW = 38.0f;
        g.drawText ("HEAT",
                    juce::Rectangle<float> (titleX, headerBounds.getY(), titleW, headerBounds.getHeight()),
                    juce::Justification::centredLeft,
                    false);

        const juce::Point<float> ledCenter (titleX + titleW + 4.5f,
                                            headerBounds.getCentreY());
        const bool isBypassed = bypassButton.getToggleState();
        const float polarity  = (std::abs (displayedPolarity) > 0.01f)
                                  ? displayedPolarity
                                  : static_cast<float> (driveKnob.getValue());

        flheat::FLStudioLookAndFeel::drawHarmonicLED (g,
                                                      ledCenter,
                                                      3.2f,
                                                      displayedActivity,
                                                      polarity,
                                                      isBypassed);
    }

    auto drawKnobCard = [&g] (const juce::Rectangle<float>& panel,
                              const juce::String& title,
                              const juce::Rectangle<float>& readoutBox,
                              double currentVal,
                              bool isEnabled)
    {
        flheat::FLStudioLookAndFeel::drawRecessedPanel (g, panel, 1.0f);

        const float alpha = isEnabled ? 1.0f : 0.45f;

        const float labelH = panel.getHeight() * 0.19f;
        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.0f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kTextSecondary.withAlpha (alpha));
        g.drawText (title,
                    juce::Rectangle<float> (panel.getX(), panel.getY() + 1.5f, panel.getWidth(), labelH),
                    juce::Justification::centred,
                    false);

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (8.5f, true));
        g.setColour (flheat::FLStudioLookAndFeel::kAccentCyan.withAlpha (0.55f * alpha));
        g.drawText ("-",
                    juce::Rectangle<float> (panel.getX() + 10.0f,
                                            readoutBox.getY() - 11.0f,
                                            12.0f,
                                            10.0f),
                    juce::Justification::centredLeft,
                    false);

        g.setColour (flheat::FLStudioLookAndFeel::kAccentOrange.withAlpha (0.55f * alpha));
        g.drawText ("+",
                    juce::Rectangle<float> (panel.getRight() - 22.0f,
                                            readoutBox.getY() - 11.0f,
                                            12.0f,
                                            10.0f),
                    juce::Justification::centredRight,
                    false);

        const juce::Rectangle<float> pill = readoutBox.reduced (14.0f, 1.0f);
        g.setColour (juce::Colour (0xff141619).withAlpha (alpha));
        g.fillRoundedRectangle (pill, 2.5f);

        juce::Colour valCol = flheat::FLStudioLookAndFeel::kTextPrimary;
        if (currentVal > 0.05)
            valCol = flheat::FLStudioLookAndFeel::kAccentOrange;
        else if (currentVal < -0.05)
            valCol = flheat::FLStudioLookAndFeel::kAccentCyan;

        g.setFont (flheat::FLStudioLookAndFeel::getFLFont (10.5f, true));
        g.setColour (valCol.withAlpha (alpha));
        g.drawText (formatSignedValue (currentVal),
                    pill,
                    juce::Justification::centred,
                    false);
    };

    const bool controlsActive = ! bypassButton.getToggleState();
    drawKnobCard (drivePanelBounds, "DRIVE", driveReadoutBounds, driveKnob.getValue(), controlsActive);
    drawKnobCard (tonePanelBounds,  "TONE",  toneReadoutBounds,  toneKnob.getValue(),  controlsActive);

    g.setColour (flheat::FLStudioLookAndFeel::kChassisBorder);
    g.drawRect (0.0f, 0.0f, w, h, 1.0f);
}
