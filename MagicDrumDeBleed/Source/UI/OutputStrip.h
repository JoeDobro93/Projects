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
    OutputStrip (MagicDrumDeBleedAudioProcessor& proc,
                 std::function<void()> onThemeToggle,
                 std::function<void()> onSimpleView);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::TextButton processedPreviewButton { "Preview Processed Signal" };
    std::unique_ptr<juce::ParameterAttachment> monitorAttachment;

    juce::TextButton themeButton { "Theme" };
    juce::TextButton simpleButton { "Simple" };
    std::function<void()> themeCallback;
    std::function<void()> simpleCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStrip)
};
