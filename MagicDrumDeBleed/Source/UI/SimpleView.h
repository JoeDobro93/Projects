#pragma once

/*
    SimpleView — a compact monitoring view: the input-level meter (with its
    draggable threshold line) and the gain-reduction meter, a vertical
    Intensity slider to their right (100 % at the top), and a button back to
    the Advanced view. Laid out at a fixed logical size and scaled uniformly
    by the editor.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "CompressorPanel.h"   // InputLevelMeter, GainReductionMeter

class SimpleView : public juce::Component
{
public:
    static constexpr int kLogicalW = 300;
    static constexpr int kLogicalH = 380;

    SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    InputLevelMeter    inputMeter;
    GainReductionMeter grMeter;

    juce::Label  intensityLabel { {}, "Intensity" };
    juce::Slider intensitySlider;
    juce::TextButton advancedButton { "Advanced View" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleView)
};
