#include "CompressorPanel.h"

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

    auto bar = bounds.reduced (juce::jmax (2.0f, bounds.getWidth() * 0.18f), 4.0f);

    constexpr float rangeDb = 60.0f;     // display 0 .. -60 dB
    const float frac = juce::jlimit (0.0f, 1.0f, -displayedDb / rangeDb);

    // Scale ticks every 12 dB
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

    // Numeric readout at the bottom
    g.setColour (pal->textDim);
    g.setFont (juce::Font (juce::FontOptions (juce::jlimit (9.0f, 12.0f, bounds.getWidth() * 0.28f))));
    g.drawText (juce::String (displayedDb, 1), bounds.removeFromBottom (14.0f),
                juce::Justification::centred, false);
}

//==============================================================================
CompressorPanel::CompressorPanel (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      meter ([&proc] { return proc.getGainReductionDb(); })
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
    attach (ceiling,   ParamIDs::learnCeiling);

    addAndMakeVisible (scEnableButton);
    scEnableAttachment = std::make_unique<ButtonAttachment> (apvts, ParamIDs::scEnable, scEnableButton);

    learnButton.onClick = [this] { learnClicked(); };
    addAndMakeVisible (learnButton);

    addAndMakeVisible (meter);
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
    meter.setPalette (pal);
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    scTitleLabel.setColour (juce::Label::textColourId, pal->textDim);
    learnButton.setColour (juce::TextButton::buttonColourId,
                           processor.isLearning() ? pal->buttonOn : pal->buttonOff);
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
    titleLabel.setBounds (titleRow);

    meter.setBounds (r.removeFromRight (juce::jlimit (36, 60, getWidth() / 14)));
    r.removeFromRight (4);

    auto scArea = r.removeFromRight (juce::roundToInt ((float) r.getWidth() * 0.36f));
    r.removeFromRight (6);

    // ---- Main knob grid (3 × 2) ----
    const int cellW = r.getWidth() / 3;
    const int cellH = r.getHeight() / 2;
    ui::LabelledKnob* grid[2][3] = { { &threshold, &reduction, &lookahead },
                                     { &rmsWindow, &hold,      &release } };
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 3; ++col)
            grid[row][col]->setBounds (r.getX() + col * cellW, r.getY() + row * cellH, cellW, cellH);

    // ---- Sidechain section ----
    auto scHeader = scArea.removeFromTop (juce::jlimit (14, 18, getHeight() / 12));
    scTitleLabel.setFont (juce::Font (juce::FontOptions ((float) scHeader.getHeight() - 4.0f, juce::Font::bold)));
    scTitleLabel.setBounds (scHeader.removeFromLeft (scHeader.getWidth() / 2));
    scEnableButton.setBounds (scHeader);

    const int scCellW = scArea.getWidth() / 2;
    const int scCellH = scArea.getHeight() / 2;
    scFreq.setBounds (scArea.getX(),           scArea.getY(), scCellW, scCellH);
    scQ.setBounds    (scArea.getX() + scCellW, scArea.getY(), scCellW, scCellH);

    auto scBottom = scArea.withTrimmedTop (scCellH);
    auto learnCell = scBottom.removeFromLeft (scCellW);
    learnButton.setBounds (learnCell.withSizeKeepingCentre (juce::jmax (52, scCellW - 16),
                                                            juce::jlimit (20, 28, scCellH / 2)));
    ceiling.setBounds (scBottom);
}
