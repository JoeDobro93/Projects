#pragma once

/*
    AdvancedView — the full plugin UI (header + presets, transient-detection
    panel, EQ panel, output strip). It is laid out at a fixed logical size;
    the editor applies a uniform scale transform so proportions never change.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "CompressorPanel.h"
#include "EQPanel.h"
#include "OutputStrip.h"
#include "PresetBrowser.h"

class AdvancedView : public juce::Component
{
public:
    // Logical design size (scale == 1). Proportions are fixed at this ratio.
    static constexpr int kLogicalW = 1000;
    static constexpr int kLogicalH = 680;

    AdvancedView (MagicDrumDeBleedAudioProcessor& proc,
                  std::function<void()> onThemeToggle,
                  std::function<void()> onSimpleView);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    const theme::Palette* pal = &theme::dark();

    juce::Label titleLabel { {}, "MAGIC DRUM DE-BLEED" };
    PresetBrowser   presetBrowser;
    CompressorPanel compressorPanel;
    EQPanel         eqPanel;
    OutputStrip     outputStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedView)
};
