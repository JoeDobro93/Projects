#pragma once

/*
    OutputStrip — bottom strip: Intensity slider, the "Preview Processed
    Signal" toggle (solo the parallel path, un-flipped, dry muted) and the
    theme toggle button.

    Normal playback is simply the default — it needs no button. The detector
    preview lives in the sidechain section, and the per-stage bypasses live
    in their sections' title rows.
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

    juce::TextButton processedPreviewButton { "Preview Processed Signal" };
    std::unique_ptr<juce::ParameterAttachment> monitorAttachment;

    juce::TextButton themeButton { "Theme" };
    std::function<void()> themeCallback;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStrip)
};
