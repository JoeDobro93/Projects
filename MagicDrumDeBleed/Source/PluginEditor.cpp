#include "PluginEditor.h"
#include "Licensing/LicenseConfig.h"
#if ENABLE_DONATION_NAG
 #include "Licensing/NagDialog.h"
#endif

//==============================================================================
void StyledLookAndFeel::setPalette (const theme::Palette& p)
{
    pal = &p;

    setColour (juce::ResizableWindow::backgroundColourId, p.windowBackground);
    setColour (juce::DocumentWindow::textColourId,        p.text);

    setColour (juce::Label::textColourId, p.text);

    setColour (juce::Slider::textBoxTextColourId,       p.text);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId,             p.sliderThumb);
    setColour (juce::Slider::trackColourId,             p.knobFill.withAlpha (0.6f));
    setColour (juce::Slider::backgroundColourId,        p.sliderTrack);
    setColour (juce::Slider::rotarySliderFillColourId,  p.knobFill);
    setColour (juce::Slider::rotarySliderOutlineColourId, p.knobTrack);

    setColour (juce::TextButton::buttonColourId,   p.buttonOff);
    setColour (juce::TextButton::buttonOnColourId, p.buttonOn);
    setColour (juce::TextButton::textColourOffId,  p.buttonText);
    setColour (juce::TextButton::textColourOnId,   p.buttonTextOn);
    setColour (juce::ComboBox::outlineColourId,    p.buttonOutline);   // also TextButton outline

    setColour (juce::ToggleButton::textColourId,         p.text);
    setColour (juce::ToggleButton::tickColourId,         p.highlight);
    setColour (juce::ToggleButton::tickDisabledColourId, p.textDim);

    setColour (juce::ComboBox::backgroundColourId, p.comboBackground);
    setColour (juce::ComboBox::textColourId,       p.text);
    setColour (juce::ComboBox::arrowColourId,      p.textDim);

    setColour (juce::PopupMenu::backgroundColourId,            p.popupBackground);
    setColour (juce::PopupMenu::textColourId,                  p.text);
    setColour (juce::PopupMenu::headerTextColourId,            p.textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.highlight);
    setColour (juce::PopupMenu::highlightedTextColourId,       p.buttonTextOn);

    setColour (juce::TextEditor::backgroundColourId,     p.comboBackground);
    setColour (juce::TextEditor::textColourId,           p.text);
    setColour (juce::TextEditor::outlineColourId,        p.buttonOutline);
    setColour (juce::TextEditor::focusedOutlineColourId, p.highlight);
    setColour (juce::TextEditor::highlightColourId,      p.highlight.withAlpha (0.5f));
    setColour (juce::CaretComponent::caretColourId,      p.text);

    setColour (juce::AlertWindow::backgroundColourId, p.popupBackground);
    setColour (juce::AlertWindow::textColourId,       p.text);
    setColour (juce::AlertWindow::outlineColourId,    p.panelOutline);
}

void StyledLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float radius   = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 2.0f;
    const auto  centre   = bounds.getCentre();
    const float angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float lineW    = juce::jlimit (2.0f, 4.0f, radius * 0.16f);
    const float arcR     = radius - lineW * 0.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (pal->knobTrack);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (pal->knobFill);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const float bodyR = arcR - lineW * 0.9f;
    g.setColour (pal->knobBody);
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-lineW * 0.4f, -bodyR, lineW * 0.8f, bodyR * 0.55f, lineW * 0.3f);
    g.setColour (pal->knobPointer);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
MagicDrumDeBleedAudioProcessorEditor::MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc)
    : AudioProcessorEditor (&proc),
      processor (proc),
      presetBrowser (proc),
      compressorPanel (proc),
      eqPanel (proc),
      outputStrip (proc, [this] { toggleTheme(); })
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (presetBrowser);
    addAndMakeVisible (compressorPanel);
    addAndMakeVisible (eqPanel);
    addAndMakeVisible (outputStrip);

    setResizable (true, true);
    setResizeLimits (650, 400, 4000, 2600);

    const auto saved = processor.getSavedEditorSize();
    setSize (juce::jlimit (650, 4000, saved.x), juce::jlimit (400, 2600, saved.y));

    applyTheme();

   #if ENABLE_DONATION_NAG
    juce::MessageManager::callAsync ([] { license::NagDialog::launchIfNeeded(); });
   #endif
}

MagicDrumDeBleedAudioProcessorEditor::~MagicDrumDeBleedAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MagicDrumDeBleedAudioProcessorEditor::toggleTheme()
{
    processor.setDarkTheme (! processor.isDarkTheme());
    applyTheme();
}

void MagicDrumDeBleedAudioProcessorEditor::applyTheme()
{
    pal = &theme::get (processor.isDarkTheme());

    lookAndFeel.setPalette (*pal);
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    compressorPanel.setPalette (*pal);
    eqPanel.setPalette (*pal);
    outputStrip.setPalette (*pal);
    presetBrowser.setPalette (*pal);

    sendLookAndFeelChange();
    repaint();
}

void MagicDrumDeBleedAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal->windowBackground);

    const int headerHeight = juce::jlimit (28, 40, getHeight() / 14);
    g.setColour (pal->headerBackground);
    g.fillRect (0, 0, getWidth(), headerHeight);
    g.setColour (pal->panelOutline);
    g.drawHorizontalLine (headerHeight, 0.0f, (float) getWidth());
}

void MagicDrumDeBleedAudioProcessorEditor::resized()
{
    processor.setSavedEditorSize (getWidth(), getHeight());

    auto r = getLocalBounds();

    // ---- Header: title + presets ----
    const int headerHeight = juce::jlimit (28, 40, getHeight() / 14);
    auto header = r.removeFromTop (headerHeight).reduced (8, 3);
    titleLabel.setFont (juce::Font (juce::FontOptions ((float) headerHeight * 0.52f, juce::Font::bold)));
    auto presetArea = header.removeFromRight (juce::jlimit (240, 380, getWidth() / 2));
    presetBrowser.setBounds (presetArea);
    titleLabel.setBounds (header);

    // ---- Bottom strip ----
    const int stripHeight = juce::jlimit (40, 60, getHeight() / 10);
    outputStrip.setBounds (r.removeFromBottom (stripHeight).reduced (4, 2));

    // ---- Compressor (top ~42%) / EQ (rest) ----
    r.reduce (4, 2);
    compressorPanel.setBounds (r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.42f)));
    eqPanel.setBounds (r);
}
