#include "AdvancedView.h"

AdvancedView::AdvancedView (MagicDrumDeBleedAudioProcessor& proc,
                            std::function<void()> onThemeToggle,
                            std::function<void()> onSimpleView)
    : presetBrowser (proc),
      compressorPanel (proc),
      eqPanel (proc),
      outputStrip (proc, std::move (onThemeToggle), std::move (onSimpleView))
{
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (presetBrowser);
    addAndMakeVisible (compressorPanel);
    addAndMakeVisible (eqPanel);
    addAndMakeVisible (outputStrip);
}

void AdvancedView::setPalette (const theme::Palette& p)
{
    pal = &p;
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    compressorPanel.setPalette (p);
    eqPanel.setPalette (p);
    outputStrip.setPalette (p);
    presetBrowser.setPalette (p);
    repaint();
}

void AdvancedView::paint (juce::Graphics& g)
{
    g.fillAll (pal->windowBackground);

    const int headerHeight = juce::jlimit (28, 40, getHeight() / 14);
    g.setColour (pal->headerBackground);
    g.fillRect (0, 0, getWidth(), headerHeight);
    g.setColour (pal->panelOutline);
    g.drawHorizontalLine (headerHeight, 0.0f, (float) getWidth());
}

void AdvancedView::resized()
{
    auto r = getLocalBounds();

    const int headerHeight = juce::jlimit (28, 40, getHeight() / 14);
    auto header = r.removeFromTop (headerHeight).reduced (8, 3);
    titleLabel.setFont (juce::Font (juce::FontOptions ((float) headerHeight * 0.52f, juce::Font::bold)));
    auto presetArea = header.removeFromRight (juce::jlimit (240, 380, getWidth() / 2));
    presetBrowser.setBounds (presetArea);
    titleLabel.setBounds (header);

    const int stripHeight = juce::jlimit (40, 60, getHeight() / 10);
    outputStrip.setBounds (r.removeFromBottom (stripHeight).reduced (4, 2));

    r.reduce (4, 2);
    compressorPanel.setBounds (r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.42f)));
    eqPanel.setBounds (r);
}
