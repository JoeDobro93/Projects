#include "TailCanvas.h"
#include "BandIds.h"

using namespace ui;

namespace
{
    // Complex product of all enabled bands' stages at frequency f.
    void chainH (juce::RangedAudioParameter* const* onP, juce::RangedAudioParameter* const* freqP,
                 juce::RangedAudioParameter* const* qP, juce::RangedAudioParameter* const* gainP,
                 juce::RangedAudioParameter* const* shapeP,
                 double f, double sr, double& reOut, double& imOut)
    {
        double re = 1.0, im = 0.0;
        for (int b = 0; b < 7; ++b)
        {
            auto val = [] (juce::RangedAudioParameter* p) { return p != nullptr ? (double) p->convertFrom0to1 (p->getValue()) : 0.0; };
            if (val (onP[b]) < 0.5) continue;

            mdd::BandParams bp;
            bp.enabled = true;
            bp.freqHz  = val (freqP[b]);
            bp.q       = b >= 2 ? val (qP[b]) : 0.707;
            bp.gainDb  = b >= 2 ? mdd::ringToGainDb (val (gainP[b])) : 0.0;
            bp.shape   = (int) val (shapeP[b]);
            bp.slope   = bp.shape;

            mdd::BiquadFilter::Coeffs cs[mdd::EQProcessor::kMaxStages];
            const int kind = b == 0 ? 0 : (b == 1 ? 1 : 2);
            const int n = mdd::EQProcessor::computeCoefficients (bp, kind, sr, cs);
            for (int s = 0; s < n; ++s)
            {
                double hr, hi;
                mdd::BiquadFilter::responseAt (cs[s], f, sr, hr, hi);
                const double t = re * hr - im * hi;
                im = re * hi + im * hr;
                re = t;
            }
        }
        reOut = re; imOut = im;
    }
}

TailCanvas::TailCanvas (MagicDrumDeBleedAudioProcessor& proc, std::function<void (int)> cb)
    : processor (proc), onBandSelected (std::move (cb))
{
    for (int b = 0; b < 7; ++b)
    {
        onP[b]    = proc.apvts.getParameter (eqids::onId (b));
        freqP[b]  = proc.apvts.getParameter (eqids::freqId (b));
        shapeP[b] = proc.apvts.getParameter (eqids::shapeId (b));
        if (b >= 2)
        {
            qP[b]    = proc.apvts.getParameter (eqids::qId (b));
            gainP[b] = proc.apvts.getParameter (eqids::gainId (b));
        }
    }
    for (int i = 0; i < kN; ++i)
        freqs[i] = 20.0 * std::pow (1000.0, i / double (kN - 1));

    sampleFifo.resize (kFftSize, 0.0f);
    fftData.resize (kFftSize * 2, 0.0f);
    smoothedDb.assign (kBins, -100.0f);
    accumDb.assign (kBins, -100.0f);
    startTimerHz (30);
}

void TailCanvas::setAccumulate (bool b)
{
    accumulate = b;
    if (! b) std::fill (accumDb.begin(), accumDb.end(), -100.0f);
}

void TailCanvas::timerCallback()
{
    for (;;)
    {
        const int got = processor.readSpectrumSamples (pull, 512);
        if (got <= 0) break;
        if (frozen) continue;
        for (int i = 0; i < got; ++i)
        {
            sampleFifo[(size_t) fifoIdx] = pull[i];
            if (++fifoIdx >= kFftSize) { fifoIdx = 0; runFFT(); }
        }
    }
    repaint();
}

void TailCanvas::runFFT()
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (sampleFifo.begin(), sampleFifo.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);
    const float sc4 = 4.0f / (float) kFftSize;
    for (int i = 0; i < kBins; ++i)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[(size_t) i] * sc4, -100.0f);
        float& s = smoothedDb[(size_t) i];
        s += ((db > s) ? 0.55f : 0.18f) * (db - s);
        if (accumulate)
        {
            float& a = accumDb[(size_t) i];
            a = juce::jmax (a - 0.08f, db);
        }
    }
}

void TailCanvas::recomputeCurve()
{
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    for (int i = 0; i < kN; ++i)
    {
        double re, im;
        chainH (onP, freqP, qP, gainP, shapeP, freqs[i], sr, re, im);
        keepDb[i] = 20.0 * std::log10 (juce::jmax (std::hypot (1.0 - re, im), 1.0e-6));
        hDb[i]    = 20.0 * std::log10 (juce::jmax (std::hypot (re, im), 1.0e-6));
    }
}

double TailCanvas::hDbAt (double f) const
{
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    double re, im;
    chainH (onP, freqP, qP, gainP, shapeP, f, sr, re, im);
    return 20.0 * std::log10 (juce::jmax (std::hypot (re, im), 1.0e-6));
}

void TailCanvas::buildGoldCurve()
{
    // The fixed log grid undersamples narrow peaks (the tip visibly jitters
    // during a frequency sweep), so cluster extra samples around every
    // enabled band's centre — the drawn tip is then always exact.
    curveF.clear();
    curveF.reserve ((size_t) kN + 7 * 7);
    curveF.insert (curveF.end(), std::begin (freqs), std::end (freqs));

    static const double offs[] = { -0.10, -0.045, -0.016, 0.0, 0.016, 0.045, 0.10 };
    auto val = [] (juce::RangedAudioParameter* p) { return p != nullptr ? (double) p->convertFrom0to1 (p->getValue()) : 0.0; };
    for (int b = 0; b < 7; ++b)
    {
        if (val (onP[b]) < 0.5) continue;
        const double f0 = val (freqP[b]);
        for (double o : offs)
        {
            const double f = f0 * std::pow (2.0, o);
            if (f >= 20.0 && f <= 20000.0)
                curveF.push_back (f);
        }
    }
    std::sort (curveF.begin(), curveF.end());

    curveHdB.resize (curveF.size());
    for (size_t i = 0; i < curveF.size(); ++i)
        curveHdB[i] = hDbAt (curveF[i]);
}

double TailCanvas::keepAvg (MagicDrumDeBleedAudioProcessor& proc, double amount)
{
    juce::RangedAudioParameter *onP[7], *freqP[7], *qP[7] {}, *gainP[7] {}, *shapeP[7];
    for (int b = 0; b < 7; ++b)
    {
        onP[b]    = proc.apvts.getParameter (eqids::onId (b));
        freqP[b]  = proc.apvts.getParameter (eqids::freqId (b));
        shapeP[b] = proc.apvts.getParameter (eqids::shapeId (b));
        if (b >= 2)
        {
            qP[b]    = proc.apvts.getParameter (eqids::qId (b));
            gainP[b] = proc.apvts.getParameter (eqids::gainId (b));
        }
    }
    const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
    double sum = 0.0;
    for (int i = 0; i <= 80; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, i / 80.0);
        double re, im;
        chainH (onP, freqP, qP, gainP, shapeP, f, sr, re, im);
        const double kr = 1.0 - amount * re, ki = amount * im;
        sum += kr * kr + ki * ki;
    }
    return std::sqrt (sum / 81.0);
}

int TailCanvas::gateState (MagicDrumDeBleedAudioProcessor& proc, float& open01, float& tail01)
{
    open01 = juce::jlimit (0.0f, 1.0f, proc.getGainReductionDb() / -96.0f);
    tail01 = juce::jlimit (0.0f, 1.0f, std::pow (10.0f, proc.getEqGateReductionDb() / 20.0f));
    const double amount = proc.getIntensity01();
    const bool keepAudible = keepAvg (proc, amount) > (1.0 - amount) + 0.02;
    return open01 > 0.5f ? 2 : (tail01 > 0.12f && keepAudible ? 1 : 0);
}

//==============================================================================
juce::Rectangle<float> TailCanvas::plotArea() const
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    r.removeFromBottom (scf (12.0f));   // frequency labels
    return r;
}
float TailCanvas::fx (double f, float w) const   { return (float) (std::log (f / 20.0) / std::log (1000.0) * w); }
double TailCanvas::xf (float x, float w) const   { return 20.0 * std::pow (1000.0, x / juce::jmax (1.0f, w)); }
float TailCanvas::dyy (float db, float h) const  { return (kTopDb - juce::jlimit (kBotDb - 8.0f, kTopDb, db)) / (kTopDb - kBotDb) * h; }

void TailCanvas::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setColour (pal->panel2); g.fillRoundedRectangle (full, 4.0f);
    recomputeCurve();

    auto area = plotArea();
    const float w = area.getWidth(), h = area.getHeight();
    g.saveState();
    g.addTransform (juce::AffineTransform::translation (area.getX(), area.getY()));

    // grid
    struct NF { double f; const char* n; };
    static const NF named[] = { {50,"50"},{100,"100"},{200,"200"},{500,"500"},{1000,"1k"},
                                {2000,"2k"},{5000,"5k"},{10000,"10k"},{20000,"20k"} };
    static const double gridF[] = { 20,30,50,100,200,300,500,1000,2000,3000,5000,10000,20000 };
    g.setFont (font (9.0f));
    for (double f : gridF)
    {
        bool isNamed = false; const char* nm = nullptr;
        for (auto& n : named) if (juce::exactlyEqual (n.f, f)) { isNamed = true; nm = n.n; break; }
        const float x = fx (f, w);
        g.setColour (pal->line.withAlpha (isNamed ? 0.55f : 0.25f));
        g.drawVerticalLine ((int) x, 0.0f, h);
        if (nm != nullptr)
        {
            g.setColour (pal->faint);
            g.drawText (nm, (int) x - sc (14), (int) h + sc (1), sc (28), sc (10), juce::Justification::centred);
        }
    }
    // horizontal gridlines at whole-dB cut levels, each labelled ON its line
    // (small bg patch under the text) so line and value read as one; the
    // right edge gets ticks for the monitor-signal axis the spectra use
    {
        const float stepDb = dispMax <= 12.0f ? 3.0f : 6.0f;
        g.setFont (font (8.5f));
        for (float d = stepDb; d < dispMax - 0.5f; d += stepDb)
        {
            const float y = h * (1.0f - d / dispMax);
            g.setColour (pal->line.withAlpha (0.3f));
            g.drawHorizontalLine ((int) y, 0.0f, w);
            const auto lr = juce::Rectangle<int> (sc (4), (int) y - sc (5), sc (36), sc (10));
            g.setColour (pal->panel2);
            g.fillRect (lr);
            g.setColour (pal->faint);
            g.drawText ("-" + juce::String ((int) d) + " dB", lr, juce::Justification::centredLeft);
        }
        // Signal-level ticks track the MON offset (they mark true input dB,
        // wherever the fader has shifted the drawn spectra), alternating the
        // dry-blue / kept-orange colours so it reads as the spectra's axis.
        for (int v = 36; v >= -84; v -= 12)
        {
            const float y = (kTopDb - ((float) v + monGainDb)) / (kTopDb - kBotDb) * h;
            if (y < scf (22.0f) || y > h - scf (6.0f))
                continue;
            g.setColour (((v / 12) % 2 == 0 ? juce::Colour (0xff4aa8e0)
                                            : juce::Colour (0xffe07e2a)).withAlpha (0.75f));
            g.drawHorizontalLine ((int) y, w - scf (5.0f), w);
            g.drawText ((v > 0 ? "+" : "") + juce::String (v),
                        (int) w - sc (33), (int) y - sc (5), sc (26), sc (10),
                        juce::Justification::centredRight);
        }
    }

    // keep the spectral layers inside the plot (out of the label strip)
    g.reduceClipRegion (0, 0, (int) w, (int) h);

    // spectral layers: blue = dry, orange = dry + keep
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    const float binHz = (float) (sr / (double) kFftSize);
    const auto& bins = accumulate ? accumDb : smoothedDb;
    auto dryAt = [&] (double f)
    {
        const float bf = juce::jlimit (0.0f, (float) (kBins - 2), (float) (f / binHz));
        const int b = (int) bf; const float fr = bf - (float) b;
        return bins[(size_t) b] * (1.0f - fr) + bins[(size_t) (b + 1)] * fr + monGainDb;
    };
    auto layer = [&] (bool useKeep, juce::Colour top, juce::Colour bot, juce::Colour stroke)
    {
        juce::Path p;
        p.startNewSubPath (0.0f, h);
        for (int i = 0; i < kN; ++i)
        {
            float d = dryAt (freqs[i]);
            if (useKeep) d += (float) keepDb[i];
            p.lineTo (fx (freqs[i], w), dyy (d, h));
        }
        p.lineTo (w, h); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (top, 0, 0, bot, 0, h, false));
        g.fillPath (p);
        g.setColour (stroke);
        g.strokePath (p, juce::PathStrokeType (1.0f));
    };
    if (showDry)  layer (false, juce::Colour (0x574aa8e0), juce::Colour (0x124aa8e0), juce::Colour (0x8c4aa8e0));
    if (showKept) layer (true,  juce::Colour (0x9ee07e2a), juce::Colour (0x29e07e2a), juce::Colour (0xe6f0963c));

    // gold band curve: cut depth v = −hDb on a 0..dispMax axis, 0 at the
    // bottom line. Identical shape to "Show internals", mirrored. Deep tips
    // clip offscreen (normal EQ behaviour).
    auto goldY = [&] (double db) { return h * (1.0f - (float) (-db) / dispMax); };
    buildGoldCurve();
    {
        juce::Path p;
        for (size_t i = 0; i < curveF.size(); ++i)
        {
            const float x = fx (curveF[i], w), y = (float) goldY (curveHdB[i]);
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (pal->gold);
        g.strokePath (p, juce::PathStrokeType (2.0f));
    }
    if (internals)
    {
        juce::Path p;
        for (size_t i = 0; i < curveF.size(); ++i)
        {
            const float x = fx (curveF[i], w), y = h * (float) (-curveHdB[i]) / dispMax;
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        const float dash[2] = { 4.0f, 3.0f };
        g.setColour (pal->dim);
        g.strokePath (p, juce::PathStrokeType (1.0f), {});
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dash, 2);
        g.fillPath (dashed);
        g.setFont (font (9.5f));
        g.drawText ("dashed = internal cancellation filter", (int) w - sc (220), sc (16), sc (212), sc (12),
                    juce::Justification::centredRight);
    }

    // handles
    bool anyOn = false;
    for (int b = 0; b < 7; ++b)
    {
        auto val = [] (juce::RangedAudioParameter* p) { return p != nullptr ? (double) p->convertFrom0to1 (p->getValue()) : 0.0; };
        if (val (onP[b]) < 0.5) { handleX[b] = -999; continue; }
        anyOn = true;
        const double f = juce::jlimit (20.0, 20000.0, val (freqP[b]));
        // every handle rides the gold curve, evaluated exactly at its frequency
        const float y = juce::jlimit (0.0f, h, (float) goldY (hDbAt (f)));
        const float x = fx (f, w);
        handleX[b] = x; handleY[b] = y;

        const auto c = pal->band[b];
        const int shape = b >= 2 ? (int) val (shapeP[b]) : 0;
        const float rr = scf (8.0f);
        g.setColour (c.withAlpha (sel == b ? 1.0f : 0.72f));
        juce::Path hp;
        if (shape == 1 && b >= 2) hp.addRoundedRectangle (x - rr, y - rr, rr * 2, rr * 2, 4.0f);
        else if (shape == 2 && b >= 2) { hp.addTriangle (x, y - rr - 1, x + rr, y + rr * 0.75f, x - rr, y + rr * 0.75f); }
        else hp.addEllipse (x - rr, y - rr, rr * 2, rr * 2);
        g.fillPath (hp);
        g.setColour (sel == b ? juce::Colours::white : juce::Colours::black.withAlpha (0.45f));
        g.strokePath (hp, juce::PathStrokeType (sel == b ? 2.0f : 1.0f));
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.setFont (font (9.0f, true));
        static const char* glyph[7] = { "L", "H", "1", "2", "3", "4", "5" };
        g.drawText (glyph[b], (int) (x - rr), (int) (y - rr), (int) (rr * 2), (int) (rr * 2), juce::Justification::centred);
    }

    if (! anyOn)
    {
        g.setColour (pal->faint);
        g.setFont (font (12.0f));
        g.drawText (juce::String::fromUTF8 ("No keep bands active \xe2\x80\x94 the gate will cut the drum off dead."),
                    0, (int) (h / 2) - sc (16), (int) w, sc (14), juce::Justification::centred);
        g.setFont (font (11.0f));
        g.drawText ("Enable LP, or drag a K band onto the drum's fundamental.",
                    0, (int) (h / 2) + sc (1), (int) w, sc (14), juce::Justification::centred);
    }
    g.restoreState();

    g.setColour (pal->faint);
    g.setFont (font (10.5f));
    g.drawText ("drag a handle onto the frequencies you want to keep", sc (8), sc (5), sc (300), sc (12),
                juce::Justification::centredLeft);
    // key (top-right) — the chips are click-toggles for their layers
    g.setFont (font (10.0f));
    const int kx = getWidth() - sc (136);   // leaves room for the freeze overlay
    dryLegend  = { kx, sc (3), sc (44), sc (16) };
    keptLegend = { kx + sc (48), sc (3), sc (50), sc (16) };
    g.setColour (juce::Colour (0x8c4aa8e0).withMultipliedAlpha (showDry ? 1.0f : 0.28f));
    g.fillRoundedRectangle ((float) kx, scf (7.0f), scf (9.0f), scf (9.0f), 2.0f);
    g.setColour (showDry ? pal->faint : pal->faint.withMultipliedAlpha (0.45f));
    g.drawText ("dry", kx + sc (12), sc (5), sc (30), sc (12), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xbfe07e2a).withMultipliedAlpha (showKept ? 1.0f : 0.28f));
    g.fillRoundedRectangle ((float) (kx + sc (48)), scf (7.0f), scf (9.0f), scf (9.0f), 2.0f);
    g.setColour (showKept ? pal->faint : pal->faint.withMultipliedAlpha (0.45f));
    g.drawText ("kept", kx + sc (60), sc (5), sc (36), sc (12), juce::Justification::centredLeft);

    g.setColour (pal->line);
    g.drawRoundedRectangle (full, 4.0f, 1.0f);
}

//==============================================================================
int TailCanvas::bandAt (juce::Point<float> pos) const
{
    auto area = plotArea();
    const juce::Point<float> p (pos.x - area.getX(), pos.y - area.getY());
    int best = -1; float bd = scf (18.0f);
    for (int b = 0; b < 7; ++b)
    {
        if (handleX[b] < -100) continue;
        const float d = std::hypot (p.x - handleX[b], p.y - handleY[b]);
        if (d < bd) { bd = d; best = b; }
    }
    return best;
}

void TailCanvas::mouseDown (const juce::MouseEvent& e)
{
    if (dryLegend.contains (e.getPosition()))  { showDry  = ! showDry;  repaint(); return; }
    if (keptLegend.contains (e.getPosition())) { showKept = ! showKept; repaint(); return; }

    drag = bandAt (e.position);
    if (drag < 0) return;
    sel = drag;
    if (onBandSelected) onBandSelected (drag);
    freqP[drag]->beginChangeGesture();
    if (gainP[drag] != nullptr) gainP[drag]->beginChangeGesture();
}

void TailCanvas::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto area = plotArea();
    auto* fp = freqP[drag];
    const auto& fr = fp->getNormalisableRange();
    const float f = juce::jlimit (fr.start, fr.end,
                                  (float) xf (juce::jlimit (0.0f, area.getWidth(), e.position.x - area.getX()), area.getWidth()));
    fp->setValueNotifyingHost (fp->convertTo0to1 (f));

    if (auto* gp = gainP[drag])
    {
        // Cursor height maps onto the display's cut-depth axis; above the
        // canvas keeps increasing the ring up to the knob's maximum.
        const float yr = e.position.y - area.getY();          // may be negative
        const float depth = (1.0f - yr / area.getHeight()) * dispMax;
        const float ring = (float) mdd::gainDbToRing (-juce::jmax (0.0f, depth));
        gp->setValueNotifyingHost (gp->convertTo0to1 (juce::jlimit (0.0f, 20.0f, ring)));
    }
}

void TailCanvas::mouseUp (const juce::MouseEvent&)
{
    if (drag < 0) return;
    freqP[drag]->endChangeGesture();
    if (gainP[drag] != nullptr) gainP[drag]->endChangeGesture();
    drag = -1;
}

void TailCanvas::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (bandAt (e.position) >= 0 ? juce::MouseCursor::DraggingHandCursor
                                             : juce::MouseCursor::NormalCursor);
}

void TailCanvas::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wh)
{
    const int b = drag >= 0 ? drag : bandAt (e.position);
    if (b < 2 || qP[b] == nullptr) return;
    auto* qp = qP[b];
    const auto& r = qp->getNormalisableRange();
    const float q = qp->convertFrom0to1 (qp->getValue());
    qp->beginChangeGesture();
    qp->setValueNotifyingHost (qp->convertTo0to1 (juce::jlimit (r.start, r.end,
                                q * std::pow (2.0f, wh.deltaY * 1.5f))));
    qp->endChangeGesture();
}
