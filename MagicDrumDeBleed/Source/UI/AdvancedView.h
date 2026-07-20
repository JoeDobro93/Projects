#pragma once
/*  AdvancedView — header (brand/presets/theme), the three stage panels, the
    right rail and the hint bar. Fluid layout: stages 1/2 fixed height, stage 3
    stretches; rail and bars fixed; ui::scale affects legibility only. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Stages.h"
#include "PresetBrowser.h"

class AdvancedView : public juce::Component
{
public:
    static constexpr int kDefaultW = 1200, kDefaultH = 830;
    static constexpr int kMinW = 940,  kMinH = 750;

    AdvancedView (MagicDrumDeBleedAudioProcessor& proc,
                  std::function<void()> onThemeToggle,
                  std::function<void()> onSimpleView);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    PresetBrowser presetBrowser;
    juce::TextButton themeBtn;
    TriggerStage trigger;
    GateStage    gate;
    TailStage    tail;
    RightRail    rail;
    HintBar      hintBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedView)
};
