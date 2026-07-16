#pragma once

/*
    EQPanel — bottom section of the UI: the parallel-path spectrum display
    (with Accumulate and Freeze modes), draggable band handles, band
    selection row and the per-band control strip.

    Band indices everywhere in this file: 0=HPF, 1=LPF, 2..6=Notch 1..5,
    matching mdd::EQProcessor and theme::Palette::bandColours.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "UIHelpers.h"

namespace eqids
{
    juce::String onId    (int band);
    juce::String freqId  (int band);
    juce::String qId     (int band);   // notches only
    juce::String gainId  (int band);   // notches only
    juce::String shapeId (int band);   // slope for HPF/LPF, shape for notches
}

namespace eqmap
{
    // Log-frequency mapping (20 Hz .. 20 kHz) shared by spectrum and overlay.
    inline float freqToX (float freq, juce::Rectangle<float> area)
    {
        const float t = std::log (juce::jlimit (20.0f, 20000.0f, freq) / 20.0f) / std::log (1000.0f);
        return area.getX() + t * area.getWidth();
    }

    inline float xToFreq (float x, juce::Rectangle<float> area)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
        return 20.0f * std::pow (1000.0f, t);
    }
}

//==============================================================================
class SpectrumDisplay : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumDisplay (MagicDrumDeBleedAudioProcessor& proc);

    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void setAccumulate (bool shouldAccumulate);
    void setFrozen (bool shouldFreeze);

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    void runFFT();

    static constexpr int kFftOrder = 11;
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kNumBins  = kFftSize / 2;

    MagicDrumDeBleedAudioProcessor& processor;
    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kFftSize,
                                                 juce::dsp::WindowingFunction<float>::hann };

    std::vector<float> sampleFifo;   // gathers samples until a full FFT frame
    std::vector<float> fftData;
    std::vector<float> smoothedDb;   // live display
    std::vector<float> accumDb;      // peak-hold layer
    float pullBuffer[512];
    int fifoIndex = 0;

    bool accumulate = false, frozen = false;
    const theme::Palette* pal = &theme::dark();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};

//==============================================================================
class BandOverlay : public juce::Component, private juce::Timer
{
public:
    BandOverlay (MagicDrumDeBleedAudioProcessor& proc, std::function<void (int)> onBandSelected);

    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void setSelectedBand (int band)             { selectedBand = band; repaint(); }

    void paint (juce::Graphics& g) override;
    void mouseDown  (const juce::MouseEvent& e) override;
    void mouseDrag  (const juce::MouseEvent& e) override;
    void mouseUp    (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    void timerCallback() override   { repaint(); }

    struct BandState { bool enabled; float freq, q, gainDb; int shape; };
    BandState getBand (int band) const;
    juce::Point<float> handlePosition (int band, const BandState& s, juce::Rectangle<float> area) const;
    int findBandAt (juce::Point<float> pos) const;

    MagicDrumDeBleedAudioProcessor& processor;
    std::function<void (int)> bandSelectedCallback;

    juce::RangedAudioParameter* onParams[7]    {};
    juce::RangedAudioParameter* freqParams[7]  {};
    juce::RangedAudioParameter* qParams[7]     {};   // null for HPF/LPF
    juce::RangedAudioParameter* gainParams[7]  {};   // null for HPF/LPF
    juce::RangedAudioParameter* shapeParams[7] {};

    int selectedBand = 0;
    int draggedBand = -1;
    const theme::Palette* pal = &theme::dark();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandOverlay)
};

//==============================================================================
class EQPanel : public juce::Component
{
public:
    explicit EQPanel (MagicDrumDeBleedAudioProcessor& proc);

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void selectBand (int band);
    void rebuildAttachments();
    void updateBandButtonColours();
    void updateShapeButtonText();

    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::Label titleLabel { {}, "EQ — PARALLEL PATH" };

    SpectrumDisplay spectrum;
    BandOverlay overlay;
    juce::TextButton accumulateButton { "Accumulate" };
    juce::TextButton freezeButton     { "Freeze" };

    juce::TextButton bandButtons[7];
    std::unique_ptr<juce::ParameterAttachment> bandOnAttachments[7];

    // Per-band control strip (attachments rebuilt when the selection changes)
    juce::ToggleButton onButton { "On" };
    ui::LabelledKnob freqKnob { "Freq" }, gainKnob { "Gain" }, qKnob { "Q" };
    juce::TextButton shapeButton;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> freqAttachment, gainAttachment, qAttachment;
    std::unique_ptr<ButtonAttachment> onAttachment;
    std::unique_ptr<juce::ParameterAttachment> shapeAttachment;

    int selectedBand = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPanel)
};
