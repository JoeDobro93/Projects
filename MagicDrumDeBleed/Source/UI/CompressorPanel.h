#pragma once

/*
    CompressorPanel — top section of the UI: gate/compressor controls, the
    sidechain sub-section (filter, Learn) and the gain-reduction meter.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "UIHelpers.h"

class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    explicit GainReductionMeter (std::function<float()> valueGetter);
    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    std::function<float()> getValue;
    float displayedDb = 0.0f;
    const theme::Palette* pal = &theme::dark();
};

class CompressorPanel : public juce::Component
{
public:
    explicit CompressorPanel (MagicDrumDeBleedAudioProcessor& proc);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void learnClicked();

    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::Label titleLabel   { {}, "COMPRESSOR" };
    juce::Label scTitleLabel { {}, "SIDECHAIN" };

    ui::LabelledKnob threshold { "Threshold" }, reduction { "Reduction" }, lookahead { "Lookahead" },
                     rmsWindow { "RMS" },       hold      { "Hold" },      release   { "Release" },
                     scFreq    { "SC Freq" },   scQ       { "SC Q" },      ceiling   { "Ceiling" };

    juce::ToggleButton scEnableButton { "Filter On" };
    juce::TextButton   learnButton    { "Learn" };

    GainReductionMeter meter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> scEnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorPanel)
};
