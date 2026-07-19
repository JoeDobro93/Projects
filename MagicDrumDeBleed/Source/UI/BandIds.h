#pragma once
/*  Band → parameter-ID mapping. Index 0=LOWS(hpf) 1=HIGHS(lpf) 2..6=K1..K5. */
#include "../PluginProcessor.h"

namespace eqids
{
inline juce::String onId (int b)    { return b == 0 ? ParamIDs::hpfOn   : b == 1 ? ParamIDs::lpfOn   : ParamIDs::notchOn (b - 2); }
inline juce::String freqId (int b)  { return b == 0 ? ParamIDs::hpfFreq : b == 1 ? ParamIDs::lpfFreq : ParamIDs::notchFreq (b - 2); }
inline juce::String qId (int b)     { return b >= 2 ? ParamIDs::notchQ (b - 2)    : juce::String(); }
inline juce::String gainId (int b)  { return b >= 2 ? ParamIDs::notchGain (b - 2) : juce::String(); }
inline juce::String shapeId (int b) { return b == 0 ? ParamIDs::hpfSlope : b == 1 ? ParamIDs::lpfSlope : ParamIDs::notchShape (b - 2); }

/*  Two-phase Learn click, shared by the Trigger stage and the Simple view.
    First click starts capture; second click analyses, sets Focus, and — when
    Link to K1 is on — updates K1 (enabling it at ring level −3 dB if it was
    off; −3 dB ring ⇒ internal cut of 20·log10(1 − 10^(−3/20)) ≈ −10.69 dB). */
inline void handleLearnClick (MagicDrumDeBleedAudioProcessor& proc, juce::TextButton& btn)
{
    if (! proc.isLearning())
    {
        proc.startLearn();
        btn.setButtonText (juce::String::fromUTF8 ("listening\xe2\x80\xa6"));
        return;
    }
    const double f = proc.finishLearnAndAnalyse();
    btn.setButtonText ("Learn");
    if (f <= 0.0) return;

    auto set = [&proc] (const juce::String& id, float real)
    {
        if (auto* p = proc.apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (real));
            p->endChangeGesture();
        }
    };
    set (ParamIDs::scFreq, (float) f);

    auto* link = proc.apvts.getParameter (ParamIDs::linkK1);
    if (link != nullptr && link->getValue() > 0.5f)
    {
        auto* on = proc.apvts.getParameter (ParamIDs::notchOn (0));
        const bool wasOn = on != nullptr && on->getValue() > 0.5f;
        set (ParamIDs::notchFreq (0), (float) f);
        if (! wasOn)
        {
            set (ParamIDs::notchOn (0), 1.0f);
            set (ParamIDs::notchGain (0), -10.69f);
        }
    }
}
}
