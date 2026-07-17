#include "CompressorPanel.h"

//==============================================================================
InputLevelMeter::InputLevelMeter (juce::String captionText, std::function<float()> levelGetter,
                                  juce::RangedAudioParameter* thresholdParam)
    : caption (std::move (captionText)), getLevel (std::move (levelGetter)), threshold (thresholdParam)
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
    g.drawText (caption, bounds.removeFromTop (12.0f), juce::Justification::centred, false);

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
GainReductionMeter::GainReductionMeter (juce::String captionText, std::function<float()> valueGetter)
    : caption (std::move (captionText)), getValue (std::move (valueGetter))
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
    g.drawText (caption, bounds.removeFromTop (12.0f), juce::Justification::centred, false);

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
      inputMeter ("IN", [&proc] { return proc.getDetectorRmsDb(); },
                  proc.apvts.getParameter (ParamIDs::threshold)),
      grMeter ("GR", [&proc] { return proc.getGainReductionDb(); })
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
    // The editor scales the whole view uniformly, so this layout always runs
    // at the fixed logical size — plain arithmetic, no clamping needed.
    auto r = getLocalBounds().reduced (8, 8);

    auto titleRow = r.removeFromTop (20);
    r.removeFromTop (4);

    auto bottomRow = r.removeFromBottom (26);
    r.removeFromBottom (4);

    // ---- Meters on the left ----
    auto meters = r.removeFromLeft (96);
    inputMeter.setBounds (meters.removeFromLeft (48));
    meters.removeFromLeft (4);
    grMeter.setBounds (meters);
    r.removeFromLeft (10);

    // ---- Five equal knob columns: 3 main + gap + 2 sidechain ----
    // Every knob cell is exactly the same size, so every knob renders at the
    // same diameter.
    const int dividerGap = 14;
    const int cellW = (r.getWidth() - dividerGap) / 5;
    const int rowGap = 8;
    const int cellH = (r.getHeight() - rowGap) / 2;

    auto mainArea = r.removeFromLeft (cellW * 3);
    auto scArea   = r.withTrimmedLeft (dividerGap);
    sidechainDividerX = scArea.getX() - dividerGap / 2;

    ui::LabelledKnob* grid[2][3] = { { &threshold, &reduction, &lookahead },
                                     { &rmsWindow, &hold,      &release } };
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 3; ++col)
            grid[row][col]->setBounds (mainArea.getX() + col * cellW,
                                       mainArea.getY() + row * (cellH + rowGap),
                                       cellW, cellH);

    // Sidechain: knobs in the top row (same cells), Learn/Preview below.
    scFreq.setBounds (scArea.getX(),         scArea.getY(), cellW, cellH);
    scQ.setBounds    (scArea.getX() + cellW, scArea.getY(), cellW, cellH);

    auto scRow2 = juce::Rectangle<int> (scArea.getX(), scArea.getY() + cellH + rowGap,
                                        cellW * 2, cellH);
    auto learnCell   = scRow2.removeFromLeft (cellW);
    learnButton.setBounds (learnCell.withSizeKeepingCentre (cellW - 26, 30));
    scPreviewButton.setBounds (scRow2.withSizeKeepingCentre (cellW - 26, 30));

    // ---- Titles ----
    titleLabel.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    titleLabel.setBounds (titleRow.withWidth (400));
    scTitleLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    scTitleLabel.setBounds (titleRow.withX (scArea.getX()).withWidth (scArea.getWidth()));

    // ---- Anchored toggles: Bypass bottom-left of the panel, Filter On
    //      bottom-left of the sidechain segment ----
    bypassButton.setBounds (bottomRow.getX(), bottomRow.getY(), 92, 24);
    scEnableButton.setBounds (scArea.getX(), bottomRow.getY(), 110, 24);
}
