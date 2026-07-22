#pragma once
/*  Band → parameter-ID mapping. Index 0=LP(hpf) 1=HP(lpf) 2..6=K1..K5.
    Also hosts the shared Learn-click handler and factory-preset applier. */
#include "../PluginProcessor.h"
#include "../PresetDefaults.h"
#include "Widgets.h"

namespace eqids
{
inline juce::String onId (int b)    { return b == 0 ? ParamIDs::hpfOn   : b == 1 ? ParamIDs::lpfOn   : ParamIDs::notchOn (b - 2); }
inline juce::String freqId (int b)  { return b == 0 ? ParamIDs::hpfFreq : b == 1 ? ParamIDs::lpfFreq : ParamIDs::notchFreq (b - 2); }
inline juce::String qId (int b)     { return b >= 2 ? ParamIDs::notchQ (b - 2)    : juce::String(); }
inline juce::String gainId (int b)  { return b >= 2 ? ParamIDs::notchGain (b - 2) : juce::String(); }
inline juce::String shapeId (int b) { return b == 0 ? ParamIDs::hpfSlope : b == 1 ? ParamIDs::lpfSlope : ParamIDs::notchShape (b - 2); }

inline void setRealValue (MagicDrumDeBleedAudioProcessor& proc, const juce::String& id, float real)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (real));
        p->endChangeGesture();
    }
}

/*  Learn button, shared by the Trigger stage and the Simple view. First click
    starts capture (green, auto-finishes after 3 s); the second click — or the
    timeout — analyses, sets Focus, and with Link to K1 on updates K1
    (enabling it at ring 4.5 ≈ −3 dB ring level if it was off). */
class LearnButton : public juce::TextButton, private juce::Timer
{
public:
    explicit LearnButton (MagicDrumDeBleedAudioProcessor& p) : juce::TextButton ("Learn"), proc (p)
    {
        onClick = [this] { proc.isLearning() ? finish() : begin(); };
        applyColours (false);
    }

private:
    void begin()
    {
        proc.startLearn();
        setButtonText (juce::String::fromUTF8 ("listening\xe2\x80\xa6"));
        applyColours (true);
        startTimer (3000);                        // auto-stop
    }

    void finish()
    {
        stopTimer();
        const double f = proc.finishLearnAndAnalyse();
        setButtonText ("Learn");
        applyColours (false);
        if (f <= 0.0) return;

        setRealValue (proc, ParamIDs::scFreq, (float) f);
        auto* link = proc.apvts.getParameter (ParamIDs::linkK1);
        if (link != nullptr && link->getValue() > 0.5f)
        {
            auto* on = proc.apvts.getParameter (ParamIDs::notchOn (0));
            const bool wasOn = on != nullptr && on->getValue() > 0.5f;
            setRealValue (proc, ParamIDs::notchFreq (0), (float) f);
            if (! wasOn)
            {
                setRealValue (proc, ParamIDs::notchOn (0), 1.0f);
                setRealValue (proc, ParamIDs::notchGain (0), 4.5f);   // ring ≈ −3 dB
            }
        }
    }

    void timerCallback() override   { if (proc.isLearning()) finish(); else stopTimer(); }

    void applyColours (bool listening)
    {
        const auto c = listening ? ui::pal->open : ui::pal->accent.brighter (0.15f);
        setColour (juce::TextButton::buttonColourId, c);
        setColour (juce::TextButton::textColourOffId, ui::pal->bg);
        repaint();
    }

    MagicDrumDeBleedAudioProcessor& proc;
};

/*  Apply a factory preset: reset everything except the per-mic calibration
    (Threshold, Selectivity, Hysteresis, MIDI trigger — those depend on the
    track, not the drum), then set the preset's values. Shared by the
    PresetBrowser and the Simple view. */
inline void applyFactoryPreset (MagicDrumDeBleedAudioProcessor& proc, const presets::FactoryPreset& pr)
{
    static const juce::StringArray preserved { ParamIDs::threshold, ParamIDs::contrast,
                                               ParamIDs::hysteresis, ParamIDs::midiTrigger,
                                               ParamIDs::compMode, ParamIDs::compRatio };
    for (auto* p : proc.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! preserved.contains (ranged->paramID))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

    setRealValue (proc, ParamIDs::scFreq,        pr.scFreqHz);
    setRealValue (proc, ParamIDs::scQ,           pr.scQ);
    setRealValue (proc, ParamIDs::lookahead,     (float) pr.lookaheadMs);
    setRealValue (proc, ParamIDs::hold,          pr.holdMs);
    setRealValue (proc, ParamIDs::release,       pr.releaseMs);
    setRealValue (proc, ParamIDs::notchOn (0),   pr.k1On ? 1.0f : 0.0f);
    setRealValue (proc, ParamIDs::notchFreq (0), pr.k1FreqHz);
    setRealValue (proc, ParamIDs::notchQ (0),    pr.k1Q);
    setRealValue (proc, ParamIDs::notchGain (0), pr.k1Ring);
    setRealValue (proc, ParamIDs::eqGateHold,    pr.tailHoldMs);
    setRealValue (proc, ParamIDs::eqGateRelease, pr.tailFadeMs);
    setRealValue (proc, ParamIDs::compAttack,    pr.compAttackMs);
    setRealValue (proc, ParamIDs::compRelease,   pr.compReleaseMs);
}
}
