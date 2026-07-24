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

/*  Resonance slot model — K1 is ALWAYS the fundamental (the loudest
    resonance), and nothing below it is ever considered: the working set is
    the fundamental plus the loudest resonances ABOVE it (5 total),
    frequency-ascending, so K1..K5 run low to high with K1 = fundamental.

    Learn-all (selfBand −1): fresh assignment of every slot from that set.

    Individual learns:
      - K1 is the exception: always the fundamental, regardless of any
        other band (it is the special band linked to the trigger Focus).
      - K2..K5: ONLY the nearest enabled band below and the nearest enabled
        band above form the search fences (an enabled band farther out is
        never consulted, even if its frequency happens to fall inside the
        gap); with no enabled band below, the fundamental itself is the
        floor — K1's slot always counts as the fundamental. Several
        disabled bands sharing one gap take positional shares of its
        candidates in ascending order (only K3 enabled, learning K2: the
        two slots under it are K1 = fundamental and K2 = the one resonance
        between fundamental and K3). The band being learned is always
        treated as fresh — its current frequency plays no part.
    Unfillable slots return 0. */
inline std::array<double, 5> assignResonanceSlots (MagicDrumDeBleedAudioProcessor& proc,
                                                   const std::vector<mdd::LearnAnalyzer::Resonance>& res,
                                                   int selfBand)
{
    std::array<double, 5> slot {};
    if (res.empty())
        return slot;

    auto spacing = [] (double f) { return juce::jmax (12.0, 0.06 * f); };

    // Working set: fundamental + loudest resonances above it, ascending.
    const double f0 = res[0].hz;                   // res is loudest-first
    std::vector<double> d { f0 };
    for (size_t k = 1; k < res.size() && d.size() < 5; ++k)
        if (res[k].hz > f0 + spacing (f0))
            d.push_back (res[k].hz);               // below-fundamental: ignored
    std::sort (d.begin(), d.end());                // d[0] == f0

    if (selfBand < 0)                              // learn-all
    {
        for (size_t i = 0; i < d.size() && i < slot.size(); ++i)
            slot[i] = d[i];
        return slot;
    }
    if (selfBand == 0)                             // K1: always the fundamental
    {
        slot[0] = f0;
        return slot;
    }

    auto enabledFreq = [&] (int b) -> double
    {
        auto* on = proc.apvts.getRawParameterValue (ParamIDs::notchOn (b));
        if (on == nullptr || on->load() <= 0.5f)
            return -1.0;
        return (double) proc.apvts.getRawParameterValue (ParamIDs::notchFreq (b))->load();
    };

    // Fences: nearest enabled band each side; no enabled band below means
    // the fundamental is the floor (K1's slot is the fundamental by fiat).
    double lower = f0;
    int lowerIdx = 0;
    for (int b = selfBand - 1; b >= 1; --b)
        if (const double f = enabledFreq (b); f > 0.0) { lower = f; lowerIdx = b; break; }
    if (lowerIdx == 0)
        if (const double f = enabledFreq (0); f > 0.0)
            lower = f;                             // enabled K1 fences at ITS freq
    double upper = 0.0;                            // 0 = unbounded above
    for (int b = selfBand + 1; b < 5; ++b)
        if (const double f = enabledFreq (b); f > 0.0) { upper = f; break; }

    // Positional rank of this band within the disabled run above the fence
    // (the band being learned always counts itself — a re-learn is fresh).
    int rank = 0;
    for (int b = lowerIdx + 1; b <= selfBand; ++b)
        if (b == selfBand || enabledFreq (b) <= 0.0)
            ++rank;

    int n = 0;
    for (const double f : d)                       // ascending
    {
        if (f <= lower + spacing (lower))
            continue;
        if (upper > 0.0 && f >= upper - spacing (upper))
            break;                                 // ascending: everything after is out too
        if (++n == rank)
        {
            slot[(size_t) selfBand] = f;
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
    learnBandCount > 0: assigns the lowest `learnBandCount` resonance
    slots (frequency-ordered from the top-5 loudest) — K1 the lowest, then
    upward; the Simple view uses 3, the Tail stage's "Learn all" uses 5.
    Bands with no detected slot are left untouched. Focus follows K1.
    Flashes red briefly when nothing was detected. */
class LearnButton : public juce::TextButton, private juce::Timer
{
public:
    explicit LearnButton (MagicDrumDeBleedAudioProcessor& p, int learnBandCount = 0)
        : juce::TextButton ("Learn"), proc (p), learnBands (learnBandCount)
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

        if (learnBands <= 0)
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
        for (int b = 0; b < juce::jmin (5, learnBands); ++b)
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
    int learnBands = 0;
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
    (both Thresholds, Ratio, Selectivity, Hysteresis, MIDI trigger — those
    depend on the track, not the drum), then set the preset's values.
    Shared by the PresetBrowser and the Simple view. */
inline void applyFactoryPreset (MagicDrumDeBleedAudioProcessor& proc, const presets::FactoryPreset& pr)
{
    static const juce::StringArray preserved { ParamIDs::threshold, ParamIDs::compThreshold,
                                               ParamIDs::contrast, ParamIDs::hysteresis,
                                               ParamIDs::midiTrigger, ParamIDs::compRatio };
    for (auto* p : proc.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! preserved.contains (ranged->paramID))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

    setRealValue (proc, ParamIDs::scFreq,        pr.scFreqHz);
    setRealValue (proc, ParamIDs::scQ,           pr.scQ);
    setRealValue (proc, ParamIDs::lookahead,     (float) pr.lookaheadMs);
    setRealValue (proc, ParamIDs::hold,          pr.holdMs);
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
