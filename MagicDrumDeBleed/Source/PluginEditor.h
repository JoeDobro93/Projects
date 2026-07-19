#pragma once
/*  PluginEditor — hosts AdvancedView / SimpleView with FLUID resize:
    no fixed aspect ratio, no scale transform. resized() lays out from
    getLocalBounds(); ui::scale (legibility only) = clamp(min(w/1160,h/830),1,2.2)
    for the advanced view. Views are rebuilt on theme change (all state lives
    in the APVTS). Each view remembers its own size. */
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AdvancedView.h"
#include "UI/SimpleView.h"
#include "Licensing/LicenseConfig.h"

class StyledLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StyledLookAndFeel()   { setPalette (theme::dark()); }
    void setPalette (const theme::Palette& p);
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
};

class MagicDrumDeBleedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc);
    ~MagicDrumDeBleedAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void rebuildViews();
    void setView (bool simple);

    MagicDrumDeBleedAudioProcessor& processor;
    StyledLookAndFeel lookAndFeel;
    std::unique_ptr<AdvancedView> advanced;
    std::unique_ptr<SimpleView> simple;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicDrumDeBleedAudioProcessorEditor)
};
