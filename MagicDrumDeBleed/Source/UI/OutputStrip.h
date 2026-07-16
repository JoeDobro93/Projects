#pragma once

/*
    OutputStrip — bottom strip: Intensity slider, the four mutually
    exclusive monitoring-mode buttons, compressor/EQ bypass toggles and the
    theme toggle button.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"

class OutputStrip : public juce::Component
{
public:
    OutputStrip (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onThemeToggle);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::Label  intensityLabel { {}, "Intensity" };
    juce::Slider intensitySlider;

    juce::TextButton modeButtons[4];
    std::unique_ptr<juce::ParameterAttachment> modeAttachment;

    juce::TextButton compBypassButton { "Comp Byp" };
    juce::TextButton eqBypassButton   { "EQ Byp" };
    juce::TextButton themeButton      { "Theme" };
    std::function<void()> themeCallback;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment;
    std::unique_ptr<ButtonAttachment> compBypassAttachment, eqBypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStrip)
};
