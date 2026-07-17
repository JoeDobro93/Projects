#pragma once

/*
    EQPanel — bottom section of the UI: the processed-signal spectrum display
    (with Accumulate / Freeze and an Input/Output-levels tap switch), the
    combined EQ response curve, draggable band handles, the band row (select +
    enable + solo per band) and the selected-band control strip.

    Band indices everywhere in this file: 0=HPF, 1=LPF, 2..6=Notch 1..5,
    matching mdd::EQProcessor and theme::Palette::bandColours.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "UIHelpers.h"
#include "CompressorPanel.h"   // GainReductionMeter (reused for the EQ gate)

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
    // Log-frequency mapping shared by the spectrum and the overlay:
    // the classic 20 Hz .. 20 kHz EQ axis, 20 kHz at the right edge.
    inline constexpr float kMinHz = 20.0f, kMaxHz = 20000.0f;

    inline float freqToX (float freq, juce::Rectangle<float> area)
    {
        const float t = std::log (juce::jlimit (kMinHz, kMaxHz, freq) / kMinHz) / std::log (kMaxHz / kMinHz);
        return area.getX() + t * area.getWidth();
    }

    inline float xToFreq (float x, juce::Rectangle<float> area)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
        return kMinHz * std::pow (kMaxHz / kMinHz, t);
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

    std::vector<float> sampleFifo;
    std::vector<float> fftData;
    std::vector<float> smoothedDb;
    std::vector<float> accumDb;
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
    void drawEqCurve (juce::Graphics& g, juce::Rectangle<float> area) const;

    MagicDrumDeBleedAudioProcessor& processor;
    std::function<void (int)> bandSelectedCallback;

    juce::RangedAudioParameter* onParams[7]    {};
    juce::RangedAudioParameter* freqParams[7]  {};
    juce::RangedAudioParameter* qParams[7]     {};
    juce::RangedAudioParameter* gainParams[7]  {};
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
    ~EQPanel() override;   // clears any active band solo

    void setPalette (const theme::Palette& p);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void selectBand (int band);
    void rebuildAttachments();
    void updateBandButtonColours();
    void updateShapeSlopeControls();
    void setShape (int shapeIndex);
    void updateSoloButtons();

    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::Label titleLabel { {}, "EQ - PROCESSED SIGNAL" };
    juce::ToggleButton bypassButton { "Bypass" };   // anchored bottom-left

    SpectrumDisplay spectrum;
    BandOverlay overlay;

    // Post EQ Gate: blends the parallel path from EQ'd (drum sounding) to
    // raw inverted-cancelling (silence) using the shared detector.
    juce::Label gateLabel { {}, "POST EQ GATE" };
    juce::ToggleButton gateBypassButton { "Bypass" };
    ui::LabelledKnob gateHold { "Hold" }, gateRelease { "Release" };
    GainReductionMeter gateMeter;
    ui::SlideSwitch levelsSwitch { "Input", "Output", {} };
    juce::TextButton accumulateButton { "Accumulate" };
    juce::TextButton freezeButton     { "Freeze" };

    juce::TextButton bandButtons[7];
    juce::TextButton enableButtons[7];
    juce::TextButton soloButtons[7];
    std::unique_ptr<juce::ParameterAttachment> bandOnAttachments[7];

    ui::LabelledKnob freqKnob { "Freq" }, gainKnob { "Gain" }, qKnob { "Q" };

    // Notch shape: three curve-icon buttons. HPF/LPF: two stacked slope buttons.
    ui::CurveIconButton shapeButtons[3];
    juce::TextButton slopeButtons[2];   // "6 dB/oct", "12 dB/oct"

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> enableAttachments[7];
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::ParameterAttachment> gateBypassAttachment;   // inverted: checked = gate off
    std::unique_ptr<SliderAttachment> gateHoldAttachment, gateReleaseAttachment;
    std::unique_ptr<SliderAttachment> freqAttachment, gainAttachment, qAttachment;
    std::unique_ptr<juce::ParameterAttachment> shapeAttachment;

    int selectedBand = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPanel)
};
