#include "CompressorPanel.h"

//==============================================================================
InputLevelMeter::InputLevelMeter (std::function<float()> levelGetter, juce::RangedAudioParameter* thresholdParam)
    : getLevel (std::move (levelGetter)), threshold (thresholdParam)
{
    startTimerHz (30);
}

juce::Rectangle<float> InputLevelMeter::barArea() const
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    bounds.removeFromTop (12.0f);   // "IN" caption
    return bounds.reduced (juce::jmax (2.0f, bounds.getWidth() * 0.16f), 4.0f);
}

float InputLevelMeter::dbToY (float db) const
{
    auto bar = barArea();
    return juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, bar.getBottom(), bar.getY());
}

float InputLevelMeter::yToDb (float y) const
{
    auto bar = barArea();
    return juce::jmap (juce::jlimit (bar.getY(), bar.getBottom(), y), bar.getBottom(), bar.getY(), minDb, maxDb);
}

void InputLevelMeter::setThresholdFromMouse (float y)
{
    if (threshold == nullptr)
        return;

    const float db = yToDb (y);
    threshold->setValueNotifyingHost (threshold->convertTo0to1 (db));
}

juce::MouseCursor InputLevelMeter::getMouseCursor()
{
    return threshold != nullptr ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor;
}

void InputLevelMeter::mouseDown (const juce::MouseEvent& e)
{
    if (threshold == nullptr)
        return;
    dragging = true;
    threshold->beginChangeGesture();
    setThresholdFromMouse (e.position.y);
}

void InputLevelMeter::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        setThresholdFromMouse (e.position.y);
}

void InputLevelMeter::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        threshold->endChangeGesture();
        dragging = false;
    }
}

void InputLevelMeter::timerCallback()
{
    const float v = juce::jlimit (-90.0f, 6.0f, getLevel());

    if (v > displayedDb)
        displayedDb = v;                                 // instant attack
    else
        displayedDb += 0.12f * (v - displayedDb);        // smooth fall

    if (v >= peakDb)
    {
        peakDb = v;
        peakHoldFrames = 36;                             // ~1.2 s linger
    }
    else if (peakHoldFrames > 0)
        --peakHoldFrames;
    else
        peakDb = juce::jmax (peakDb - 0.8f, -90.0f);

    repaint();
}

void InputLevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (pal->meterBackground);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    g.setColour (pal->textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText ("IN", bounds.removeFromTop (12.0f), juce::Justification::centred, false);

    auto bar = barArea();

    const bool drawNumbers = getWidth() >= 40;
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    for (int db = 0; db >= (int) minDb; db -= 12)
    {
        const float y = dbToY ((float) db);
        g.setColour (pal->meterTicks);
        g.drawHorizontalLine ((int) y, bar.getX(), bar.getRight());
        if (drawNumbers && db < 0 && db > (int) minDb)
        {
            g.setColour (pal->textDim);
            g.drawText (juce::String (db), (int) bar.getX(), (int) y - 9,
                        (int) bar.getWidth(), 9, juce::Justification::centredRight, false);
        }
    }

    // Level bar (bottom-up)
    if (displayedDb > minDb)
    {
        const float top = dbToY (displayedDb);
        g.setColour (pal->meterFill.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (bar.getX(), top, bar.getWidth(), bar.getBottom() - top));
    }

    // Peak-hold line
    if (peakDb > minDb + 0.5f)
    {
        g.setColour (pal->meterPeak);
        const float y = dbToY (peakDb);
        g.fillRect (juce::Rectangle<float> (bar.getX(), y - 1.0f, bar.getWidth(), 2.0f));
    }

    // Threshold marker: draggable line + grab tab on the left edge.
    if (threshold != nullptr)
    {
        const float td = threshold->convertFrom0to1 (threshold->getValue());
        const float ty = dbToY (td);
        g.setColour (pal->highlight.brighter (0.4f));
        g.fillRect (juce::Rectangle<float> (bounds.getX() + 1.0f, ty - 1.0f, bounds.getWidth() - 2.0f, 2.0f));

        juce::Path tab;
        tab.addTriangle (bounds.getX() + 1.0f, ty - 4.0f,
                         bounds.getX() + 1.0f, ty + 4.0f,
                         bounds.getX() + 6.0f, ty);
        g.fillPath (tab);
    }
}

//==============================================================================
GainReductionMeter::GainReductionMeter (std::function<float()> valueGetter)
    : getValue (std::move (valueGetter))
{
    startTimerHz (30);
}

void GainReductionMeter::timerCallback()
{
    const float v = getValue();          // negative dB (0 = no reduction)
    if (v < displayedDb)
        displayedDb = v;                                 // instant attack
    else
        displayedDb += 0.15f * (v - displayedDb);        // smooth release

    repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (pal->meterBackground);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    g.setColour (pal->textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText ("GR", bounds.removeFromTop (12.0f), juce::Justification::centred, false);

    g.setFont (juce::Font (juce::FontOptions (juce::jlimit (9.0f, 12.0f, bounds.getWidth() * 0.28f))));
    g.drawText (juce::String (displayedDb, 1), bounds.removeFromBottom (14.0f),
                juce::Justification::centred, false);

    auto bar = bounds.reduced (juce::jmax (2.0f, bounds.getWidth() * 0.18f), 2.0f);

    constexpr float rangeDb = 60.0f;     // display 0 .. -60 dB, top-down
    const float frac = juce::jlimit (0.0f, 1.0f, -displayedDb / rangeDb);

    g.setColour (pal->meterTicks);
    for (int dB = 12; dB < (int) rangeDb; dB += 12)
    {
        const float y = bar.getY() + bar.getHeight() * ((float) dB / rangeDb);
        g.drawHorizontalLine ((int) y, bar.getX(), bar.getRight());
    }

    if (frac > 0.001f)
    {
        auto fill = bar.removeFromTop (bar.getHeight() * frac);
        g.setColour (frac > 0.8f ? pal->meterPeak : pal->meterFill);
        g.fillRect (fill);
    }
}

//==============================================================================
CompressorPanel::CompressorPanel (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      inputMeter ([&proc] { return proc.getDetectorRmsDb(); },
                  proc.apvts.getParameter (ParamIDs::threshold)),
      grMeter ([&proc] { return proc.getGainReductionDb(); })
{
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);
    scTitleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (scTitleLabel);

    auto& apvts = processor.apvts;
    auto attach = [&] (ui::LabelledKnob& knob, const juce::String& id)
    {
        addAndMakeVisible (knob);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (apvts, id, knob.slider));
    };

    attach (threshold, ParamIDs::threshold);
    attach (reduction, ParamIDs::reduction);
    attach (lookahead, ParamIDs::lookahead);
    attach (rmsWindow, ParamIDs::rmsWindow);
    attach (hold,      ParamIDs::hold);
    attach (release,   ParamIDs::release);
    attach (scFreq,    ParamIDs::scFreq);
    attach (scQ,       ParamIDs::scQ);

    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (apvts, ParamIDs::compBypass, bypassButton);

    addAndMakeVisible (scEnableButton);
    scEnableAttachment = std::make_unique<ButtonAttachment> (apvts, ParamIDs::scEnable, scEnableButton);

    learnButton.onClick = [this] { learnClicked(); };
    addAndMakeVisible (learnButton);

    scPreviewButton.setClickingTogglesState (false);
    scPreviewButton.onClick = [this]
    {
        if (monitorAttachment != nullptr)
        {
            const bool active = (int) processor.apvts.getRawParameterValue (ParamIDs::monitorMode)->load()
                                    == MagicDrumDeBleedAudioProcessor::monitorSidechain;
            monitorAttachment->setValueAsCompleteGesture (active
                ? (float) MagicDrumDeBleedAudioProcessor::monitorNormal
                : (float) MagicDrumDeBleedAudioProcessor::monitorSidechain);
        }
    };
    addAndMakeVisible (scPreviewButton);

    if (auto* modeParam = apvts.getParameter (ParamIDs::monitorMode))
    {
        monitorAttachment = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
        {
            scPreviewButton.setToggleState (juce::roundToInt (v)
                                                == MagicDrumDeBleedAudioProcessor::monitorSidechain,
                                            juce::dontSendNotification);
        });
        monitorAttachment->sendInitialUpdate();
    }

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (grMeter);
}

void CompressorPanel::learnClicked()
{
    if (! processor.isLearning())
    {
        processor.startLearn();
        learnButton.setButtonText ("Stop");
        learnButton.setColour (juce::TextButton::buttonColourId, pal->buttonOn);
        return;
    }

    const double freq = processor.finishLearnAndAnalyse();
    learnButton.setButtonText (freq > 0.0 ? "Learn" : "No signal");
    learnButton.setColour (juce::TextButton::buttonColourId, pal->buttonOff);

    if (freq > 0.0)
    {
        if (auto* param = processor.apvts.getParameter (ParamIDs::scFreq))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) freq));
            param->endChangeGesture();
        }
    }
    else
    {
        juce::Timer::callAfterDelay (1500, [safe = juce::Component::SafePointer<CompressorPanel> (this)]
        {
            if (safe != nullptr)
                safe->learnButton.setButtonText ("Learn");
        });
    }
}

void CompressorPanel::setPalette (const theme::Palette& p)
{
    pal = &p;
    inputMeter.setPalette (pal);
    grMeter.setPalette (pal);
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    scTitleLabel.setColour (juce::Label::textColourId, pal->textDim);
    learnButton.setColour (juce::TextButton::buttonColourId,
                           processor.isLearning() ? pal->buttonOn : pal->buttonOff);
    scPreviewButton.setColour (juce::TextButton::buttonOnColourId, pal->buttonOn);
    repaint();
}

void CompressorPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (pal->panelBackground);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    if (sidechainDividerX > 0)
    {
        g.setColour (pal->panelOutline);
        g.drawLine ((float) sidechainDividerX, bounds.getY() + 8.0f,
                    (float) sidechainDividerX, bounds.getBottom() - 8.0f);
    }
}

void CompressorPanel::resized()
{
    auto r = getLocalBounds().reduced (juce::jmax (4, getHeight() / 40));

    const int titleHeight = juce::jlimit (14, 20, getHeight() / 10);
    auto titleRow = r.removeFromTop (titleHeight);
    titleLabel.setFont (juce::Font (juce::FontOptions ((float) titleHeight - 3.0f, juce::Font::bold)));
    titleLabel.setBounds (titleRow);
    r.removeFromTop (2);

    // Bottom strip holds the anchored toggles (Bypass under the knobs,
    // Filter On + Learn/Preview under the sidechain).
    const int bottomH = juce::jlimit (20, 28, getHeight() / 8);
    auto bottom = r.removeFromBottom (bottomH);
    r.removeFromBottom (3);

    // ---- Meters on the left ----
    const int meterW = juce::jlimit (34, 56, getWidth() / 16);
    inputMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (3);
    grMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (8);

    auto scArea = r.removeFromRight (juce::roundToInt ((float) r.getWidth() * 0.36f));
    r.removeFromRight (10);
    sidechainDividerX = scArea.getX() - 5;

    // ---- Main knob grid (3 × 2) with room between rows ----
    const int rowGap = juce::jlimit (4, 14, getHeight() / 22);
    const int cellW = r.getWidth() / 3;
    const int cellH = (r.getHeight() - rowGap) / 2;
    ui::LabelledKnob* grid[2][3] = { { &threshold, &reduction, &lookahead },
                                     { &rmsWindow, &hold,      &release } };
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 3; ++col)
            grid[row][col]->setBounds (r.getX() + col * cellW,
                                       r.getY() + row * (cellH + rowGap),
                                       cellW, cellH);

    // ---- Sidechain header + knobs ----
    auto scHeader = scArea.removeFromTop (juce::jlimit (14, 18, getHeight() / 12));
    scTitleLabel.setFont (juce::Font (juce::FontOptions ((float) scHeader.getHeight() - 4.0f, juce::Font::bold)));
    scTitleLabel.setBounds (scHeader);

    const int scCellW = scArea.getWidth() / 2;
    scFreq.setBounds (scArea.getX(),           scArea.getY(), scCellW, scArea.getHeight());
    scQ.setBounds    (scArea.getX() + scCellW, scArea.getY(), scCellW, scArea.getHeight());

    // ---- Bottom strip layout ----
    // Compressor bypass: bottom-left of the main (knob) segment.
    auto knobBottom = bottom.withRight (sidechainDividerX);
    bypassButton.setBounds (knobBottom.removeFromLeft (juce::jlimit (66, 96, getWidth() / 9))
                                .withSizeKeepingCentre (juce::jlimit (66, 96, getWidth() / 9), bottomH - 2));

    // Sidechain bottom row: [Filter On]  [Learn][Preview]
    auto scBottom = bottom.withLeft (scArea.getX());
    const int btnW = juce::jlimit (46, 72, scBottom.getWidth() / 3);
    scEnableButton.setBounds (scBottom.removeFromLeft (juce::jlimit (60, 84, scBottom.getWidth() / 2))
                                  .withSizeKeepingCentre (juce::jlimit (60, 84, scBottom.getWidth()), bottomH - 2));
    scPreviewButton.setBounds (scBottom.removeFromRight (btnW).reduced (1, 1));
    learnButton.setBounds (scBottom.removeFromRight (btnW + 2).reduced (1, 1));
}
