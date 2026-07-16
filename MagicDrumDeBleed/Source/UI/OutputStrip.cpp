#include "OutputStrip.h"

OutputStrip::OutputStrip (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onThemeToggle)
    : processor (proc), themeCallback (std::move (onThemeToggle))
{
    intensityLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (intensityLabel);

    intensitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    intensitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 18);
    addAndMakeVisible (intensitySlider);
    intensityAttachment = std::make_unique<SliderAttachment> (processor.apvts, ParamIDs::intensity,
                                                              intensitySlider);

    static const char* modeNames[4] = { "Normal", "Sidechain", "Processing", "Delta" };
    for (int i = 0; i < 4; ++i)
    {
        modeButtons[i].setButtonText (modeNames[i]);
        modeButtons[i].setClickingTogglesState (false);
        modeButtons[i].onClick = [this, i]
        {
            if (modeAttachment != nullptr)
                modeAttachment->setValueAsCompleteGesture ((float) i);
        };
        addAndMakeVisible (modeButtons[i]);
    }

    if (auto* modeParam = processor.apvts.getParameter (ParamIDs::monitorMode))
    {
        modeAttachment = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
        {
            const int mode = juce::roundToInt (v);
            for (int i = 0; i < 4; ++i)
                modeButtons[i].setToggleState (i == mode, juce::dontSendNotification);
        });
        modeAttachment->sendInitialUpdate();
    }

    compBypassButton.setClickingTogglesState (true);
    eqBypassButton.setClickingTogglesState (true);
    addAndMakeVisible (compBypassButton);
    addAndMakeVisible (eqBypassButton);
    compBypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, ParamIDs::compBypass,
                                                               compBypassButton);
    eqBypassAttachment   = std::make_unique<ButtonAttachment> (processor.apvts, ParamIDs::eqBypass,
                                                               eqBypassButton);

    themeButton.onClick = [this] { if (themeCallback) themeCallback(); };
    addAndMakeVisible (themeButton);
}

void OutputStrip::setPalette (const theme::Palette& p)
{
    pal = &p;
    themeButton.setButtonText (processor.isDarkTheme() ? "Light" : "Dark");

    compBypassButton.setColour (juce::TextButton::buttonOnColourId, pal->meterPeak.withAlpha (0.8f));
    eqBypassButton.setColour   (juce::TextButton::buttonOnColourId, pal->meterPeak.withAlpha (0.8f));
    repaint();
}

void OutputStrip::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (pal->panelBackground);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void OutputStrip::resized()
{
    auto r = getLocalBounds().reduced (6, 5);

    themeButton.setBounds (r.removeFromRight (juce::jlimit (44, 64, getWidth() / 14)).reduced (0, 3));
    r.removeFromRight (6);

    auto intensityArea = r.removeFromLeft (juce::roundToInt ((float) r.getWidth() * 0.34f));
    intensityLabel.setBounds (intensityArea.removeFromLeft (juce::jlimit (48, 70, intensityArea.getWidth() / 3)));
    intensitySlider.setBounds (intensityArea);
    r.removeFromLeft (8);

    auto bypassArea = r.removeFromRight (juce::roundToInt ((float) r.getWidth() * 0.28f));
    const int halfW = bypassArea.getWidth() / 2;
    compBypassButton.setBounds (bypassArea.removeFromLeft (halfW).reduced (2, 3));
    eqBypassButton.setBounds (bypassArea.reduced (2, 3));

    const int modeW = r.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        modeButtons[i].setBounds (r.getX() + i * modeW, r.getY() + 3, modeW - 3, r.getHeight() - 6);
}
