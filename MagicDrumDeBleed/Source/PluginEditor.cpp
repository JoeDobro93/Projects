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
    setColour (juce::ComboBox::outlineColourId,    p.buttonOutline);

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

juce::Label* StyledLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    // A comfortable fixed logical font that never clips inside the value box,
    // and shrinks-to-fit horizontally instead of cropping while typing.
    label->setFont (juce::Font (juce::FontOptions (13.0f)));
    label->setJustificationType (juce::Justification::centred);
    label->setMinimumHorizontalScale (0.8f);
    label->setBorderSize (juce::BorderSize<int> (1));
    return label;
}

//==============================================================================
MagicDrumDeBleedAudioProcessorEditor::MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc)
    : AudioProcessorEditor (&proc),
      processor (proc),
      advancedView (proc, [this] { toggleTheme(); }, [this] { setView (true); }),
      simpleView (proc, [this] { setView (false); })
{
    setLookAndFeel (&lookAndFeel);

    addChildComponent (advancedView);
    addChildComponent (simpleView);

    setConstrainer (&constrainer);
    setResizable (true, true);

    applyTheme();

    const bool simple = processor.isSimpleView();
    advancedView.setVisible (! simple);
    simpleView.setVisible (simple);
    configureConstrainerForView (simple);

    const auto saved = simple ? processor.getSimpleSize() : processor.getAdvancedSize();
    const int logicalW = simple ? SimpleView::kLogicalW : AdvancedView::kLogicalW;
    const int logicalH = simple ? SimpleView::kLogicalH : AdvancedView::kLogicalH;
    setSize (saved.x > 0 ? saved.x : logicalW,
             saved.y > 0 ? saved.y : logicalH);

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
    advancedView.setPalette (*pal);
    simpleView.setPalette (*pal);
    sendLookAndFeelChange();
    repaint();
}

void MagicDrumDeBleedAudioProcessorEditor::configureConstrainerForView (bool simple)
{
    const int logicalW = simple ? SimpleView::kLogicalW : AdvancedView::kLogicalW;
    const int logicalH = simple ? SimpleView::kLogicalH : AdvancedView::kLogicalH;

    constrainer.setFixedAspectRatio ((double) logicalW / (double) logicalH);
    constrainer.setSizeLimits (juce::roundToInt (logicalW * kMinScale),
                               juce::roundToInt (logicalH * kMinScale),
                               juce::roundToInt (logicalW * kMaxScale),
                               juce::roundToInt (logicalH * kMaxScale));
}

void MagicDrumDeBleedAudioProcessorEditor::setView (bool simple)
{
    processor.setSimpleView (simple);
    advancedView.setVisible (! simple);
    simpleView.setVisible (simple);

    configureConstrainerForView (simple);

    const auto saved = simple ? processor.getSimpleSize() : processor.getAdvancedSize();
    const int logicalW = simple ? SimpleView::kLogicalW : AdvancedView::kLogicalW;
    const int logicalH = simple ? SimpleView::kLogicalH : AdvancedView::kLogicalH;
    setSize (saved.x > 0 ? saved.x : logicalW,
             saved.y > 0 ? saved.y : logicalH);
}

void MagicDrumDeBleedAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal->windowBackground);   // covers any sub-pixel edge from scaling
}

void MagicDrumDeBleedAudioProcessorEditor::resized()
{
    const bool simple = processor.isSimpleView();
    const int logicalW = simple ? SimpleView::kLogicalW : AdvancedView::kLogicalW;
    const int logicalH = simple ? SimpleView::kLogicalH : AdvancedView::kLogicalH;

    const float scale = juce::jmin ((float) getWidth() / (float) logicalW,
                                    (float) getHeight() / (float) logicalH);

    auto layoutView = [&] (juce::Component& view)
    {
        view.setTransform ({});
        view.setBounds (0, 0, logicalW, logicalH);
        view.setTransform (juce::AffineTransform::scale (scale));
    };

    if (simple)
        layoutView (simpleView);
    else
        layoutView (advancedView);

    // Remember this view's size for next time.
    if (simple)
        processor.setSimpleSize (getWidth(), getHeight());
    else
        processor.setAdvancedSize (getWidth(), getHeight());
}
