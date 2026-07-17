#include "AdvancedView.h"

AdvancedView::AdvancedView (MagicDrumDeBleedAudioProcessor& proc,
                            std::function<void()> onThemeToggle,
                            std::function<void()> onSimpleView)
    : presetBrowser (proc),
      compressorPanel (proc),
      eqPanel (proc),
      outputStrip (proc, std::move (onThemeToggle), std::move (onSimpleView)),
      outputMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); }, nullptr)
{
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (presetBrowser);
    addAndMakeVisible (compressorPanel);
    addAndMakeVisible (eqPanel);
    addAndMakeVisible (outputStrip);

    intensityLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (intensityLabel);

    intensitySlider.setSliderStyle (juce::Slider::LinearVertical);
    intensitySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (intensitySlider);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ParamIDs::intensity, intensitySlider);

    addAndMakeVisible (outputMeter);
}

void AdvancedView::setPalette (const theme::Palette& p)
{
    pal = &p;
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    intensityLabel.setColour (juce::Label::textColourId, pal->text);
    compressorPanel.setPalette (p);
    eqPanel.setPalette (p);
    outputStrip.setPalette (p);
    presetBrowser.setPalette (p);
    outputMeter.setPalette (&p);
    repaint();
}

void AdvancedView::paint (juce::Graphics& g)
{
    g.fillAll (pal->windowBackground);

    g.setColour (pal->headerBackground);
    g.fillRect (0, 0, getWidth(), 40);
    g.setColour (pal->panelOutline);
    g.drawHorizontalLine (40, 0.0f, (float) getWidth());
}

void AdvancedView::resized()
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (40).reduced (8, 3);
    titleLabel.setFont (juce::Font (juce::FontOptions (21.0f, juce::Font::bold)));
    presetBrowser.setBounds (header.removeFromRight (380));
    titleLabel.setBounds (header);

    // ---- Right column: vertical Intensity + full-height output meter ----
    auto rightColumn = r.removeFromRight (116).reduced (4, 6);
    outputMeter.setBounds (rightColumn.removeFromRight (46));
    rightColumn.removeFromRight (4);
    intensityLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    intensityLabel.setBounds (rightColumn.removeFromTop (16));
    intensitySlider.setBounds (rightColumn.reduced (2, 2));

    // ---- Bottom strip + panels ----
    outputStrip.setBounds (r.removeFromBottom (54).reduced (4, 2));
    r.reduce (4, 2);
    compressorPanel.setBounds (r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.42f)));
    eqPanel.setBounds (r);
}
