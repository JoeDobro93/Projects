#include "SimpleView.h"

SimpleView::SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView)
    : processor (proc),
      inputMeter ("IN", [&proc] { return proc.getDetectorRmsDb(); },
                  proc.apvts.getParameter (ParamIDs::threshold)),
      grMeter ("GR", [&proc] { return proc.getGainReductionDb(); }),
      outputMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); }, nullptr)
{
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (grMeter);
    addAndMakeVisible (outputMeter);

    intensityLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (intensityLabel);

    intensitySlider.setSliderStyle (juce::Slider::LinearVertical);
    intensitySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (intensitySlider);
    intensityAttachment = std::make_unique<SliderAttachment> (processor.apvts, ParamIDs::intensity, intensitySlider);

    advancedButton.onClick = [onAdvancedView = std::move (onAdvancedView)] { if (onAdvancedView) onAdvancedView(); };
    addAndMakeVisible (advancedButton);
}

void SimpleView::setPalette (const theme::Palette& p)
{
    pal = &p;
    inputMeter.setPalette (&p);
    grMeter.setPalette (&p);
    outputMeter.setPalette (&p);
    intensityLabel.setColour (juce::Label::textColourId, p.text);
    repaint();
}

void SimpleView::paint (juce::Graphics& g)
{
    g.fillAll (pal->windowBackground);

    const int headerHeight = juce::jlimit (26, 40, getHeight() / 12);
    g.setColour (pal->headerBackground);
    g.fillRect (0, 0, getWidth(), headerHeight);
    g.setColour (pal->title);
    g.setFont (juce::Font (juce::FontOptions ((float) headerHeight * 0.5f, juce::Font::bold)));
    g.drawText ("MAGIC DRUM DE-BLEED", juce::Rectangle<int> (10, 0, getWidth() - 20, headerHeight),
                juce::Justification::centredLeft, false);
}

void SimpleView::resized()
{
    auto r = getLocalBounds();

    const int headerHeight = juce::jlimit (26, 40, getHeight() / 12);
    r.removeFromTop (headerHeight);
    r.reduce (12, 10);

    const int buttonHeight = juce::jlimit (26, 38, getHeight() / 12);
    auto buttonRow = r.removeFromBottom (buttonHeight);
    advancedButton.setBounds (buttonRow.withSizeKeepingCentre (juce::jmin (buttonRow.getWidth(), 180), buttonHeight - 4));
    r.removeFromBottom (8);

    // Left to right: IN, GR, Intensity (vertical), OUT.
    const int gap = 8;
    const int meterW = 56;
    inputMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (gap);
    grMeter.setBounds (r.removeFromLeft (meterW));
    r.removeFromLeft (gap);

    auto outArea = r.removeFromRight (meterW);
    outputMeter.setBounds (outArea);
    r.removeFromRight (gap);

    intensityLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    intensityLabel.setBounds (r.removeFromTop (16));
    intensitySlider.setBounds (r.reduced (2, 2));
}
