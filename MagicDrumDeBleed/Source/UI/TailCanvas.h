#pragma once
/*  TailCanvas — the stage-3 display. Layers (back to front):
      blue  = dry input spectrum (from the processor FIFO, real FFT)
      orange= dry + 20·log10|1−H|  (what survives the EQ-only cancellation)
      gold  = the internal cancellation curve H mirrored vertically, carrying
              the band handles (peaks up where the internal filter cuts)
    H is the complex product of every enabled band's stages, computed from the
    same EQProcessor::computeCoefficients the audio path runs.
    The gold curve/handles use a cut-depth axis: 0 dB cut = the bottom line,
    dispMax (12/18/24, the Scale selector) = the top; deeper tips clip
    offscreen but dragging above the canvas still works to the ring max.
    Spectrum layers keep the +9..−54 dB axis. Drag: x=freq, y=ring; wheel=Q.
    The dry/kept legend chips toggle their layers; monitor gain offsets both. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"

class TailCanvas : public juce::Component, private juce::Timer
{
public:
    TailCanvas (MagicDrumDeBleedAudioProcessor& proc, std::function<void (int)> onBandSelected);

    void setSelectedBand (int b)      { sel = b; repaint(); }
    void setShowInternals (bool b)    { internals = b; repaint(); }
    void setAccumulate (bool b);
    void setFrozen (bool b)           { frozen = b; }
    void setMonitorGain (float db)    { monGainDb = db; repaint(); }
    void setDisplayScale (float maxCutDb)   { dispMax = maxCutDb; repaint(); }   // 12/18/24
    // Lock freq: handle drags change only the ring level (and wheel the Q);
    // frequency stays put — knobs still move it.
    void setFreqLock (bool b)         { freqLock = b; }

    // Broadband average of |1 − amount·H| — the keepAudible() feed (spec §3).
    static double keepAvg (MagicDrumDeBleedAudioProcessor& proc, double amount);

    // Shared OPEN/TAIL/CLOSED state (0 closed, 1 tail, 2 open) — used by the
    // Gate stage chip, the rail GATE meter and the Simple view.
    static int gateState (MagicDrumDeBleedAudioProcessor& proc, float& open01, float& tail01);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr float kTopDb = 9.0f, kBotDb = -54.0f;

private:
    void timerCallback() override;
    void runFFT();
    void recomputeCurve();
    void buildGoldCurve();                    // adaptive grid: exact band tips
    double hDbAt (double f) const;
    int  bandAt (juce::Point<float> pos) const;
    juce::Rectangle<float> plotArea() const;
    float fx (double f, float w) const;
    double xf (float x, float w) const;
    float dyy (float db, float h) const;

    MagicDrumDeBleedAudioProcessor& processor;
    std::function<void (int)> onBandSelected;

    // band params, index 0=LP(hpf) 1=HP(lpf) 2..6=K1..5
    juce::RangedAudioParameter *onP[7] {}, *freqP[7] {}, *qP[7] {}, *gainP[7] {}, *shapeP[7] {};

    static constexpr int kFftOrder = 11, kFftSize = 1 << kFftOrder, kBins = kFftSize / 2;
    static constexpr int kN = 240;
    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kFftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> sampleFifo, fftData, smoothedDb, accumDb;
    float pull[512];
    int fifoIdx = 0;
    bool accumulate = false, frozen = false, internals = false;
    bool showDry = true, showKept = true;                 // legend toggles
    float monGainDb = 0.0f;                               // display-only spectrum offset
    bool freqLock = false;
    float dispMax = 24.0f;                                // gold-curve axis: cut depth 0..dispMax
    juce::Rectangle<int> dryLegend, keptLegend;           // clickable legend chips

    double keepDb[kN], hDb[kN], freqs[kN];
    std::vector<double> curveF, curveHdB;     // gold/internals grid (base + band tips)
    float handleX[7] {}, handleY[7] {};
    int sel = 0, drag = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TailCanvas)
};
