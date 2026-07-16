#include "CompressorPanel.h"

//==============================================================================
InputLevelMeter::InputLevelMeter (std::function<float()> levelGetter, std::function<float()> thresholdGetter)
    : getLevel (std::move (levelGetter)), getThreshold (std::move (thresholdGetter))
{
    startTimerHz (30);
}

void InputLevelMeter::timerCallback()
{
    const float v = juce::jlimit (-90.0f, 6.0f, getLevel());

    if (v > displayedDb)
        displayedDb = v;                                 // instant attack
    else
        displayedDb += 0.12f * (v - displayedDb);        // smooth fall

    // Lingering peak line: hold ~1.2 s, then drift down.
    if (v >= peakDb)
    {
        peakDb = v;
        peakHoldFrames = 36;
    }
    else if (peakHoldFrames > 0)
    {
        --peakHoldFrames;
    }
    else
    {
        peakDb = juce::jmax (peakDb - 0.8f, -90.0f);
    }

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

    auto bar = bounds.reduced (juce::jmax (2.0f, bounds.getWidth() * 0.16f), 4.0f);

    // Scale: -60 .. 0 dB, matching the Threshold range.
    constexpr float minDb = -60.0f, maxDb = 0.0f;
    auto yForDb = [&] (float db)
    {
        return juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, bar.getBottom(), bar.getY());
    };

    // Tick marks every 12 dB (with numbers when there's room)
    const bool drawNumbers = getWidth() >= 40;
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    for (int db = 0; db >= (int) minDb; db -= 12)
    {
        const float y = yForDb ((float) db);
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
        const float top = yForDb (displayedDb);
        g.setColour (pal->meterFill.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (bar.getX(), top, bar.getWidth(), bar.getBottom() - top));
    }

    // Peak-hold line
    if (peakDb > minDb + 0.5f)
    {
        g.setColour (pal->meterPeak);
        const float y = yForDb (peakDb);
        g.fillRect (juce::Rectangle<float> (bar.getX(), y - 1.0f, bar.getWidth(), 2.0f));
    }

    // Threshold marker: where the gate engages relative to this meter.
    g.setColour (pal->highlight.brighter (0.4f));
    const float ty = yForDb (getThreshold());
    g.fillRect (juce::Rectangle<float> (bounds.getX() + 1.0f, ty - 1.0f, bounds.getWidth() - 2.0f, 2.0f));
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

    // Numeric readout at the bottom
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
                  [&proc] { return proc.apvts.getRawParameterValue (ParamIDs::threshold)->load(); }),
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

    // Preview = solo the detector signal (monitor mode "Sidechain").
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
        // Restore the label after a moment.
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

    // Divider before the sidechain section
    if (! scTitleLabel.getBounds().isEmpty())
    {
        g.setColour (pal->panelOutline);
        const float x = (float) scTitleLabel.getX() - 6.0f;
        g.drawLine (x, bounds.getY() + 8.0f, x, bounds.getBottom() - 8.0f);
    }
}

void CompressorPanel::resized()
{
    auto r = getLocalBounds().reduced (juce::jmax (4, getHeight() / 40));

    const int titleHeight = juce::jlimit (14, 20, getHeight() / 10);
    auto titleRow = r.removeFromTop (titleHeight);
    titleLabel.setFont (juce::Font (juce::FontOptions ((float) titleHeight - 3.0f, juce::Font::bold)));
    titleLabel.setBounds (titleRow.removeFromLeft (juce::roundToInt ((float) getWidth() * 0.32f)));
    bypassButton.setBounds (titleRow.removeFromLeft (juce::jlimit (66, 90, getWidth() / 9)));
    r.removeFromTop (2);

    // ---- Meters on the left: input level, then gain reduction ----
    const int meterW = juce::jlimit (34, 56, getWidth() / 16);
    inputMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (3);
    grMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (8);

    auto scArea = r.removeFromRight (juce::roundToInt ((float) r.getWidth() * 0.36f));
    r.removeFromRight (10);

    // ---- Main knob grid (3 × 2) with breathing room between the rows ----
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

    // ---- Sidechain section ----
    auto scHeader = scArea.removeFromTop (juce::jlimit (14, 18, getHeight() / 12));
    scTitleLabel.setFont (juce::Font (juce::FontOptions ((float) scHeader.getHeight() - 4.0f, juce::Font::bold)));
    scTitleLabel.setBounds (scHeader.removeFromLeft (scHeader.getWidth() / 2));
    scEnableButton.setBounds (scHeader);

    const int scCellW = scArea.getWidth() / 2;
    const int scRowGap = juce::jlimit (2, 10, getHeight() / 30);
    const int scCellH = (scArea.getHeight() - scRowGap) / 2;
    scFreq.setBounds (scArea.getX(),           scArea.getY(), scCellW, scCellH);
    scQ.setBounds    (scArea.getX() + scCellW, scArea.getY(), scCellW, scCellH);

    auto scBottom = juce::Rectangle<int> (scArea.getX(), scArea.getY() + scCellH + scRowGap,
                                          scArea.getWidth(), scCellH);
    const int buttonH = juce::jlimit (20, 28, scCellH / 2);
    auto learnCell = scBottom.removeFromLeft (scCellW);
    learnButton.setBounds (learnCell.withSizeKeepingCentre (juce::jmax (52, scCellW - 16), buttonH));
    scPreviewButton.setBounds (scBottom.withSizeKeepingCentre (juce::jmax (52, scBottom.getWidth() - 16), buttonH));
}
