#pragma once

/*
    PluginEditor — top-level layout, resizing, theming.

    Layout (all proportional — no hardcoded pixel positions):
        ┌────────────────────────────────────────────┐
        │ header: title + preset browser             │
        ├────────────────────────────────────────────┤
        │ CompressorPanel (controls, SC, GR meter)   │
        ├────────────────────────────────────────────┤
        │ EQPanel (spectrum + bands)                 │
        ├────────────────────────────────────────────┤
        │ OutputStrip (intensity, monitors, theme)   │
        └────────────────────────────────────────────┘

    Default 820×520, minimum 650×400, freeform scaling above that.
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ThemeColors.h"
#include "UI/CompressorPanel.h"
#include "UI/EQPanel.h"
#include "UI/OutputStrip.h"
#include "UI/PresetBrowser.h"

//==============================================================================
class StyledLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StyledLookAndFeel()   { setPalette (theme::dark()); }

    void setPalette (const theme::Palette& p);

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

private:
    const theme::Palette* pal = &theme::dark();
};

//==============================================================================
class MagicDrumDeBleedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc);
    ~MagicDrumDeBleedAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void applyTheme();
    void toggleTheme();

    MagicDrumDeBleedAudioProcessor& processor;
    StyledLookAndFeel lookAndFeel;
    const theme::Palette* pal = &theme::dark();

    juce::Label titleLabel { {}, "MAGIC DRUM DE-BLEED" };
    PresetBrowser presetBrowser;
    CompressorPanel compressorPanel;
    EQPanel eqPanel;
    OutputStrip outputStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicDrumDeBleedAudioProcessorEditor)
};
