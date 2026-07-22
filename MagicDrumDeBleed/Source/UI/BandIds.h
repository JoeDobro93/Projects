#pragma once
/*  Band → parameter-ID mapping. Index 0=LP(hpf) 1=HP(lpf) 2..6=K1..K5.
    Also hosts the shared Learn-click handler and factory-preset applier. */
#include "../PluginProcessor.h"
#include "../PresetDefaults.h"
#include "Widgets.h"
#include <array>

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

/*  Resonance slot model. Slot 0 (K1) = the fundamental; slots 1..4
    (K2..K5) = the loudest remaining resonances ABOVE the fundamental in
    loudness order. ENABLED K bands other than `selfBand` PIN their centres
    into their slots — hit-to-hit variation can't shuffle a neighbour's
    resonance onto the band being learned, and detected peaks near a pinned
    centre are absorbed by it. selfBand −1 = learn-all (no pinning, fresh
    assignment). Unfillable slots return 0. */
inline std::array<double, 5> assignResonanceSlots (MagicDrumDeBleedAudioProcessor& proc,
                                                   const std::vector<mdd::LearnAnalyzer::Resonance>& res,
                                                   int selfBand)
{
    std::array<double, 5> slot {};
    if (res.empty())
        return slot;

    auto spacing = [] (double f) { return juce::jmax (12.0, 0.06 * f); };
    if (selfBand >= 0)
        for (int b = 0; b < 5; ++b)
            if (b != selfBand)
                if (auto* on = proc.apvts.getRawParameterValue (ParamIDs::notchOn (b));
                    on != nullptr && on->load() > 0.5f)
                    slot[(size_t) b] = (double) proc.apvts.getRawParameterValue (ParamIDs::notchFreq (b))->load();

    if (slot[0] <= 0.0)
        slot[0] = res[0].hz;                       // res is loudest-first
    const double f0 = slot[0];

    for (const auto& r : res)                      // level order fills K2 first
    {
        if (r.hz <= f0 + spacing (f0))
            continue;                              // the fundamental region itself
        bool taken = false;
        for (double s : slot)
            if (s > 0.0 && std::abs (r.hz - s) < spacing (s))
            {
                taken = true;                      // a pinned centre owns this one
                break;
            }
        if (taken)
            continue;
        for (size_t i = 1; i < slot.size(); ++i)
            if (slot[i] <= 0.0)
            {
                slot[i] = r.hz;
                break;
            }
    }
    return slot;
}

/*  Turn band b (0..4 = K1..K5) into a learned resonance keeper: enabled,
    centred on hz, Q 10, ring 7 (K1: ring 10 — the fundamental is the main
    keeper). Setting K1's frequency mirrors into Focus when Link is on. */
inline void applyResonanceToBand (MagicDrumDeBleedAudioProcessor& proc, int b, double hz)
{
    setRealValue (proc, ParamIDs::notchFreq (b), (float) hz);
    setRealValue (proc, ParamIDs::notchQ (b), 10.0f);
    setRealValue (proc, ParamIDs::notchGain (b), b == 0 ? 10.0f : 7.0f);
    setRealValue (proc, ParamIDs::notchOn (b), 1.0f);
}

/*  Learn button, shared by the Trigger stage, the Simple view and the Tail
    stage's "Learn all". First click starts capture (green, auto-finishes
    after 3 s); the second click — or the timeout — analyses.
    learnAllBands == false (Trigger): sets Focus, and with Link to K1 on
    updates K1 (enabling it at ring 4.5 ≈ −3 dB ring level if it was off).
    learnAllBands == true: assigns EVERY resonance slot it can fill —
    fundamental → K1 (+ Focus), then the loudest above-fundamental
    resonances → K2.. as far as they go; bands with no detected slot are
    left untouched. Flashes red briefly when nothing was detected. */
class LearnButton : public juce::TextButton, private juce::Timer
{
public:
    explicit LearnButton (MagicDrumDeBleedAudioProcessor& p, bool learnAllBands = false)
        : juce::TextButton ("Learn"), proc (p), learnAll (learnAllBands)
    {
        onClick = [this] { proc.isLearning() ? finish() : begin(); };
        applyColours (false);
    }

    void setIdleText (const juce::String& t)  { idleText = t; setButtonText (t); }

private:
    void begin()
    {
        flashing = false;
        proc.startLearn();
        setButtonText (juce::String::fromUTF8 ("listening\xe2\x80\xa6"));
        applyColours (true);
        startTimer (3000);                        // auto-stop
    }

    void finish()
    {
        stopTimer();
        setButtonText (idleText);
        applyColours (false);

        if (! learnAll)
        {
            const double f = proc.finishLearnAndAnalyse();
            if (f <= 0.0) { fail(); return; }

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
            return;
        }

        const auto res = proc.finishLearnAndAnalyseResonances();
        const auto slot = assignResonanceSlots (proc, res, -1);
        if (slot[0] <= 0.0) { fail(); return; }

        setRealValue (proc, ParamIDs::scFreq, (float) slot[0]);
        for (int b = 0; b < 5; ++b)
            if (slot[(size_t) b] > 0.0)
                applyResonanceToBand (proc, b, slot[(size_t) b]);
    }

    void fail()
    {
        flashing = true;
        const auto c = ui::pal->warn;
        setColour (juce::TextButton::buttonColourId, c);
        setColour (juce::TextButton::textColourOffId, ui::pal->bg);
        repaint();
        startTimer (900);
    }

    void timerCallback() override
    {
        if (proc.isLearning()) { finish(); return; }
        stopTimer();
        if (flashing) { flashing = false; applyColours (false); }
    }

    void applyColours (bool listening)
    {
        const auto c = listening ? ui::pal->open : ui::pal->accent.brighter (0.15f);
        setColour (juce::TextButton::buttonColourId, c);
        setColour (juce::TextButton::textColourOffId, ui::pal->bg);
        repaint();
    }

    MagicDrumDeBleedAudioProcessor& proc;
    juce::String idleText { "Learn" };
    bool learnAll = false;
    bool flashing = false;
};

/*  Per-band Learn (under each K band's solo button): listens, then assigns
    THIS band's resonance slot — K1 the fundamental, K2 the next loudest
    above it, and so on — pinning the other enabled bands' centres so the
    ranking stays stable between hits. Flashes red if that slot could not
    be detected (band is left untouched, not enabled). */
class BandLearnButton : public juce::TextButton, private juce::Timer
{
public:
    BandLearnButton (MagicDrumDeBleedAudioProcessor& p, int kBand)
        : juce::TextButton ("Learn"), proc (p), band (kBand)
    {
        onClick = [this] { proc.isLearning() ? finish() : begin(); };
        applyColours (false);
    }

private:
    void begin()
    {
        flashing = false;
        proc.startLearn();
        setButtonText (juce::String::fromUTF8 ("\xe2\x80\xa6"));
        applyColours (true);
        startTimer (3000);
    }

    void finish()
    {
        stopTimer();
        setButtonText ("Learn");
        applyColours (false);

        const auto res = proc.finishLearnAndAnalyseResonances();
        const auto slot = assignResonanceSlots (proc, res, band);
        if (slot[(size_t) band] <= 0.0)
        {
            flashing = true;                       // that resonance wasn't there
            setColour (juce::TextButton::buttonColourId, ui::pal->warn);
            setColour (juce::TextButton::textColourOffId, ui::pal->bg);
            repaint();
            startTimer (900);
            return;
        }
        applyResonanceToBand (proc, band, slot[(size_t) band]);
    }

    void timerCallback() override
    {
        if (proc.isLearning()) { finish(); return; }
        stopTimer();
        if (flashing) { flashing = false; applyColours (false); }
    }

    void applyColours (bool listening)
    {
        setColour (juce::TextButton::buttonColourId, listening ? ui::pal->open : ui::pal->btn);
        setColour (juce::TextButton::textColourOffId, listening ? ui::pal->bg : ui::pal->dim);
        repaint();
    }

    MagicDrumDeBleedAudioProcessor& proc;
    const int band;
    bool flashing = false;
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
