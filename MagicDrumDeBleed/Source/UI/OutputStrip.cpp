#include "OutputStrip.h"

OutputStrip::OutputStrip (MagicDrumDeBleedAudioProcessor& proc,
                          std::function<void()> onThemeToggle,
                          std::function<void()> onSimpleView)
    : processor (proc), themeCallback (std::move (onThemeToggle)), simpleCallback (std::move (onSimpleView))
{
    // Solo the processed parallel path (compressor + EQ, no polarity flip,
    // dry muted). Off = normal output.
    processedPreviewButton.setClickingTogglesState (false);
    processedPreviewButton.onClick = [this]
    {
        if (monitorAttachment != nullptr)
        {
            const bool active = (int) processor.apvts.getRawParameterValue (ParamIDs::monitorMode)->load()
                                    == MagicDrumDeBleedAudioProcessor::monitorProcessing;
            monitorAttachment->setValueAsCompleteGesture (active
                ? (float) MagicDrumDeBleedAudioProcessor::monitorNormal
                : (float) MagicDrumDeBleedAudioProcessor::monitorProcessing);
        }
    };
    addAndMakeVisible (processedPreviewButton);

    if (auto* modeParam = processor.apvts.getParameter (ParamIDs::monitorMode))
    {
        monitorAttachment = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
        {
            processedPreviewButton.setToggleState (juce::roundToInt (v)
                                                       == MagicDrumDeBleedAudioProcessor::monitorProcessing,
                                                   juce::dontSendNotification);
        });
        monitorAttachment->sendInitialUpdate();
    }

    themeButton.onClick = [this] { if (themeCallback) themeCallback(); };
    addAndMakeVisible (themeButton);

    simpleButton.onClick = [this] { if (simpleCallback) simpleCallback(); };
    addAndMakeVisible (simpleButton);
}

void OutputStrip::setPalette (const theme::Palette& p)
{
    pal = &p;
    themeButton.setButtonText (processor.isDarkTheme() ? "Light" : "Dark");
    processedPreviewButton.setColour (juce::TextButton::buttonOnColourId, pal->buttonOn);
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

    themeButton.setBounds (r.removeFromRight (64).reduced (0, 3));
    r.removeFromRight (4);
    simpleButton.setBounds (r.removeFromRight (68).reduced (0, 3));
    r.removeFromRight (8);

    processedPreviewButton.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 280),
                                                               r.getHeight() - 6));
}
