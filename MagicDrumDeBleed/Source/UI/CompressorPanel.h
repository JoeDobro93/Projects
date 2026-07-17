#pragma once

/*
    CompressorPanel — top section of the UI: transient-detection (gate)
    controls, the sidechain sub-section (filter, Learn, detector Preview)
    and, on the left, the input-level and gain-reduction meters.

    The input meter shows the detector RMS level — the exact quantity the
    Threshold is compared against — with a threshold marker line that can be
    dragged directly to set the Threshold, plus a lingering peak-hold line.

    InputLevelMeter and GainReductionMeter are reused by the Simple view, so
    they live here as standalone components.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "UIHelpers.h"

//==============================================================================
class InputLevelMeter : public juce::Component, private juce::Timer
{
public:
    // thresholdParam may be null for a read-only meter; when supplied the
    // threshold line becomes click/drag editable.
    InputLevelMeter (juce::String captionText, std::function<float()> levelGetter,
                     juce::RangedAudioParameter* thresholdParam);

    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    juce::MouseCursor getMouseCursor() override;

private:
    void timerCallback() override;
    juce::Rectangle<float> barArea() const;
    float yToDb (float y) const;
    float dbToY (float db) const;
    void  setThresholdFromMouse (float y);

    static constexpr float minDb = -60.0f, maxDb = 0.0f;

    juce::String caption;
    std::function<float()> getLevel;
    juce::RangedAudioParameter* threshold = nullptr;
    bool dragging = false;

    float displayedDb = -90.0f;
    float peakDb = -90.0f;
    int peakHoldFrames = 0;
    const theme::Palette* pal = &theme::dark();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputLevelMeter)
};

//==============================================================================
class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    GainReductionMeter (juce::String captionText, std::function<float()> valueGetter);
    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::String caption;
    std::function<float()> getValue;
    float displayedDb = 0.0f;
    const theme::Palette* pal = &theme::dark();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};

//==============================================================================
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

    juce::ToggleButton bypassButton   { "Bypass" };     // bottom-left of the compressor segment
    juce::ToggleButton scEnableButton { "Filter On" };  // bottom-left of the sidechain segment

    ui::LabelledKnob threshold { "Threshold" }, reduction { "Reduction" }, lookahead { "Lookahead" },
                     rmsWindow { "RMS" },       hold      { "Hold" },      release   { "Release" },
                     scFreq    { "SC Freq" },   scQ       { "SC Q" };

    juce::TextButton   learnButton     { "Learn" };
    juce::TextButton   scPreviewButton { "Preview" };   // solo the detector signal

    InputLevelMeter inputMeter;
    GainReductionMeter grMeter;

    // divider x, remembered from resized() for the paint() separator
    int sidechainDividerX = 0;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> scEnableAttachment, bypassAttachment;
    std::unique_ptr<juce::ParameterAttachment> monitorAttachment;   // drives the Preview toggle

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorPanel)
};
