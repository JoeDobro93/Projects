#pragma once

/*
    CompressorPanel — top section of the UI: transient-detection (gate)
    controls, the sidechain sub-section (filter, Learn, detector Preview)
    and, on the left, the input-level and gain-reduction meters.

    The input meter shows the detector RMS level — the exact quantity the
    Threshold is compared against — with a threshold marker line and a
    lingering peak-hold line.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "UIHelpers.h"

class InputLevelMeter : public juce::Component, private juce::Timer
{
public:
    InputLevelMeter (std::function<float()> levelGetter, std::function<float()> thresholdGetter);
    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    std::function<float()> getLevel, getThreshold;
    float displayedDb = -90.0f;
    float peakDb = -90.0f;
    int peakHoldFrames = 0;
    const theme::Palette* pal = &theme::dark();
};

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

    juce::Label titleLabel   { {}, "TRANSIENT DETECTION" };
    juce::Label scTitleLabel { {}, "SIDECHAIN" };

    juce::ToggleButton bypassButton { "Bypass" };

    ui::LabelledKnob threshold { "Threshold" }, reduction { "Reduction" }, lookahead { "Lookahead" },
                     rmsWindow { "RMS" },       hold      { "Hold" },      release   { "Release" },
                     scFreq    { "SC Freq" },   scQ       { "SC Q" };

    juce::ToggleButton scEnableButton  { "Filter On" };
    juce::TextButton   learnButton     { "Learn" };
    juce::TextButton   scPreviewButton { "Preview" };   // solo the detector signal

    InputLevelMeter inputMeter;
    GainReductionMeter grMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> scEnableAttachment, bypassAttachment;
    std::unique_ptr<juce::ParameterAttachment> monitorAttachment;   // drives the Preview toggle

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorPanel)
};
