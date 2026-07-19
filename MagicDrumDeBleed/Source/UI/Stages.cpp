#include "Stages.h"
#include "BandIds.h"

using namespace ui;

static void styleSeg (juce::TextButton& b, bool on)
{
    b.setColour (juce::TextButton::buttonColourId,   on ? pal->btnOn : pal->btn);
    b.setColour (juce::TextButton::buttonOnColourId, pal->btnOn);
    b.setColour (juce::TextButton::textColourOffId,  on ? pal->btnOnText : pal->dim);
    b.setColour (juce::TextButton::textColourOnId,   pal->btnOnText);
}

//==============================================================================
StageHeader::StageHeader (int n, juce::String t, int c)
    : num (n), clr (c), title (std::move (t))
{
    setInterceptsMouseClicks (false, false);
}

void StageHeader::paint (juce::Graphics& g)
{
    const juce::Colour cc = clr == 1 ? pal->open : clr == 2 ? pal->tail : pal->accent;
    const float d = scf (17.0f);
    auto circle = juce::Rectangle<float> (0, ((float) getHeight() - d) / 2, d, d);
    g.setColour (cc);
    g.fillEllipse (circle);
    g.setColour (pal->bg);
    g.setFont (font (11.0f, true));
    g.drawText (juce::String (num), circle.toNearestInt(), juce::Justification::centred);
    g.setColour (pal->txt);
    g.setFont (font (12.5f, true));
    const int tw = sc (9) + (int) d;
    g.drawText (title, tw, 0, sc (110), getHeight(), juce::Justification::centredLeft);
}

//==============================================================================
TriggerStage::TriggerStage (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      trigMeter ("TRIGGER", [&proc] { return proc.getDetectorRmsDb(); },
                 proc.apvts.getParameter (ParamIDs::threshold))
{
    addAndMakeVisible (header);
    auto& ap = processor.apvts;

    addAndMakeVisible (bypassBtn);
    setHint (bypassBtn, "Bypass", "Bypass the trigger stage: the gate stays permanently open, so no bleed is removed.");
    bypassBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::compBypass);
        bypassAtt->setValueAsCompleteGesture (p->convertFrom0to1 (p->getValue()) > 0.5f ? 0.0f : 1.0f);
    };
    bypassAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::compBypass),
        [this] (float v) { styleSeg (bypassBtn, v > 0.5f); bypassBtn.repaint(); });
    bypassAtt->sendInitialUpdate();

    addAndMakeVisible (trigMeter);
    setHint (trigMeter, "TRIGGER", "Detector level — what the plugin is listening to, AFTER the trigger filter. Drag the red line to set the Threshold.");

    addAndMakeVisible (threshold);
    threshold.attach (ap.getParameter (ParamIDs::threshold));
    setHint (threshold, "Threshold", "The level a hit must exceed to open the gate. Anything quieter is treated as bleed and cancelled.");

    addAndMakeVisible (smoothing);
    smoothing.attach (ap.getParameter (ParamIDs::rmsWindow));
    setHint (smoothing, "Smoothing", "How much the detector averages. Opening always uses a fast detector so attacks are never clipped; Smoothing mainly steadies when the gate closes and rejects short spikes of bleed.");

    addAndMakeVisible (focus);
    focus.attach (ap.getParameter (ParamIDs::scFreq));
    setHint (focus, "Focus", "Bandpass: the centre of the band the detector listens to. High/Low Pass: the cutoff frequency.");

    addAndMakeVisible (width);
    setHint (width, "Width", "Bandpass: how wide a band the detector hears. High/Low Pass: how steeply it rolls off.");
    updateWidthKnob();

    static const char* typeNames[3] = { "High Pass", "Low Pass", "Bandpass" };
    static const char* typeHints[3] = {
        "Detector ignores everything below the cutoff. Good for stopping the kick from opening the gate.",
        "Detector ignores everything above the cutoff. Good for stopping cymbals and hats.",
        "Detector listens to one band only. Rejects both the kick and the cymbals at once." };
    for (int i = 0; i < 3; ++i)
    {
        typeBtns[i].setButtonText (typeNames[i]);
        setHint (typeBtns[i], typeNames[i], typeHints[i]);
        typeBtns[i].onClick = [this, i]
        {
            auto* p = processor.apvts.getParameter (ParamIDs::scType);
            typeAtt->setValueAsCompleteGesture ((float) i);
            juce::ignoreUnused (p);
        };
        addAndMakeVisible (typeBtns[i]);
    }
    typeAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::scType),
        [this] (float) { rebuildTypeButtons(); updateWidthKnob(); });
    typeAtt->sendInitialUpdate();

    addAndMakeVisible (enableBtn);
    setHint (enableBtn, "Enabled", "Turn the trigger filter on or off. It narrows what the detector hears so other drums don't open the gate. Never affects the sound itself.");
    enableBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::scEnable);
        scEnableAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    scEnableAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::scEnable),
        [this] (float v) { styleSeg (enableBtn, v > 0.5f); });
    scEnableAtt->sendInitialUpdate();

    addAndMakeVisible (learnBtn);
    setHint (learnBtn, "Learn", "Analyse the incoming audio and park the filter on this drum's dominant frequency. With Link to K1 on, K1 follows it too.");
    learnBtn.onClick = [this] { eqids::handleLearnClick (processor, learnBtn); };

    addAndMakeVisible (linkBtn);
    setHint (linkBtn, "Link to K1", "Locks the K1 keep band's frequency to the Focus frequency: turning either knob moves both, and Learn updates them together.");
    linkBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::linkK1);
        linkAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    linkAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::linkK1),
        [this] (float v) { styleSeg (linkBtn, v > 0.5f); });
    linkAtt->sendInitialUpdate();
}

void TriggerStage::rebuildTypeButtons()
{
    auto* p = processor.apvts.getParameter (ParamIDs::scType);
    const int cur = (int) std::lround (p->convertFrom0to1 (p->getValue()));
    for (int i = 0; i < 3; ++i)
        styleSeg (typeBtns[i], i == cur);
    repaint();
}

void TriggerStage::updateWidthKnob()
{
    auto* p = processor.apvts.getParameter (ParamIDs::scType);
    const bool bp = (int) std::lround (p->convertFrom0to1 (p->getValue())) == 2;
    if (bp)
    {
        width.setNameText ("Width");
        width.setReversed (true);
        width.setFormat ([] (float q) { return juce::String (qToOct (q), 2) + " oct"; });
        width.attach (processor.apvts.getParameter (ParamIDs::scQ));
    }
    else
    {
        width.setNameText ("Slope");
        width.setReversed (false);
        width.setFormat ([] (float s) { return juce::String ((int) s) + " dB/oct"; });
        width.attach (processor.apvts.getParameter (ParamIDs::scSlope));
    }
}

void TriggerStage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal->panel);  g.fillRoundedRectangle (r, 5.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (r, 5.0f, 1.0f);

    g.setColour (pal->faint);
    g.setFont (font (9.5f, true));
    g.drawText ("SENSITIVITY", sensLabelX, sc (34), sc (160), sc (12), juce::Justification::centredLeft);
    g.drawText ("TRIGGER FILTER", filtLabelX, sc (34), sc (400), sc (12), juce::Justification::centredLeft);
    g.setColour (pal->line);
    g.fillRect (dividerX, sc (32), 1, getHeight() - sc (42));
}

void TriggerStage::resized()
{
    auto r = getLocalBounds().reduced (sc (11), sc (8));
    auto head = r.removeFromTop (sc (20));
    bypassBtn.setBounds (head.removeFromRight (sc (64)).reduced (0, 0));
    header.setBounds (head);
    r.removeFromTop (sc (4));

    trigMeter.setBounds (r.removeFromLeft (sc (36)));
    r.removeFromLeft (sc (13));

    sensLabelX = r.getX();
    auto sens = r.removeFromLeft (sc (150));
    sens.removeFromTop (sc (14));
    threshold.setBounds (sens.removeFromLeft (sc (70)));
    smoothing.setBounds (sens.removeFromLeft (sc (70)));

    r.removeFromLeft (sc (6));
    dividerX = r.getX();
    r.removeFromLeft (sc (7));
    filtLabelX = r.getX();
    r.removeFromTop (sc (14));

    // button stack (Enabled / Learn / Link to K1) left, knobs middle, type right
    auto stack = r.removeFromLeft (sc (96));
    enableBtn.setBounds (stack.getX(), stack.getY(),           stack.getWidth(), sc (24));
    learnBtn.setBounds  (stack.getX(), stack.getY() + sc (27), stack.getWidth(), sc (24));
    linkBtn.setBounds   (stack.getX(), stack.getY() + sc (54), stack.getWidth(), sc (24));
    r.removeFromLeft (sc (11));
    focus.setBounds (r.removeFromLeft (sc (70)));
    width.setBounds (r.removeFromLeft (sc (70)));
    r.removeFromLeft (sc (11));
    auto seg = r.removeFromLeft (sc (96));
    for (int i = 0; i < 3; ++i)
        typeBtns[i].setBounds (seg.getX(), seg.getY() + i * sc (27), seg.getWidth(), sc (24));
}

//==============================================================================
GateStage::GateStage (MagicDrumDeBleedAudioProcessor& proc) : processor (proc)
{
    addAndMakeVisible (header);
    auto& ap = processor.apvts;
    addAndMakeVisible (lookahead);
    lookahead.attach (ap.getParameter (ParamIDs::lookahead));
    setHint (lookahead, "Lookahead", "The gate opens this far BEFORE the transient arrives, so attacks are never clipped. Costs this much latency, compensated by your DAW.");
    addAndMakeVisible (hold);
    hold.attach (ap.getParameter (ParamIDs::hold));
    setHint (hold, "Hold", "Minimum time the gate stays fully open after a hit.");
    addAndMakeVisible (release);
    release.attach (ap.getParameter (ParamIDs::release));
    setHint (release, "Release", juce::String::fromUTF8 ("How quickly the gate closes after Hold. Too short chops the drum \xe2\x80\x94 the TAIL stage then takes over."));

    setHint (*this, "History", "Live history. Blue = detector level, red dashes = threshold. The background colour is the gate state at that moment.");
    hist.reserve (kHist);
    startTimerHz (30);
}

void GateStage::timerCallback()
{
    state = TailCanvas::gateState (processor, open01, tail01);

    hist.push_back ({ processor.getDetectorRmsDb(), state });
    if ((int) hist.size() > kHist)
        hist.erase (hist.begin(), hist.begin() + ((int) hist.size() - kHist));

    auto* p = processor.apvts.getParameter (ParamIDs::lookahead);
    latencyText = "adds " + juce::String ((int) p->convertFrom0to1 (p->getValue())) + " ms latency";
    repaint();
}

void GateStage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal->panel);  g.fillRoundedRectangle (r, 5.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (pal->faint);
    g.setFont (font (10.5f));
    g.drawText (latencyText, getWidth() - sc (200), sc (8), sc (188), sc (14), juce::Justification::centredRight);

    // ---- history canvas ----
    auto cv = canvasArea.toFloat();
    g.setColour (pal->panel2); g.fillRoundedRectangle (cv, 4.0f);
    const int n = (int) hist.size();
    if (n > 1)
    {
        const float cw = cv.getWidth() / (float) kHist;
        for (int i = 0; i < n; ++i)
        {
            if (hist[(size_t) i].state == 0) continue;
            g.setColour (hist[(size_t) i].state == 2 ? pal->open.withAlpha (0.20f) : pal->tail.withAlpha (0.14f));
            g.fillRect (cv.getRight() - (float) (n - i) * cw, cv.getY(), cw + 0.6f, cv.getHeight());
        }
        auto yFor = [&] (float db) { return cv.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * cv.getHeight(); };
        // threshold dashes
        if (auto* tp = processor.apvts.getParameter (ParamIDs::threshold))
        {
            const float ty = yFor (tp->convertFrom0to1 (tp->getValue()));
            g.setColour (pal->warn);
            const float dash[2] = { 5.0f, 4.0f };
            g.drawDashedLine ({ cv.getX(), ty, cv.getRight(), ty }, dash, 2, 1.2f);
        }
        juce::Path line, fill;
        fill.startNewSubPath (cv.getRight() - (float) n * cw, cv.getBottom());
        for (int i = 0; i < n; ++i)
        {
            const float x = cv.getRight() - (float) (n - i) * cw, y = yFor (hist[(size_t) i].det);
            i == 0 ? line.startNewSubPath (x, y) : line.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (cv.getRight(), cv.getBottom());
        fill.closeSubPath();
        g.setColour (pal->accent.withAlpha (0.22f)); g.fillPath (fill);
        g.setColour (pal->accent);                   g.strokePath (line, juce::PathStrokeType (1.4f));
    }
    g.setColour (pal->line); g.drawRoundedRectangle (cv, 4.0f, 1.0f);
    g.setColour (pal->faint); g.setFont (font (10.5f));
    g.drawText ("detector history", canvasArea.getX() + sc (8), canvasArea.getY() + sc (4), sc (140), sc (12),
                juce::Justification::centredLeft);

    // ---- state box ----
    auto sb = stateArea.toFloat();
    auto chip = sb.removeFromTop (scf (30.0f));
    const juce::Colour chipBg = state == 2 ? pal->open : state == 1 ? pal->tail : pal->panel2;
    g.setColour (chipBg); g.fillRoundedRectangle (chip, 4.0f);
    if (state == 0) { g.setColour (pal->line); g.drawRoundedRectangle (chip, 4.0f, 1.0f); }
    g.setColour (state == 0 ? pal->faint : pal->bg);
    g.setFont (font (13.0f, true));
    g.drawText (state == 2 ? "OPEN" : state == 1 ? "TAIL" : "CLOSED", chip.toNearestInt(), juce::Justification::centred);

    sb.removeFromTop (scf (5.0f));
    static const char* notes[3] = { "Bleed is being cancelled \xe2\x80\x94 the track is quiet.",
                                    "Only your keep bands are ringing through.",
                                    "Full drum passing through, untouched." };
    g.setColour (pal->dim);
    g.setFont (font (10.5f));
    g.drawFittedText (juce::String::fromUTF8 (notes[state]), sb.removeFromTop (scf (28.0f)).toNearestInt(),
                      juce::Justification::centredTop, 2);

    auto barRow = [&] (float v, juce::Colour c, const char* l, const char* rlab)
    {
        sb.removeFromTop (scf (4.0f));
        auto bar = sb.removeFromTop (scf (9.0f));
        g.setColour (pal->panel2); g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (pal->line);   g.drawRoundedRectangle (bar, 3.0f, 1.0f);
        g.setColour (c);
        if (v > 0.01f) g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * v), 3.0f);
        auto leg = sb.removeFromTop (scf (11.0f));
        g.setColour (pal->faint); g.setFont (font (9.0f));
        g.drawText (l, leg.toNearestInt(), juce::Justification::centredLeft);
        g.drawText (rlab, leg.toNearestInt(), juce::Justification::centredRight);
    };
    barRow (open01, pal->open, "closed", "gate open");
    barRow (tail01, pal->tail, "silent", "tail ringing");
}

void GateStage::resized()
{
    auto r = getLocalBounds().reduced (sc (11), sc (8));
    auto head = r.removeFromTop (sc (20));
    header.setBounds (head);
    r.removeFromTop (sc (7));

    auto knobs = r.removeFromLeft (sc (212));
    knobs.removeFromTop (sc (2));
    lookahead.setBounds (knobs.removeFromLeft (sc (70)));
    hold.setBounds (knobs.removeFromLeft (sc (70)));
    release.setBounds (knobs.removeFromLeft (sc (70)));

    r.removeFromLeft (sc (13));
    stateArea = r.removeFromRight (sc (124));
    r.removeFromRight (sc (13));
    canvasArea = r;
}

//==============================================================================
TailStage::TailStage (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      canvas (proc, [this] (int b) { selectBand (b); }),
      tailMeter ([&proc] { return std::pow (10.0f, proc.getEqGateReductionDb() / 20.0f); })
{
    addAndMakeVisible (header);
    auto& ap = processor.apvts;

    addAndMakeVisible (tailGateTg);
    setHint (tailGateTg, "Tail gate", juce::String::fromUTF8 ("When off, the kept bands ring continuously \xe2\x80\x94 including between hits, where they will rumble. Normally leave this on."));
    tailGateTg.onChange = [this] (bool on) { tailGateAtt->setValueAsCompleteGesture (on ? 1.0f : 0.0f); };
    tailGateAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::eqGateOn),
        [this] (float v) { tailGateTg.setState (v > 0.5f, false); });
    tailGateAtt->sendInitialUpdate();

    addAndMakeVisible (internalsBtn);
    setHint (internalsBtn, "Show internals", juce::String::fromUTF8 ("Show the raw filter curve applied to the internal cancellation copy \xe2\x80\x94 the curve the DSP actually runs."));
    internalsBtn.setClickingTogglesState (true);
    internalsBtn.onClick = [this] { canvas.setShowInternals (internalsBtn.getToggleState()); styleSeg (internalsBtn, internalsBtn.getToggleState()); };

    addAndMakeVisible (accumBtn);
    setHint (accumBtn, "Accumulate", "Hold spectrum peaks so resonant frequencies build up and stand out.");
    accumBtn.setClickingTogglesState (true);
    accumBtn.onClick = [this] { canvas.setAccumulate (accumBtn.getToggleState()); styleSeg (accumBtn, accumBtn.getToggleState()); };

    addAndMakeVisible (freezeBtn);
    setHint (freezeBtn, "Freeze", "Freeze the spectrum display while you adjust bands.");
    freezeBtn.setClickingTogglesState (true);
    freezeBtn.onClick = [this] { canvas.setFrozen (freezeBtn.getToggleState()); styleSeg (freezeBtn, freezeBtn.getToggleState()); };

    addAndMakeVisible (mon);
    setHint (mon, "Monitor gain", juce::String::fromUTF8 ("Boosts or cuts the dry/kept spectrum displays only \xe2\x80\x94 never the audio. Handy for quiet sources. Double-click resets."));
    mon.onChange = [this] (float db) { canvas.setMonitorGain (db); };

    addAndMakeVisible (canvas);
    setHint (canvas, "TAIL display", "Blue = the dry signal. Orange = what survives once the EQ'd cancellation copy is subtracted. Drag a handle: sideways = frequency, up/down = how loud it rings. Mouse-wheel = Q. Click the dry/kept legend to hide a layer.");

    static const char* labs[7] = { "LOWS", "HIGHS", "K1", "K2", "K3", "K4", "K5" };
    static const char* bandHints[7] = {
        "Everything BELOW this frequency keeps ringing through the tail \xe2\x80\x94 the main body-of-the-drum control.",
        "Everything ABOVE this frequency keeps ringing \xe2\x80\x94 snare wires, stick noise, air.",
        "A band of frequencies that keeps ringing through the tail.",
        "A band of frequencies that keeps ringing through the tail.",
        "A band of frequencies that keeps ringing through the tail.",
        "A band of frequencies that keeps ringing through the tail.",
        "A band of frequencies that keeps ringing through the tail." };
    for (int b = 0; b < 7; ++b)
    {
        bandBtns[b].setButtonText (labs[b]);
        setHint (bandBtns[b], labs[b], juce::String::fromUTF8 (bandHints[b]));
        bandBtns[b].onClick = [this, b] { selectBand (b); };
        addAndMakeVisible (bandBtns[b]);

        dotBtns[b].setButtonText ({});
        setHint (dotBtns[b], labs[b], "Enable / disable this keep band.");
        dotBtns[b].onClick = [this, b]
        {
            auto* p = processor.apvts.getParameter (eqids::onId (b));
            onAtts[b]->setValueAsCompleteGesture (p->convertFrom0to1 (p->getValue()) > 0.5f ? 0.0f : 1.0f);
        };
        addAndMakeVisible (dotBtns[b]);
        onAtts[b] = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (eqids::onId (b)),
            [this, b] (float v)
            {
                dotBtns[b].setColour (juce::TextButton::buttonColourId, v > 0.5f ? pal->band[b] : pal->panel2);
                dotBtns[b].repaint();
            });
        onAtts[b]->sendInitialUpdate();

        soloBtns[b].setButtonText ("S");
        setHint (soloBtns[b], "Solo", juce::String::fromUTF8 ("Solo \xe2\x80\x94 audition just this band to hear what you're keeping."));
        soloBtns[b].onClick = [this, b]
        {
            processor.setSoloBand (processor.getSoloBand() == b ? -1 : b);
            updateSoloButtons();
            selectBand (b);
        };
        addAndMakeVisible (soloBtns[b]);
    }

    bandLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (bandLabel);

    addAndMakeVisible (freq);
    setHint (freq, "Frequency", "Where this band sits.");
    addAndMakeVisible (widthK);
    widthK.setFormat ([] (float q) { return juce::String (q, q < 10.0f ? 2 : 1); });
    setHint (widthK, "Q", "Band width. Low Q = broad, keeps general body. High Q = narrow, rings just one note.");
    addAndMakeVisible (slopeK);
    slopeK.setFormat ([] (float v) { return juce::String (v < 0.5f ? 6 : 12) + " dB/oct"; });
    setHint (slopeK, "Slope", "How steeply frequencies beyond the cutoff stop ringing through.");
    addAndMakeVisible (ring);
    ring.setReversed (true);
    ring.setFormat ([] (float gdb)
    {
        const double k = 1.0 - std::pow (10.0, gdb / 20.0);
        return k <= 0.002 ? juce::String::fromUTF8 ("\xe2\x80\x94")
                          : juce::String (20.0 * std::log10 (k), 1) + " dB";
    });
    setHint (ring, "Ring level", "How loudly this band rings through during the tail, in dB relative to the original. Up = louder.");

    for (auto& s : shapeBtns) addChildComponent (s);

    addAndMakeVisible (tailHold);
    tailHold.attach (ap.getParameter (ParamIDs::eqGateHold));
    setHint (tailHold, "Tail hold", "How long the kept bands ring at full level before starting to fade.");
    addAndMakeVisible (tailFade);
    tailFade.attach (ap.getParameter (ParamIDs::eqGateRelease));
    setHint (tailFade, "Tail fade", "How long the kept bands take to fade to silence. This is what makes a decay sound natural instead of chopped.");

    addAndMakeVisible (tailMeter);
    setHint (tailMeter, "TAIL", "How much of the tail is currently ringing through.");

    selectBand (2);                                     // K1 — the default-enabled band
    updateSoloButtons();
}

TailStage::~TailStage()   { processor.setSoloBand (-1); }

void TailStage::selectBand (int b)
{
    sel = juce::jlimit (0, 6, b);
    canvas.setSelectedBand (sel);
    auto& ap = processor.apvts;
    const bool isKeep = sel >= 2;
    selIsKeep = isKeep;

    freq.attach (ap.getParameter (eqids::freqId (sel)));
    widthK.attach (isKeep ? ap.getParameter (eqids::qId (sel)) : nullptr);
    ring.attach (isKeep ? ap.getParameter (eqids::gainId (sel)) : nullptr);
    slopeK.attach (isKeep ? nullptr : ap.getParameter (eqids::shapeId (sel)));   // hpf/lpf slope

    static const char* labs[7] = { "LOWS", "HIGHS", "K1", "K2", "K3", "K4", "K5" };
    bandLabel.setText (juce::String (labs[sel]) + (sel == 0 ? juce::String::fromUTF8 (" \xe2\x80\x94 keeps everything below")
                                     : sel == 1 ? juce::String::fromUTF8 (" \xe2\x80\x94 keeps everything above")
                                                : juce::String::fromUTF8 (" \xe2\x80\x94 keep band")),
                       juce::dontSendNotification);
    rebuildBandRow();
    rebuildShapeSeg();
}

void TailStage::rebuildBandRow()
{
    for (int b = 0; b < 7; ++b)
    {
        const bool s = b == sel;
        bandBtns[b].setColour (juce::TextButton::buttonColourId, s ? pal->band[b] : pal->btn);
        bandBtns[b].setColour (juce::TextButton::textColourOffId, s ? pal->bg : pal->dim);
    }
}

void TailStage::rebuildShapeSeg()
{
    shapeAtt.reset();
    const bool isKeep = sel >= 2;
    static const char* shapeNames[3] = { "Bell", "Prop Q", "Shelf" };
    static const char* shapeHints[3] = {
        "Bell \xe2\x80\x94 a classic smooth cut. High Q rings one note, low Q keeps broad body.",
        "Proportional Q \xe2\x80\x94 a bell that tightens as the cut deepens: gentle when shallow, surgical when deep.",
        "Band Shelf \xe2\x80\x94 a flat-topped range between two edges, kept at an even level." };

    for (int i = 0; i < 3; ++i)
    {
        auto& b = shapeBtns[i];
        b.setVisible (isKeep);          // LOWS/HIGHS use the Slope knob instead
        if (isKeep)
        {
            b.setButtonText (shapeNames[i]);
            setHint (b, shapeNames[i], juce::String::fromUTF8 (shapeHints[i]));
            b.onClick = [this, i]
            {
                if (auto* p = processor.apvts.getParameter (eqids::shapeId (sel)))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
                    p->endChangeGesture();
                }
            };
        }
    }
    if (isKeep)
        if (auto* p = processor.apvts.getParameter (eqids::shapeId (sel)))
        {
            shapeAtt = std::make_unique<juce::ParameterAttachment> (*p, [this] (float)
            {
                auto* sp = processor.apvts.getParameter (eqids::shapeId (sel));
                const int cur = (int) std::lround (sp->convertFrom0to1 (sp->getValue()));
                for (int i = 0; i < 3; ++i)
                    styleSeg (shapeBtns[i], i == cur);
            });
            shapeAtt->sendInitialUpdate();
        }
    repaint();
}

void TailStage::updateSoloButtons()
{
    const int solo = processor.getSoloBand();
    for (int b = 0; b < 7; ++b)
    {
        soloBtns[b].setColour (juce::TextButton::buttonColourId, solo == b ? pal->warn : pal->btn);
        soloBtns[b].setColour (juce::TextButton::textColourOffId, solo == b ? juce::Colours::white : pal->faint);
    }
}

void TailStage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal->panel);  g.fillRoundedRectangle (r, 5.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (r, 5.0f, 1.0f);

    g.setColour (pal->faint);
    g.setFont (font (9.5f, true));
    g.drawText ("KEEP BANDS", sc (11), bandLabelsY, sc (120), sc (12), juce::Justification::centredLeft);
    if (selIsKeep)
        g.drawText ("SHAPE", shapeX, bandLabelsY, sc (80), sc (12), juce::Justification::centredLeft);
}

void TailStage::resized()
{
    auto r = getLocalBounds().reduced (sc (11), sc (8));
    auto head = r.removeFromTop (sc (20));
    internalsBtn.setBounds (head.removeFromRight (sc (104)));
    head.removeFromRight (sc (7));
    freezeBtn.setBounds (head.removeFromRight (sc (62)));
    head.removeFromRight (sc (4));
    accumBtn.setBounds (head.removeFromRight (sc (88)));
    header.setBounds (head);
    r.removeFromTop (sc (4));

    auto controls = r.removeFromBottom (sc (104));
    r.removeFromBottom (sc (8));
    mon.setBounds (r.removeFromLeft (sc (26)));
    r.removeFromLeft (sc (5));
    canvas.setBounds (r);

    // Column widths shrink proportionally when the stage is narrower than the
    // design width, so the TAIL meter always fits.
    const float design = scf (244 + 13 + 212 + 6 + 86 + 13 + 142 + 6 + 36);
    const float shrink = juce::jmin (1.0f, (float) controls.getWidth() / design);
    auto col = [&] (float px) { return (int) (scf (px) * shrink); };

    bandLabelsY = controls.getY();
    auto strip = controls.removeFromLeft (col (244));
    strip.removeFromTop (sc (14));
    const int colW = strip.getWidth() / 7;
    for (int b = 0; b < 7; ++b)
    {
        const int x = strip.getX() + b * colW;
        bandBtns[b].setBounds (x, strip.getY(), colW - sc (3), sc (22));
        dotBtns[b].setBounds (x + sc (2), strip.getY() + sc (27), sc (13), sc (13));
        soloBtns[b].setBounds (x + sc (17), strip.getY() + sc (26), sc (16), sc (14));
    }
    controls.removeFromLeft (col (13));

    auto bandGrp = controls.removeFromLeft (col (212));
    bandLabel.setFont (font (9.5f, true));
    bandLabel.setColour (juce::Label::textColourId, pal->faint);
    bandLabel.setBounds (bandGrp.removeFromTop (sc (14)));
    const int kw = bandGrp.getWidth() / 3;
    freq.setBounds (bandGrp.removeFromLeft (kw));
    auto wkCell = bandGrp.removeFromLeft (kw);
    widthK.setBounds (wkCell);
    slopeK.setBounds (wkCell);                          // shares the Q knob's cell
    ring.setBounds (bandGrp.removeFromLeft (kw));

    controls.removeFromLeft (col (6));
    shapeX = controls.getX();
    auto shapes = controls.removeFromLeft (col (86));
    shapes.removeFromTop (sc (14));
    for (int i = 0; i < 3; ++i)
        shapeBtns[i].setBounds (shapes.getX(), shapes.getY() + i * sc (26), shapes.getWidth(), sc (23));

    controls.removeFromLeft (col (13));
    auto tailGrp = controls.removeFromLeft (col (142));
    tailGateTg.setBounds (tailGrp.removeFromTop (sc (14)));   // acts as the group header
    const int tw = tailGrp.getWidth() / 2;
    tailHold.setBounds (tailGrp.removeFromLeft (tw));
    tailFade.setBounds (tailGrp.removeFromLeft (tw));

    controls.removeFromLeft (col (6));
    tailMeter.setBounds (controls.removeFromLeft (col (36)));
}

//==============================================================================
RightRail::RightRail (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      fader (proc.apvts.getParameter (ParamIDs::intensity)),
      outMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); }),
      gateMeter ([&proc] (float& o, float& t) { return TailCanvas::gateState (proc, o, t); })
{
    setHint (fader, "AMOUNT", "How much bleed is removed when the gate is closed. 100% = digital silence between hits. 0% = the plugin does nothing. This is the classic gate 'Range' control.");
    addAndMakeVisible (fader);

    amountVal.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (amountVal);
    amtAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::intensity),
        [this] (float v) { amountVal.setText (juce::String ((int) std::round (v)) + " %", juce::dontSendNotification); });
    amtAtt->sendInitialUpdate();

    setHint (outMeter, "OUT", "Plugin output level.");
    addAndMakeVisible (outMeter);
    setHint (gateMeter, "GATE", juce::String::fromUTF8 ("Follows the gate state: green = open (drum passing), amber = tail ringing out and fading, empty = closed \xe2\x80\x94 bleed cancelled."));
    addAndMakeVisible (gateMeter);

    static const char* names[3] = { "Output", "Removed bleed", "Trigger signal" };
    static const char* hints[3] = { "Normal plugin output.",
        "Solo the signal being subtracted. If you hear drum hits in here, the gate is closing too early.",
        "Solo the detector's filtered signal \xe2\x80\x94 what the trigger hears." };
    static const int modes[3] = { MagicDrumDeBleedAudioProcessor::monitorNormal,
                                  MagicDrumDeBleedAudioProcessor::monitorProcessing,
                                  MagicDrumDeBleedAudioProcessor::monitorSidechain };
    for (int i = 0; i < 3; ++i)
    {
        monBtns[i].setButtonText (names[i]);
        setHint (monBtns[i], names[i], juce::String::fromUTF8 (hints[i]));
        monBtns[i].onClick = [this, i]
        {
            auto* p = processor.apvts.getParameter (ParamIDs::monitorMode);
            const int cur = juce::roundToInt (p->convertFrom0to1 (p->getValue()));
            const int target = modes[i];
            monAtt->setValueAsCompleteGesture ((float) ((cur == target && target != 0) ? 0 : target));
        };
        addAndMakeVisible (monBtns[i]);
    }
    monAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::monitorMode),
        [this] (float v)
        {
            const int m = juce::roundToInt (v);
            for (int i = 0; i < 3; ++i)
                styleSeg (monBtns[i], m == modes[i]);
        });
    monAtt->sendInitialUpdate();
}

void RightRail::paint (juce::Graphics& g)
{
    g.setColour (pal->panel);
    g.fillAll();
    g.setColour (pal->line);
    g.fillRect (0, 0, 1, getHeight());
    g.setColour (pal->faint);
    g.setFont (font (9.5f, true));
    g.drawText ("AMOUNT", 0, sc (9), getWidth(), sc (12), juce::Justification::centred);
    g.drawText ("LISTEN TO", 0, listenY, getWidth(), sc (12), juce::Justification::centred);
}

void RightRail::resized()
{
    auto r = getLocalBounds().reduced (sc (9));
    r.removeFromTop (sc (14));

    auto listen = r.removeFromBottom (sc (94));
    listenY = listen.getY();
    listen.removeFromTop (sc (16));
    for (int i = 0; i < 3; ++i)
    {
        monBtns[i].setBounds (listen.removeFromTop (sc (24)));
        listen.removeFromTop (sc (3));
    }
    r.removeFromBottom (sc (6));

    auto meters = r.removeFromBottom (sc (126));
    const int mw = sc (34);                             // OUT and GATE share one width
    auto mrow = meters.withSizeKeepingCentre (mw * 2 + sc (7), meters.getHeight());
    outMeter.setBounds (mrow.removeFromLeft (mw));
    mrow.removeFromLeft (sc (7));
    gateMeter.setBounds (mrow);
    r.removeFromBottom (sc (6));

    amountVal.setFont (font (15.0f, true));
    amountVal.setColour (juce::Label::textColourId, pal->txt);
    amountVal.setBounds (r.removeFromBottom (sc (20)));
    fader.setBounds (r.withSizeKeepingCentre (sc (52), r.getHeight()));
}

//==============================================================================
HintBar::HintBar()
{
    setHint (simpleBtn, "Simple view", "A stripped-back view: trigger level, gate, amount, output, plus Focus, Learn and Tail hold.");
    addAndMakeVisible (simpleBtn);
    startTimerHz (15);
}

void HintBar::timerCallback()
{
    juce::String t, x;
    auto pos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
    if (auto* top = getTopLevelComponent())
        if (auto* c = top->getComponentAt (top->getLocalPoint (nullptr, pos.toInt())))
            ui::findHint (c, t, x);
    if (t != title || x != text)
    {
        title = t; text = x;
        repaint();
    }
}

void HintBar::paint (juce::Graphics& g)
{
    g.fillAll (pal->panel);
    g.setColour (pal->line);
    g.fillRect (0, 0, getWidth(), 1);

    auto r = getLocalBounds().reduced (sc (12), 0).withTrimmedRight (sc (104));
    if (text.isEmpty())
    {
        g.setColour (pal->dim);
        g.setFont (font (11.5f));
        g.drawText (juce::String::fromUTF8 ("Hover any control for an explanation. Drag knobs vertically \xc2\xb7 Shift for fine \xc2\xb7 double-click to reset \xc2\xb7 click a value to type it."),
                    r, juce::Justification::centredLeft);
    }
    else
    {
        juce::AttributedString s;
        s.append (title, font (11.5f, true), pal->txt);
        s.append (juce::String::fromUTF8 (" \xe2\x80\x94 ") + text, font (11.5f), pal->dim);
        s.setJustification (juce::Justification::centredLeft);
        s.setWordWrap (juce::AttributedString::none);
        s.draw (g, r.toFloat());
    }
}

void HintBar::resized()
{
    simpleBtn.setBounds (getLocalBounds().reduced (sc (12), sc (5)).removeFromRight (sc (92)));
}
