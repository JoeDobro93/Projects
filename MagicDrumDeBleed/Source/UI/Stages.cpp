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
        [this] (float v)
        {
            styleSeg (bypassBtn, v > 0.5f); bypassBtn.repaint();
            const float a = v > 0.5f ? 0.45f : 1.0f;
            threshold.setAlpha (a);
            smoothing.setAlpha (a);
            midiBtn.setAlpha (a);
        });
    bypassAtt->sendInitialUpdate();

    addAndMakeVisible (trigMeter);
    setHint (trigMeter, "TRIGGER", "Detector level — what the plugin is listening to, AFTER the trigger filter. Drag the red line to set the Threshold.");

    addAndMakeVisible (threshold);
    threshold.attach (ap.getParameter (ParamIDs::threshold));
    setHint (threshold, "Threshold", "The level a hit must exceed to open the gate. Anything quieter is treated as bleed and cancelled.");

    addAndMakeVisible (smoothing);
    smoothing.attach (ap.getParameter (ParamIDs::rmsWindow));
    setHint (smoothing, "Smoothing", "How much the detector averages. Opening always uses a fast detector so attacks are never clipped; Smoothing mainly steadies when the gate closes and rejects short spikes of bleed.");

    addAndMakeVisible (midiBtn);
    setHint (midiBtn, "MIDI trigger", "Notes routed to this track force the gate open for the length of the note (hold and release then run as normal) - the manual repair path for hits the detector misses. Any note, any channel.");
    midiBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::midiTrigger);
        midiAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    midiAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::midiTrigger),
        [this] (float v) { midiBtn.setState (v > 0.5f); });
    midiAtt->sendInitialUpdate();

    addAndMakeVisible (focus);
    focus.attach (ap.getParameter (ParamIDs::scFreq));
    setHint (focus, "Focus", "Bandpass: the centre of the band the detector listens to. High/Low Pass: the cutoff frequency.");

    addAndMakeVisible (width);
    setHint (width, "Width", "Bandpass: how wide a band the detector hears. High/Low Pass: how steeply it rolls off.");
    updateWidthKnob();

    addAndMakeVisible (contrastK);
    contrastK.attach (ap.getParameter (ParamIDs::contrast));
    contrastK.setReversed (true);          // Off (24) = empty arc; stricter = fuller
    setHint (contrastK, "Selectivity",
             juce::String::fromUTF8 ("Tom-proofing: a hit only opens the gate if the focused band beats the OFF-BAND rest of the mic (orange trace in the history) by at least this margin at its onset \xe2\x80\x94 and a hit that starts as another drum stays vetoed even when its buzz later leaks into the band. Dial DOWN from Off until other drums stop triggering; on-frequency ghost notes still pass. Engaging it adds 10 ms of latency (the attack-recovery margin)."));

    addAndMakeVisible (typeSel);
    setHint (typeSel, "Trigger Filter Type",
             "High Pass ignores everything below the cutoff (kick-proof). Low Pass ignores everything above (cymbal-proof). Bandpass listens to one band only.");
    typeSel.onChange = [this] (int i) { typeAtt->setValueAsCompleteGesture ((float) i); };
    typeAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::scType),
        [this] (float v) { typeSel.setSelected (juce::roundToInt (v), false); updateWidthKnob(); });
    typeAtt->sendInitialUpdate();

    addAndMakeVisible (enableBtn);
    setHint (enableBtn, "Enabled", "Turn the trigger filter on or off. It narrows what the detector hears so other drums don't open the gate. Never affects the sound itself.");
    enableBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::scEnable);
        scEnableAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    scEnableAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::scEnable),
        [this] (float v)
        {
            filterOn = v > 0.5f;
            enableBtn.setState (filterOn);
            const float a = filterOn ? 1.0f : 0.45f;
            focus.setAlpha (a);
            width.setAlpha (a);
            typeSel.setAlpha (a);
            updateSelectivityDim();
            repaint();
        });
    scEnableAtt->sendInitialUpdate();

    // Mode switch: COMP (left, default) / GATE. Lives here so the mode is
    // set where the detector is configured; the Gate pane retitles itself.
    setHint (modeSw, "Mode", "COMP = a continuous high-ratio compressor on the cancelling copy (smoother, level-tracking). GATE = binary open/close with hold and hysteresis. Each mode remembers its own knob settings.");
    modeSw.onChange = [this] (bool left) { compModeAtt->setValueAsCompleteGesture (left ? 1.0f : 0.0f); };
    addAndMakeVisible (modeSw);

    // Selectivity is gate-only (dims in Comp mode); Smoothing is a separate
    // parameter per mode so switching back and forth keeps both states.
    compModeAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::compMode),
        [this] (float v)
        {
            compOn = v > 0.5f;
            modeSw.setLeftActive (compOn, false);
            smoothing.attach (processor.apvts.getParameter (compOn ? ParamIDs::compRmsWindow
                                                                   : ParamIDs::rmsWindow));
            setHint (smoothing, "Smoothing",
                     compOn ? "The compressor's RMS window (like RMS size in a console compressor). Longer = calmer level tracking, slower response. Separate from gate mode's Smoothing."
                            : "How much the detector averages. Opening always uses a fast detector so attacks are never clipped; Smoothing mainly steadies when the gate closes and rejects short spikes of bleed. Separate from comp mode's Smoothing.");
            updateSelectivityDim();
        });
    compModeAtt->sendInitialUpdate();

    addAndMakeVisible (learnBtn);
    setHint (learnBtn, "Learn", "Listens for up to 3 seconds and parks the filter on this drum's dominant frequency. With Link to K1 on, K1 follows it too.");

    addAndMakeVisible (linkBtn);
    setHint (linkBtn, "Link to K1", "Locks the K1 keep band's frequency to the Focus frequency: turning either knob moves both, and Learn updates them together.");
    linkBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::linkK1);
        linkAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    linkAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::linkK1),
        [this] (float v) { linkBtn.setState (v > 0.5f); });
    linkAtt->sendInitialUpdate();
}

void TriggerStage::updateSelectivityDim()
{
    contrastK.setAlpha (filterOn && ! compOn ? 1.0f : 0.45f);
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
    g.setFont (font (8.5f, true));
    g.drawText ("MODE", modeLabelArea.withTrimmedRight (sc (6)), juce::Justification::centredRight);
    g.setFont (font (9.5f, true));
    g.drawText ("SENSITIVITY", sensLabelX, sc (34), sc (160), sc (12), juce::Justification::centredLeft);
    if (! filterOn) g.setColour (pal->faint.withAlpha (0.45f));
    g.drawText ("TRIGGER FILTER", filtLabelX, sc (34), sc (400), sc (12), juce::Justification::centredLeft);
    g.setColour (pal->faint);
    g.setColour (pal->line);
    g.fillRect (dividerX, sc (32), 1, getHeight() - sc (42));
}

void TriggerStage::resized()
{
    auto r = getLocalBounds().reduced (sc (11), sc (8));
    auto head = r.removeFromTop (sc (20));
    bypassBtn.setBounds (head.removeFromRight (sc (64)).reduced (0, 0));
    head.removeFromRight (sc (10));
    modeSw.setBounds (head.removeFromRight (sc (76)));
    modeLabelArea = head.removeFromRight (sc (40));
    header.setBounds (head);
    r.removeFromTop (sc (4));

    trigMeter.setBounds (r.removeFromLeft (sc (36)));
    r.removeFromLeft (sc (13));

    sensLabelX = r.getX();
    auto sens = r.removeFromLeft (sc (150));
    sens.removeFromTop (sc (14));
    auto sensKnobs = sens.removeFromTop (sc (84));
    threshold.setBounds (sensKnobs.removeFromLeft (sc (70)));
    smoothing.setBounds (sensKnobs.removeFromLeft (sc (70)));
    midiBtn.setBounds (sens.withSizeKeepingCentre (juce::jmin (sens.getWidth(), sc (64)),
                                                   juce::jmin (sens.getHeight(), sc (24))));

    r.removeFromLeft (sc (6));
    dividerX = r.getX();
    r.removeFromLeft (sc (7));
    filtLabelX = r.getX();
    r.removeFromTop (sc (14));

    // button stack (Enabled / Link to K1 / Learn) left, knobs middle, type right
    auto stack = r.removeFromLeft (sc (96));
    enableBtn.setBounds (stack.getX(), stack.getY(),           stack.getWidth(), sc (24));
    linkBtn.setBounds   (stack.getX(), stack.getY() + sc (27), stack.getWidth(), sc (24));
    learnBtn.setBounds  (stack.getX(), stack.getY() + sc (54), stack.getWidth(), sc (24));
    r.removeFromLeft (sc (11));
    focus.setBounds (r.removeFromLeft (sc (70)));
    width.setBounds (r.removeFromLeft (sc (70)));
    contrastK.setBounds (r.removeFromLeft (sc (70)));
    r.removeFromLeft (sc (11));
    typeSel.setBounds (r.removeFromLeft (sc (110)).withHeight (sc (78)));
}

//==============================================================================
GateStage::GateStage (MagicDrumDeBleedAudioProcessor& proc) : processor (proc)
{
    addAndMakeVisible (header);
    auto& ap = processor.apvts;
    addAndMakeVisible (lookahead);
    lookahead.attach (ap.getParameter (ParamIDs::lookahead));
    setHint (lookahead, "Lookahead", "The gate opens this far BEFORE the transient arrives, so attacks are never clipped. This knob never changes the plugin's latency.");
    addAndMakeVisible (hold);
    hold.attach (ap.getParameter (ParamIDs::hold));
    setHint (hold, "Hold", "Minimum time the gate stays fully open after a hit.");
    addAndMakeVisible (release);
    release.attach (ap.getParameter (ParamIDs::release));
    setHint (release, "Release", juce::String::fromUTF8 ("How quickly the gate closes after Hold. Too short chops the drum \xe2\x80\x94 the TAIL stage then takes over."));

    setHint (*this, "History", "Live history. Blue = smoothed detector, bright trace = fast opening detector, orange trace = off-band level (what Selectivity compares against: your drum puts the bright trace ON TOP of the orange, other drums the reverse). Red dashes = Threshold, dimmer dashes = the close level. Background = gate state.");

    addAndMakeVisible (hystK);
    hystK.attach (ap.getParameter (ParamIDs::hysteresis));
    setHint (hystK, "Hysteresis", "The gate only closes once the detector has fallen this far below the Threshold (second dashed line) while still falling. Bigger = marginal hits ring out longer.");

    addAndMakeVisible (ratioK);
    ratioK.attach (ap.getParameter (ParamIDs::compRatio));
    setHint (ratioK, "Ratio", "Compression ratio on the cancelling copy. Standard ratios squeeze it toward the Threshold — hits escape the null partially attenuated (the classic trick; 100:1 ≈ limiter). Mirror is a negative ratio (−1:1): every dB over pushes the cancelling copy a dB UNDER the threshold, so hits escape nearly untouched — the most gate-like setting.");
    addAndMakeVisible (compAtk);
    compAtk.attach (ap.getParameter (ParamIDs::compAttack));
    setHint (compAtk, "Attack", "How fast the compressor clamps down once a hit crosses the Threshold - the speed the hit opens. Lookahead gives it a head start.");
    addAndMakeVisible (compRel);
    compRel.attach (ap.getParameter (ParamIDs::compRelease));
    setHint (compRel, "Release", "How fast bleed removal re-engages as the hit falls back under the Threshold.");

    modeAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::compMode),
        [this] (float v) { applyMode (v > 0.5f); });

    addAndMakeVisible (speedSlider);
    setHint (speedSlider, "History speed", "How fast the detector history scrolls. Double-click resets.");
    speedSlider.onChange = [this] (float v)
    {
        const float hz = v < 0.5f ? 30.0f * std::pow (9.0f, v)
                                  : 90.0f * std::pow (4.0f, 2.0f * v - 1.0f);
        startTimerHz (juce::roundToInt (hz));
    };

    auto traceToggle = [this] (ui::LightToggle& t, bool& flag, const char* title, const char* hint)
    {
        t.setState (true);
        setHint (t, title, hint);
        t.onClick = [this, &t, &flag] { flag = ! flag; t.setState (flag); repaint(); };
        addAndMakeVisible (t);
    };
    traceToggle (outTg, showOut, "Output",
                 "The plugin's actual output level (green trace) - what survives the cancellation, after the tail EQ.");
    traceToggle (trigTg, showTrig, "Trigger",
                 "The fast opening detector (bright trace) - the level that actually fires the gate.");
    traceToggle (smoothTg, showSmooth, "Smoothed",
                 "The smoothed detector (blue) - Smoothing's averaged level; it closes the gate and is compared to the Threshold.");
    traceToggle (offTg, showOff, "Off-band",
                 "Everything OUTSIDE the trigger filter's band (orange) - what Selectivity compares against.");

    addAndMakeVisible (histFreeze);
    setHint (histFreeze, "Freeze", "Freeze the history so you can inspect it.");

    dimAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::compBypass),
        [this] (float v)
        {
            const float a = v > 0.5f ? 0.45f : 1.0f;
            lookahead.setAlpha (a);
            hold.setAlpha (a);
            release.setAlpha (a);
            hystK.setAlpha (a);
            ratioK.setAlpha (a);
            compAtk.setAlpha (a);
            compRel.setAlpha (a);
        });
    dimAtt->sendInitialUpdate();
    modeAtt->sendInitialUpdate();

    hist.reserve (kHist);
    startTimerHz (90);
}

void GateStage::applyMode (bool comp)
{
    compOn = comp;
    header.setTitle (comp ? "COMPRESSOR" : "GATE");
    hold.setVisible (! comp);
    hystK.setVisible (! comp);
    release.setVisible (! comp);
    ratioK.setVisible (comp);
    compAtk.setVisible (comp);
    compRel.setVisible (comp);

    trigTg.setVisible (! comp);
    smoothTg.setText (comp ? "Input" : "Average");
    if (comp)
    {
        setHint (smoothTg, "Input", "The smoothed detector level (blue) the compressor responds to - compared against the Threshold.");
        setHint (*this, "History", "Live history. Green = the actual output escaping the null, blue = the smoothed input driving the compressor, orange = the raw mic. Red dashes = Threshold. The strip along the bottom shows activity: green = reduction engaged (hit passing), gold = tail ringing.");
    }
    else
    {
        setHint (smoothTg, "Average", "The smoothed input (blue) - Smoothing's averaged level; it closes the gate and is compared to the Threshold.");
        setHint (*this, "History", "Live history. Green = the actual output, bright trace = the fast level that fires the gate, blue = averaged input, orange = the raw mic (Selectivity vetoes hits whose orange towers over the bright trace). Red dashes = Threshold, dimmer dashes = the close level. The strip along the bottom shows the gate: green = open, gold = tail.");
    }
    setHint (trigTg, "Input", "The fast input level (bright trace) - the level that actually fires the gate.");
    setHint (offTg, "Dry", "The raw, unfiltered mic level (orange) - everything the mic hears, before the trigger filter.");
    resized();
    repaint();
}

void GateStage::timerCallback()
{
    state = TailCanvas::gateState (processor, open01, tail01);

    if (! histFreeze.getToggleState())
    {
        hist.push_back ({ processor.getDetectorRmsDb(), processor.getFastDetectorDb(),
                          processor.getDryDb(), processor.getOutputPeakDb(),
                          juce::jlimit (0.0f, 1.0f, -processor.getGainReductionDb() / 48.0f),
                          open01, tail01, state, processor.getMidiForced() });
        if ((int) hist.size() > kHist)
            hist.erase (hist.begin(), hist.begin() + ((int) hist.size() - kHist));
    }

    repaint();
}

void GateStage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal->panel);  g.fillRoundedRectangle (r, 5.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (r, 5.0f, 1.0f);
    if (! speedLabelArea.isEmpty())     // reads bottom-up beside the vertical slider
    {
        juce::Graphics::ScopedSaveState ss (g);
        const auto c = speedLabelArea.getCentre().toFloat();
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, c.x, c.y));
        g.setFont (font (8.5f, true));
        g.drawText ("SPEED", juce::Rectangle<int> (sc (70), sc (12)).withCentre (speedLabelArea.getCentre()),
                    juce::Justification::centred);
    }

    // ---- history canvas ----
    auto cv = canvasArea.toFloat();
    g.setColour (pal->panel2); g.fillRoundedRectangle (cv, 4.0f);
    const int n = (int) hist.size();
    if (n > 1)
    {
        const float cw = cv.getWidth() / (float) kHist;
        // ---- activity lane along the bottom: the traces' floor is raised a
        // few px and the freed strip shows WHEN the gate/reduction and tail
        // are active (green/gold, brightness follows the envelopes) without
        // painting over the peaks. Shared by both modes.
        const float laneH = scf (8.0f);
        const auto lane = juce::Rectangle<float> (cv.getX() + 1.0f, cv.getBottom() - laneH - 1.0f,
                                                  cv.getWidth() - 2.0f, laneH);
        const auto plot = cv.withTrimmedBottom (laneH + 3.0f);
        for (int i = 0; i < n; ++i)
        {
            const auto& s = hist[(size_t) i];
            if (s.t01 <= 0.004f && s.o01 <= 0.004f)
                continue;
            const float x = cv.getRight() - (float) (n - i) * cw;
            if (s.t01 > 0.004f)                          // tail: gold, envelope-true
            {
                g.setColour (pal->tail.withAlpha (0.25f + 0.65f * s.t01));
                g.fillRect (x, lane.getY(), cw + 0.6f, lane.getHeight());
            }
            if (s.o01 > 0.004f)                          // gate / reduction on top
            {
                g.setColour ((s.forced ? pal->accent : pal->open).withAlpha (0.30f + 0.70f * s.o01));
                g.fillRect (x, lane.getY(), cw + 0.6f, lane.getHeight());
            }
        }
        g.setColour (pal->line.withAlpha (0.7f));
        g.drawHorizontalLine ((int) (lane.getY() - 1.0f), cv.getX() + 1.0f, cv.getRight() - 1.0f);
        auto yFor = [&] (float db) { return plot.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * plot.getHeight(); };
        // threshold dashes + the close level (threshold - hysteresis) beneath
        if (auto* tp = processor.apvts.getParameter (ParamIDs::threshold))
        {
            const float thr = tp->convertFrom0to1 (tp->getValue());
            const float dash[2] = { 5.0f, 4.0f };
            if (auto* hp = processor.apvts.getParameter (ParamIDs::hysteresis); hp != nullptr && ! compOn)
            {
                const float cy2 = yFor (thr - hp->convertFrom0to1 (hp->getValue()));
                g.setColour (pal->warn.withAlpha (0.45f));
                const float dash2[2] = { 2.0f, 4.0f };
                g.drawDashedLine ({ cv.getX(), cy2, cv.getRight(), cy2 }, dash2, 2, 1.0f);
            }
            const float ty = yFor (thr);
            g.setColour (pal->warn);
            g.drawDashedLine ({ cv.getX(), ty, cv.getRight(), ty }, dash, 2, 1.2f);
        }
        juce::Path line, fill, fastLine, offLine, outLine;
        fill.startNewSubPath (cv.getRight() - (float) n * cw, plot.getBottom());
        for (int i = 0; i < n; ++i)
        {
            const float x = cv.getRight() - (float) (n - i) * cw, y = yFor (hist[(size_t) i].det);
            i == 0 ? line.startNewSubPath (x, y) : line.lineTo (x, y);
            fill.lineTo (x, y);
            const float fy = yFor (hist[(size_t) i].fast);
            i == 0 ? fastLine.startNewSubPath (x, fy) : fastLine.lineTo (x, fy);
            const float oy = yFor (hist[(size_t) i].off);
            i == 0 ? offLine.startNewSubPath (x, oy) : offLine.lineTo (x, oy);
            const float uy = yFor (hist[(size_t) i].out);
            i == 0 ? outLine.startNewSubPath (x, uy) : outLine.lineTo (x, uy);
        }
        fill.lineTo (cv.getRight(), plot.getBottom());
        fill.closeSubPath();
        if (showSmooth)
        {
            if (! compOn) { g.setColour (pal->accent.withAlpha (0.22f)); g.fillPath (fill); }
            g.setColour (pal->accent);
            g.strokePath (line, juce::PathStrokeType (compOn ? 1.1f : 1.4f));
        }
        // raw mic level (Dry) — in gate mode also the Selectivity reference
        // (tune by eye: your drum lifts the bright trace above it)
        if (showOff)
        {
            g.setColour (pal->tail.withAlpha (0.6f));
            g.strokePath (offLine, juce::PathStrokeType (0.9f));
        }
        // gate mode only: the fast opening detector — what fires the gate
        if (showTrig && ! compOn)
        {
            g.setColour (pal->accent.interpolatedWith (pal->txt, 0.65f).withAlpha (0.85f));
            g.strokePath (fastLine, juce::PathStrokeType (0.9f));
        }
        // comp mode: the OUTPUT layer in front — what escapes the null.
        // Fill deepens with gain reduction and turns gold as the tail takes
        // over (GR released, tail envelope still ringing).
        // the actual output level (post-EQ) — both modes, in front
        if (showOut)
        {
            g.setColour (pal->open);
            g.strokePath (outLine, juce::PathStrokeType (1.3f));
        }
    }
    g.setColour (pal->line); g.drawRoundedRectangle (cv, 4.0f, 1.0f);
    g.setColour (pal->faint); g.setFont (font (10.5f));
    g.drawText ("detector history", canvasArea.getX() + sc (8), canvasArea.getY() + sc (4), sc (140), sc (12),
                juce::Justification::centredLeft);

    // ---- state box (gate mode only; comp mode gives the room to the chart) ----
    if (compOn || stateArea.isEmpty())
        return;
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
    header.setBounds (r.removeFromTop (sc (20)));
    r.removeFromTop (sc (7));

    auto knobs = r.removeFromLeft (sc (140));
    knobs.removeFromTop (sc (2));
    auto krow1 = knobs.removeFromTop (sc (84));
    lookahead.setBounds (krow1.removeFromLeft (sc (70)));
    const auto slot2 = krow1.removeFromLeft (sc (70));
    hystK.setBounds (slot2);
    ratioK.setBounds (slot2);
    auto krow2 = knobs;
    const auto slot3 = krow2.removeFromLeft (sc (70));
    hold.setBounds (slot3);
    compAtk.setBounds (slot3);
    const auto slot4 = krow2.removeFromLeft (sc (70));
    release.setBounds (slot4);
    compRel.setBounds (slot4);

    r.removeFromLeft (sc (13));
    if (compOn)
        stateArea = {};                     // comp mode: the chart takes the room
    else
    {
        stateArea = r.removeFromRight (sc (124));
        r.removeFromRight (sc (8));
    }
    auto ctop = r.removeFromTop (sc (18));              // trace toggles above the chart
    outTg.setBounds (ctop.removeFromLeft (sc (70)));
    if (! compOn)
        trigTg.setBounds (ctop.removeFromLeft (sc (62)));
    smoothTg.setBounds (ctop.removeFromLeft (sc (78)));
    offTg.setBounds (ctop.removeFromLeft (sc (56)));
    auto scol = r.removeFromRight (sc (16));
    speedLabelArea = r.removeFromRight (sc (12));
    r.removeFromRight (sc (2));
    canvasArea = r;
    speedSlider.setBounds (scol.reduced (0, sc (2)));
    histFreeze.setBounds (canvasArea.getRight() - sc (24), canvasArea.getY() + sc (5), sc (19), sc (19));
}

//==============================================================================
TailStage::TailStage (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      canvas (proc, [this] (int b) { selectBand (b); }),
      tailMeter ([&proc] { return std::pow (10.0f, proc.getEqGateReductionDb() / 20.0f); })
{
    addAndMakeVisible (header);
    auto& ap = processor.apvts;

    addAndMakeVisible (bypassBtn);
    setHint (bypassBtn, "Bypass", "Bypass the tail EQ: no keep bands, so the gate cuts every frequency equally and nothing rings through.");
    bypassBtn.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::eqBypass);
        bypassAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    bypassAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::eqBypass),
        [this] (float v) { styleSeg (bypassBtn, v > 0.5f); eqByp = v > 0.5f; updateDim(); });
    bypassAtt->sendInitialUpdate();

    addAndMakeVisible (scaleSel);
    setHint (scaleSel, "Scale", "Vertical zoom of the gold band curves: Fine = 12 dB, Med = 18 dB, Wide = 24 dB of cut. Deep tips may go offscreen; dragging above the display still works.");
    scaleSel.onChange = [this] (int i)
    {
        canvas.setDisplayScale (i == 0 ? 12.0f : i == 1 ? 18.0f : 24.0f);
        processor.apvts.state.setProperty ("tailScale", i, nullptr);
    };
    {
        const int i = (int) processor.apvts.state.getProperty ("tailScale", 2);
        scaleSel.setSelected (i, false);
        canvas.setDisplayScale (i == 0 ? 12.0f : i == 1 ? 18.0f : 24.0f);
    }

    addAndMakeVisible (tailGateSw);
    tailGateSw.setOnColour (pal->tail);
    setHint (tailGateSw, "Tail gate", juce::String::fromUTF8 ("When off, the kept bands ring continuously \xe2\x80\x94 including between hits, where they will rumble. Normally leave this on."));
    tailGateSw.onChange = [this] (bool on) { tailGateAtt->setValueAsCompleteGesture (on ? 1.0f : 0.0f); };
    tailGateAtt = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (ParamIDs::eqGateOn),
        [this] (float v) { tailGateSw.setState (v > 0.5f, false); tgOn = v > 0.5f; updateDim(); });
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
    freezeBtn.onClick = [this] { canvas.setFrozen (freezeBtn.getToggleState()); };

    addAndMakeVisible (mon);
    setHint (mon, "Monitor gain", juce::String::fromUTF8 ("Boosts or cuts the dry/kept spectrum displays only \xe2\x80\x94 never the audio. Handy for quiet sources. Double-click resets."));
    mon.onChange = [this] (float db) { canvas.setMonitorGain (db); };

    addAndMakeVisible (canvas);
    setHint (canvas, "TAIL display", "Blue = the dry signal. Orange = what survives once the EQ'd cancellation copy is subtracted. Drag a handle: sideways = frequency, up/down = how loud it rings. Mouse-wheel = Q. Click the dry/kept legend to hide a layer.");

    static const char* labs[7] = { "LP", "HP", "K1", "K2", "K3", "K4", "K5" };
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

        bandSw[b].setOnColour (pal->band[b]);
        setHint (bandSw[b], labs[b], "Enable / disable this keep band.");
        bandSw[b].onChange = [this, b] (bool on)
        {
            onAtts[b]->setValueAsCompleteGesture (on ? 1.0f : 0.0f);
        };
        addAndMakeVisible (bandSw[b]);
        onAtts[b] = std::make_unique<juce::ParameterAttachment> (*ap.getParameter (eqids::onId (b)),
            [this, b] (float v) { bandSw[b].setState (v > 0.5f, false); });
        onAtts[b]->sendInitialUpdate();

        soloBtns[b].setButtonText ("SOLO");
        setHint (soloBtns[b], "Solo", juce::String::fromUTF8 ("Audition ONLY this band's kept ring \xe2\x80\x94 no gate, no tail gate, no Amount \xe2\x80\x94 to find the right frequency."));
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
    slopeK.setFormat ([] (float v)
    {
        static const int s[5] = { 6, 12, 24, 36, 48 };
        return juce::String (s[juce::jlimit (0, 4, (int) std::lround (v))]) + " dB/oct";
    });
    setHint (slopeK, "Slope", "How steeply frequencies beyond the cutoff stop ringing through.");
    addAndMakeVisible (ring);
    ring.setFormat ([] (float u) { return juce::String (u, 1); });
    setHint (ring, "Ring level", "How much this band rings through during the tail. 0 = nothing, 20 = maximum ring.");

    addChildComponent (shapeSel);
    setHint (shapeSel, "Shape", "Bell = classic smooth cut. Prop Q = tightens as it deepens. Shelf = flat-topped range at an even level.");
    shapeSel.onChange = [this] (int i)
    {
        if (auto* p = processor.apvts.getParameter (eqids::shapeId (sel)))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
            p->endChangeGesture();
        }
    };

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

    static const char* labs[7] = { "LP", "HP", "K1", "K2", "K3", "K4", "K5" };
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
    shapeSel.setVisible (isKeep);       // LP/HP use the Slope knob instead
    if (isKeep)
        if (auto* p = processor.apvts.getParameter (eqids::shapeId (sel)))
        {
            shapeAtt = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v)
            {
                shapeSel.setSelected (juce::roundToInt (v), false);
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

    g.setColour (eqByp ? pal->faint.withAlpha (0.45f) : pal->faint);
    g.setFont (font (9.5f, true));
    g.drawText ("KEEP BANDS", sc (11), bandLabelsY, sc (120), sc (12), juce::Justification::centredLeft);
    if (selIsKeep)
        g.drawText ("SHAPE", shapeX, bandLabelsY, sc (80), sc (12), juce::Justification::centredLeft);
    g.setColour (pal->faint);
    g.drawText ("SCALE", sc (11), scaleLabelY, sc (52), sc (12), juce::Justification::centredLeft);
}

void TailStage::updateDim()
{
    // Bypassed EQ dulls every sound-shaping control; the tail knobs also dim
    // when the tail gate is off (they only time the gated tail).
    const float band = eqByp ? 0.45f : 1.0f;
    const float tail = (eqByp || ! tgOn) ? 0.45f : 1.0f;
    for (int b = 0; b < 7; ++b)
    {
        bandBtns[b].setAlpha (band);
        soloBtns[b].setAlpha (band);
        bandSw[b].setAlpha (band);
    }
    bandLabel.setAlpha (band);
    freq.setAlpha (band);
    widthK.setAlpha (band);
    ring.setAlpha (band);
    slopeK.setAlpha (band);
    shapeSel.setAlpha (band);
    tailGateSw.setAlpha (band);
    tailHold.setAlpha (tail);
    tailFade.setAlpha (tail);
    repaint();
}

void TailStage::resized()
{
    auto r = getLocalBounds().reduced (sc (11), sc (8));
    auto head = r.removeFromTop (sc (20));
    internalsBtn.setBounds (head.removeFromRight (sc (104)));
    head.removeFromRight (sc (7));
    accumBtn.setBounds (head.removeFromRight (sc (88)));
    head.removeFromRight (sc (7));
    bypassBtn.setBounds (head.removeFromRight (sc (64)));
    header.setBounds (head);
    r.removeFromTop (sc (4));

    auto controls = r.removeFromBottom (sc (104));
    r.removeFromBottom (sc (8));

    // left of the canvas: SCALE selector column, then the MON fader spanning
    // the full canvas height between the selector and the display
    auto leftCol = r.removeFromLeft (sc (56));
    scaleLabelY = leftCol.getY();
    leftCol.removeFromTop (sc (14));
    scaleSel.setBounds (leftCol.removeFromTop (sc (54)));
    r.removeFromLeft (sc (5));
    mon.setBounds (r.removeFromRight (sc (26)));
    r.removeFromRight (sc (4));
    canvas.setBounds (r);
    freezeBtn.setBounds (r.getRight() - sc (24), r.getY() + sc (5), sc (19), sc (19));
    freezeBtn.toFront (false);

    // Three anchored groups: keep bands left, band settings centred, tail
    // gate + meter right. Widths shrink proportionally when space is tight.
    const float leftW = scf (244), centreW = scf (212 + 6 + 96), rightW = scf (142 + 6 + 36);
    const float gap = scf (10);
    const float shrink = juce::jmin (1.0f, (float) controls.getWidth() / (leftW + centreW + rightW + gap * 2));
    auto col = [&] (float px) { return (int) (scf (px) * shrink); };

    bandLabelsY = controls.getY();
    auto strip = juce::Rectangle<int> (controls.getX(), controls.getY(), (int) (leftW * shrink), controls.getHeight());
    strip.removeFromTop (sc (14));
    const int colW = strip.getWidth() / 7;
    for (int b = 0; b < 7; ++b)
    {
        const int x = strip.getX() + b * colW;
        bandSw[b].setBounds (x + (colW - sc (20)) / 2, strip.getY(), sc (20), sc (10));
        bandBtns[b].setBounds (x, strip.getY() + sc (13), colW - sc (3), sc (22));
        soloBtns[b].setBounds (x, strip.getY() + sc (38), colW - sc (3), sc (13));
    }

    auto right = juce::Rectangle<int> (controls.getRight() - (int) (rightW * shrink), controls.getY(),
                                       (int) (rightW * shrink), controls.getHeight());
    auto tailGrp = right.removeFromLeft (col (142));
    tailGrp.removeFromTop (sc (14));                          // aligns with the other knob groups
    auto swRow = tailGrp.removeFromBottom (sc (16));
    tailGateSw.setBounds (swRow.withSizeKeepingCentre (juce::jmin (swRow.getWidth(), sc (96)), sc (16)));
    const int tw = tailGrp.getWidth() / 2;
    tailHold.setBounds (tailGrp.removeFromLeft (tw));
    tailFade.setBounds (tailGrp.removeFromLeft (tw));
    right.removeFromLeft (col (6));
    tailMeter.setBounds (right.removeFromLeft (col (36)));

    const int cw = (int) (centreW * shrink);
    const int cx = juce::jlimit (strip.getRight() + (int) (gap * shrink),
                                 juce::jmax (strip.getRight() + (int) (gap * shrink),
                                             controls.getRight() - (int) (rightW * shrink) - (int) (gap * shrink) - cw),
                                 controls.getX() + (controls.getWidth() - cw) / 2);
    auto centre = juce::Rectangle<int> (cx, controls.getY(), cw, controls.getHeight());

    auto bandGrp = centre.removeFromLeft (col (212));
    bandLabel.setFont (font (9.5f, true));
    bandLabel.setColour (juce::Label::textColourId, pal->faint);
    bandLabel.setBounds (bandGrp.removeFromTop (sc (14)));
    const int kw = bandGrp.getWidth() / 3;
    freq.setBounds (bandGrp.removeFromLeft (kw));
    auto wkCell = bandGrp.removeFromLeft (kw);
    widthK.setBounds (wkCell);
    slopeK.setBounds (wkCell);                          // shares the Q knob's cell
    ring.setBounds (bandGrp.removeFromLeft (kw));

    centre.removeFromLeft (col (6));
    shapeX = centre.getX();
    auto shapes = centre.removeFromLeft (col (96));
    shapes.removeFromTop (sc (14));
    shapeSel.setBounds (shapes.withHeight (sc (78)));
}

//==============================================================================
RightRail::RightRail (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      fader (proc.apvts.getParameter (ParamIDs::intensity)),
      outMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); }),
      gateMeter ([&proc] (float& o, float& t) { return TailCanvas::gateState (proc, o, t); }),
      grMeter ([&proc] { return proc.getGainReductionDb(); })
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
    setHint (grMeter, "GR", juce::String::fromUTF8 ("Compression on the cancelling copy, down to \xe2\x88\x92" "48 dB \xe2\x80\x94 depth means the hit is escaping the null. Empty = no reduction, bleed fully cancelled."));
    addChildComponent (grMeter);
    modeAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::compMode),
        [this] (float v)
        {
            gateMeter.setVisible (v <= 0.5f);
            grMeter.setVisible (v > 0.5f);
            updateLatencyText();
        });
    modeAtt->sendInitialUpdate();

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

    // Latency readout: base window, + the flam-recovery margin only while
    // Selectivity is engaged in gate mode.
    contrastAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::contrast),
        [this] (float) { updateLatencyText(); });
    updateLatencyText();
}

void RightRail::updateLatencyText()
{
    const bool comp = processor.apvts.getRawParameterValue (ParamIDs::compMode)->load() > 0.5f;
    const bool sel = ! comp && processor.apvts.getRawParameterValue (ParamIDs::contrast)->load() < 23.75f;
    latencyText = juce::String (MagicDrumDeBleedAudioProcessor::kMaxLookaheadMs
                                + (sel ? MagicDrumDeBleedAudioProcessor::kEnvMarginMs : 0)) + " ms latency";
    repaint();
}

void RightRail::paint (juce::Graphics& g)
{
    g.setColour (pal->panel);
    g.fillAll();
    g.setColour (pal->line);
    g.fillRect (0, 0, 1, getHeight());
    g.setColour (pal->faint);
    g.setFont (font (9.5f, true));
    g.drawText ("AMOUNT", amountX, amountY, sc (52), sc (12), juce::Justification::centred);
    g.drawText ("LISTEN TO", 0, listenY, getWidth(), sc (12), juce::Justification::centred);
    g.setColour (pal->faint);
    g.setFont (font (9.5f));
    g.drawText (latencyText, 0, latencyY, getWidth(), sc (12), juce::Justification::centred);
}

void RightRail::resized()
{
    auto r = getLocalBounds().reduced (sc (9));
    r.removeFromTop (sc (4));

    auto listen = r.removeFromBottom (sc (110));
    listenY = listen.getY();
    listen.removeFromTop (sc (16));
    for (int i = 0; i < 3; ++i)
    {
        monBtns[i].setBounds (listen.removeFromTop (sc (24)));
        listen.removeFromTop (sc (3));
    }
    latencyY = listen.getY() + sc (1);
    r.removeFromBottom (sc (6));

    // fader column (AMOUNT caption above, % value below, both fader-width)
    // beside the OUT / GATE meters, which span the full row height
    const int fw = sc (52), mw = sc (34), rowW = fw + sc (8) + mw * 2 + sc (7);
    auto row = r.withSizeKeepingCentre (juce::jmin (rowW, r.getWidth()), r.getHeight());
    auto fcol = row.removeFromLeft (fw);
    amountX = fcol.getX();
    amountY = fcol.getY();
    fcol.removeFromTop (sc (14));
    amountVal.setFont (font (13.0f, true));
    amountVal.setColour (juce::Label::textColourId, pal->txt);
    amountVal.setBounds (fcol.removeFromBottom (sc (16)));
    fader.setBounds (fcol);
    row.removeFromLeft (sc (8));
    outMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (sc (7));
    const auto meterCell = row.removeFromLeft (mw);
    gateMeter.setBounds (meterCell);
    grMeter.setBounds (meterCell);
}

//==============================================================================
HintBar::HintBar()
{
    setHint (simpleBtn, "Simple view", "A stripped-back view: trigger level, gate, amount, output, plus Focus, Learn and Tail hold.");
    addAndMakeVisible (simpleBtn);
    juce::Desktop::getInstance().addGlobalMouseListener (this);
    startTimerHz (15);
}

HintBar::~HintBar()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
}

void HintBar::mouseDown (const juce::MouseEvent& e)   { pressed = e.eventComponent; }
void HintBar::mouseUp (const juce::MouseEvent&)       { pressed = nullptr; }

void HintBar::timerCallback()
{
    juce::String t, x;
    if (auto* top = getTopLevelComponent())
    {
        if (pressed != nullptr)                 // a control is being held/dragged
        {
            ui::findHint (pressed.getComponent(), t, x);
        }
        else if (auto* peer = top->getPeer(); peer != nullptr && peer->isFocused())
        {
            const auto pos = top->getLocalPoint (nullptr,
                juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().toInt());
            if (top->getLocalBounds().contains (pos))
                if (auto* c = top->getComponentAt (pos))
                    ui::findHint (c, t, x);
        }
    }
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
