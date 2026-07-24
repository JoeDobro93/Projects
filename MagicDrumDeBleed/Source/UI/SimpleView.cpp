#include "SimpleView.h"

using namespace ui;

/*  Preset button with a simple vector line drawing of the drum type. */
class SimpleView::DrumButton : public juce::Button
{
public:
    enum Kind { def = 0, kick, snare, toms };
    DrumButton (Kind k, const juce::String& name) : juce::Button (name), kind (k) {}

    static constexpr float kIconAspect = 1.15f;

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (over || down ? pal->btnHover : pal->btn);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (pal->line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        // icon centred on top, label centred underneath
        auto inner = r.reduced (scf (4.0f));
        auto textArea = inner.removeFromBottom (scf (13.0f));
        inner.removeFromBottom (scf (2.0f));
        const float ih = inner.getHeight();
        auto icon = juce::Rectangle<float> (juce::jmin (ih * kIconAspect, inner.getWidth()), ih)
                        .withCentre ({ inner.getCentreX(), inner.getCentreY() });
        drawIcon (g, icon);

        g.setColour (pal->txt);
        g.setFont (font (11.5f, true));
        g.drawText (getButtonText(), textArea.toNearestInt(), juce::Justification::centred);
    }

private:
    void drawIcon (juce::Graphics& g, juce::Rectangle<float> b)
    {
        const float lw = 1.3f * scale;
        const auto line = pal->dim;
        const auto strong = pal->txt.withAlpha (0.85f);

        if (kind == def)
        {
            // crossed drumsticks — the neutral "any drum" starting point
            auto c = b.getCentre().translated (0.0f, b.getHeight() * 0.04f);
            const float L = juce::jmin (b.getWidth() * 0.87f, b.getHeight()) * 0.5f;
            // one transparency layer so overlapping fills (shaft + round butt
            // + tip bead) composite to a single uniform tone
            g.beginTransparencyLayer (strong.getFloatAlpha());
            g.setColour (pal->txt);
            for (int side = 0; side < 2; ++side)
            {
                const float dir = side == 0 ? -1.0f : 1.0f;
                const juce::Point<float> tip  (c.x + dir * L * 0.74f, c.y - L * 0.76f);
                const juce::Point<float> butt (c.x - dir * L * 0.56f, c.y + L * 0.94f);
                const float ang = std::atan2 (tip.y - butt.y, tip.x - butt.x);
                const juce::Point<float> perp (-std::sin (ang), std::cos (ang));
                const float wb = lw * 1.15f, wt = lw * 0.55f;   // half-widths
                juce::Path shaft;                           // one continuous taper
                shaft.startNewSubPath (butt + perp * wb);
                shaft.lineTo (tip + perp * wt);
                shaft.lineTo (tip - perp * wt);
                shaft.lineTo (butt - perp * wb);
                shaft.closeSubPath();
                g.fillPath (shaft);
                g.fillEllipse (butt.x - wb, butt.y - wb, wb * 2, wb * 2);
                juce::Path bead;                            // elongated acorn tip
                bead.addEllipse (-L * 0.13f, -L * 0.075f, L * 0.26f, L * 0.15f);
                bead.applyTransform (juce::AffineTransform::rotation (ang)
                    .translated (tip.x + std::cos (ang) * L * 0.11f,
                                 tip.y + std::sin (ang) * L * 0.11f));
                g.fillPath (bead);
            }
            g.endTransparencyLayer();
        }
        else if (kind == kick)
        {
            // front view: shell circle, head hoop, radial lugs, beater + pedal
            auto c = b.getCentre().translated (0.0f, -b.getHeight() * 0.04f);
            const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.44f;
            g.setColour (strong);
            g.drawEllipse (c.x - R, c.y - R, R * 2, R * 2, lw);
            g.setColour (line);
            const float r2 = R * 0.72f;
            g.drawEllipse (c.x - r2, c.y - r2, r2 * 2, r2 * 2, lw * 0.8f);
            for (int i = 0; i < 8; ++i)                     // lugs
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
                g.drawLine (c.x + std::cos (a) * r2, c.y + std::sin (a) * r2,
                            c.x + std::cos (a) * R,  c.y + std::sin (a) * R, lw * 0.8f);
            }
            g.setColour (strong);                           // beater pad + vertical shaft + pedal base
            g.fillEllipse (c.x - R * 0.14f, c.y - R * 0.14f, R * 0.28f, R * 0.28f);
            const float by = c.y + R;                       // base level with the shell bottom
            g.drawLine (c.x, by, c.x, c.y + R * 0.14f, lw);
            g.drawLine (c.x - R * 0.38f, by, c.x + R * 0.38f, by, lw * 1.2f);
        }
        else
        {
            // straight side view: shell with overhanging hoops + lugs.
            // Snare = shallow shell; tom = deep shell with legs.
            const bool isSnare = kind == snare;
            const float shellH = b.getHeight() * (isSnare ? 0.44f : 0.66f);
            auto shell = juce::Rectangle<float> (b.getX() + b.getWidth() * 0.08f,
                                                 b.getCentreY() - shellH * (isSnare ? 0.5f : 0.56f),
                                                 b.getWidth() * 0.84f, shellH);
            g.setColour (strong);
            g.drawRect (shell, lw);
            const float hoopOver = b.getWidth() * 0.045f;   // hoops overhang the shell
            g.drawLine (shell.getX() - hoopOver, shell.getY(),
                        shell.getRight() + hoopOver, shell.getY(), lw * 1.3f);
            g.drawLine (shell.getX() - hoopOver, shell.getBottom(),
                        shell.getRight() + hoopOver, shell.getBottom(), lw * 1.3f);

            g.setColour (line);
            for (int i = 0; i < 4; ++i)                     // lugs
            {
                const float x = shell.getX() + shell.getWidth() * (0.16f + 0.226f * (float) i);
                const float m = shell.getHeight() * 0.22f;
                g.drawLine (x, shell.getY() + m, x, shell.getBottom() - m, lw);
            }

            if (! isSnare)                                  // tom legs: straight down the
            {                                               // shell sides, small outward
                g.setColour (strong);                       // foot, then straight again
                const float legTop = shell.getBottom(), legBot = b.getBottom();
                const float jut = shell.getWidth() * 0.06f;
                for (int side = 0; side < 2; ++side)
                {
                    const float x = side == 0 ? shell.getX() : shell.getRight();
                    const float dir = side == 0 ? -1.0f : 1.0f;
                    juce::Path leg;
                    leg.startNewSubPath (x, legTop);
                    leg.lineTo (x, legTop + (legBot - legTop) * 0.45f);
                    leg.lineTo (x + dir * jut, legTop + (legBot - legTop) * 0.72f);
                    leg.lineTo (x + dir * jut, legBot);
                    g.strokePath (leg, juce::PathStrokeType (lw * 0.9f, juce::PathStrokeType::curved,
                                                             juce::PathStrokeType::rounded));
                }
            }
        }
    }

    Kind kind;
};

SimpleView::SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onThemeToggle,
                        std::function<void()> onAdvancedView)
    : processor (proc),
      trigMeter ("TRIGGER", [&proc] { return proc.getDetectorRmsDb(); },
                 proc.apvts.getParameter (ParamIDs::threshold)),
      grMeter ([&proc] { return proc.getGainReductionDb(); }),
      fader (proc.apvts.getParameter (ParamIDs::intensity)),
      outMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); })
{
    setHint (trigMeter, "TRIGGER", "Detector level after the trigger filter. Drag the red line to set the Threshold.");
    addAndMakeVisible (trigMeter);
    setHint (grMeter, "GR", "How hard the hit is escaping the null (compression on the cancelling copy). Empty = bleed fully cancelled.");
    addAndMakeVisible (grMeter);
    setHint (fader, "AMOUNT", "How much bleed is removed when the gate is closed.");
    addAndMakeVisible (fader);
    amountVal.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (amountVal);
    amtAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::intensity),
        [this] (float v) { amountVal.setText (juce::String ((int) std::round (v)) + " %", juce::dontSendNotification); });
    amtAtt->sendInitialUpdate();
    setHint (outMeter, "OUT", "Plugin output level.");
    addAndMakeVisible (outMeter);

    addAndMakeVisible (resonance);
    resonance.attach (proc.apvts.getParameter (ParamIDs::scFreq));
    setHint (resonance, "Resonance", "The drum's ringing frequency — what the detector listens for, and (with Link) what K1 keeps ringing.");
    addAndMakeVisible (resoAmt);
    resoAmt.setFormat ([] (float u) { return juce::String (u, 1); });
    resoAmt.attach (proc.apvts.getParameter (ParamIDs::notchGain (0)));
    setHint (resoAmt, "Reso Amt", "How much of the resonance rings through the tail. 0 = nothing, 20 = maximum.");
    addAndMakeVisible (tailHold);
    tailHold.attach (proc.apvts.getParameter (ParamIDs::eqGateHold));
    setHint (tailHold, "Tail hold", "How long the kept resonance rings at full level before fading.");
    addAndMakeVisible (tailFade);
    tailFade.attach (proc.apvts.getParameter (ParamIDs::eqGateRelease));
    setHint (tailFade, "Tail fade", "How long the kept resonance takes to fade to silence.");
    setHint (learnBtn, "Learn", "Listens for up to 3 seconds, parks the trigger on the detector signal's fundamental (the Sidechain input when Ext SC is on) and sets up the three lowest keep bands from THIS track's loudest resonances.");
    addAndMakeVisible (learnBtn);

    setHint (midiTg, "MIDI trigger", "Notes routed to this track force the gate open for the length of the note - the manual repair path for hits the detector misses.");
    midiTg.onClick = [this]
    {
        auto* p = processor.apvts.getParameter (ParamIDs::midiTrigger);
        midiAtt->setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    midiAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::midiTrigger),
        [this] (float v) { midiTg.setState (v > 0.5f); });
    midiAtt->sendInitialUpdate();
    addAndMakeVisible (midiTg);

    static const char* presetNames[4] = { "Default", "Kick", "Snare", "Toms" };
    for (int i = 0; i < 4; ++i)
    {
        presetBtns[i] = std::make_unique<DrumButton> ((DrumButton::Kind) i, presetNames[i]);
        presetBtns[i]->setButtonText (presetNames[i]);
        setHint (*presetBtns[i], presetNames[i],
                 "Load the " + juce::String (presetNames[i]) + " starting point. Both Thresholds, Ratio, Selectivity, Hysteresis and MIDI are kept.");
        presetBtns[i]->onClick = [this, i]
        {
            for (auto& fp : presets::kFactoryPresets)
                if (juce::String (fp.name) == presetBtns[i]->getButtonText())
                    eqids::applyFactoryPreset (processor, fp);
        };
        addAndMakeVisible (*presetBtns[i]);
    }

    setHint (advancedBtn, "Advanced View", "Back to the full view: trigger filter, gate timing and tail shaping.");
    advancedBtn.onClick = [cb = std::move (onAdvancedView)] { if (cb) cb(); };
    addAndMakeVisible (advancedBtn);

    themeBtn.setButtonText (processor.isDarkTheme() ? "Light" : "Dark");
    setHint (themeBtn, "Theme", "Switch between the dark and light theme.");
    themeBtn.onClick = [cb = std::move (onThemeToggle)] { if (cb) cb(); };
    addAndMakeVisible (themeBtn);
}

void SimpleView::paint (juce::Graphics& g)
{
    g.fillAll (pal->bg);
    const int hh = sc (46);                 // toolbar strip matches the Advanced view
    g.setColour (pal->panel);
    g.fillRect (0, 0, getWidth(), hh);
    g.setColour (pal->line);
    g.fillRect (0, hh - 1, getWidth(), 1);
    drawLogo (g);
    g.setColour (pal->faint);
    g.setFont (font (9.0f, true));
    g.drawText ("AMOUNT", amountX, amountY, sc (62), sc (12), juce::Justification::centred);
    if (! divider.isEmpty())
    {
        g.setColour (pal->line);
        g.fillRect (divider);
    }
}

void SimpleView::resized()
{
    auto full = getLocalBounds();
    auto bar = full.removeFromTop (sc (46));
    bar.reduce (sc (12), sc (8));
    themeBtn.setBounds (bar.removeFromRight (sc (56)));
    auto r = full.reduced (sc (12));
    r.removeFromTop (sc (2));

    // One shared content width — the knob row — aligns every section:
    // meter row edges, preset button edges and knobs all line up.
    const int kw = sc (66);
    const int contentW = juce::jmin (r.getWidth(), kw * 4 + sc (8) * 3);
    const int cx0 = r.getX() + (r.getWidth() - contentW) / 2;

    // preset row at the top, away from the working controls. All three
    // buttons are the same size (widest icon-or-label plus padding),
    // justified so the outer edges meet the content edges.
    auto prow = r.removeFromTop (sc (58));
    r.removeFromTop (sc (9));
    divider = { cx0, r.getY(), contentW, 1 };
    r.removeFromTop (sc (13));
    {
        const auto f = font (11.5f, true);
        const float iconW = scf (58 - 8 - 13 - 2) * 1.15f;
        float widest = iconW;
        for (auto& b : presetBtns)
        {
            juce::GlyphArrangement ga;
            ga.addLineOfText (f, b->getButtonText(), 0.0f, 0.0f);
            widest = juce::jmax (widest, ga.getBoundingBox (0, -1, true).getWidth());
        }
        const int bw = juce::jmin ((int) widest + sc (20), (contentW - sc (18)) / 4);
        for (int i = 0; i < 4; ++i)
            presetBtns[i]->setBounds (cx0 + juce::roundToInt ((contentW - bw) * (float) i / 3.0f),
                                      prow.getY(), bw, prow.getHeight());
    }

    advancedBtn.setBounds (r.removeFromBottom (sc (26)));
    r.removeFromBottom (sc (8));

    // knob row: Resonance (Learn under it) · Reso Amt · Tail hold · Tail fade
    auto knobs = r.removeFromBottom (sc (108));
    r.removeFromBottom (sc (6));
    auto krow = juce::Rectangle<int> (cx0, knobs.getY(), contentW, knobs.getHeight());
    const int kgap = (contentW - kw * 4) / 3;
    auto rcol = krow.removeFromLeft (kw);
    learnBtn.setBounds (rcol.removeFromBottom (sc (24)));
    rcol.removeFromBottom (sc (4));
    resonance.setBounds (rcol);
    const int learnY = learnBtn.getY();
    krow.removeFromLeft (kgap);
    auto c2 = krow.removeFromLeft (kw);
    resoAmt.setBounds (c2.withTrimmedBottom (sc (28)));
    krow.removeFromLeft (kgap);
    auto c3 = krow.removeFromLeft (kw);
    tailHold.setBounds (c3.withTrimmedBottom (sc (28)));
    krow.removeFromLeft (kgap);
    auto c4 = krow.removeFromLeft (kw);
    tailFade.setBounds (c4.withTrimmedBottom (sc (28)));
    midiTg.setBounds (c4.getX(), learnY, kw, sc (24));

    // meter row: TRIGGER · GR · AMOUNT · OUT spread across the content
    // width so its bounding box matches the knobs below
    const int mw = sc (46), fw = sc (62);
    const int mgap = (contentW - mw * 3 - fw) / 3;
    auto row = juce::Rectangle<int> (cx0, r.getY(), contentW, r.getHeight());

    trigMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (mgap);
    grMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (mgap);
    auto fcol = row.removeFromLeft (fw);
    amountX = fcol.getX();
    amountY = fcol.getY();
    fcol.removeFromTop (sc (14));
    amountVal.setFont (font (12.0f, true));
    amountVal.setColour (juce::Label::textColourId, pal->txt);
    amountVal.setBounds (fcol.removeFromBottom (sc (16)));
    fader.setBounds (fcol);
    row.removeFromLeft (mgap);
    outMeter.setBounds (row.removeFromLeft (mw));
}
