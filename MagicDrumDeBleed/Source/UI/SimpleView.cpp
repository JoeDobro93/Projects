#include "SimpleView.h"
#include "TailCanvas.h"

using namespace ui;

/*  Preset button with a simple vector line drawing of the drum type. */
class SimpleView::DrumButton : public juce::Button
{
public:
    enum Kind { kick = 0, snare, toms };
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

        if (kind == kick)
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
      gateMeter ([&proc] (float& o, float& t) { return TailCanvas::gateState (proc, o, t); }),
      fader (proc.apvts.getParameter (ParamIDs::intensity)),
      outMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); })
{
    setHint (trigMeter, "TRIGGER", "Detector level after the trigger filter. Drag the red line to set the Threshold.");
    addAndMakeVisible (trigMeter);
    setHint (gateMeter, "GATE", "Green = open (drum passing), amber = tail fading, empty = closed.");
    addAndMakeVisible (gateMeter);
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
    setHint (learnBtn, "Learn", "Listens for up to 3 seconds and parks the plugin on this drum's dominant frequency.");
    addAndMakeVisible (learnBtn);

    static const char* presetNames[3] = { "Kick", "Snare", "Toms" };
    for (int i = 0; i < 3; ++i)
    {
        presetBtns[i] = std::make_unique<DrumButton> ((DrumButton::Kind) i, presetNames[i]);
        presetBtns[i]->setButtonText (presetNames[i]);
        setHint (*presetBtns[i], presetNames[i],
                 "Load the " + juce::String (presetNames[i]) + " starting point. Threshold is kept.");
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
    g.setColour (pal->accent);
    g.setFont (font (13.0f, true));
    g.drawText ("MAGIC DRUM GATE", 0, sc (10), getWidth(), sc (16), juce::Justification::centred);
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
    auto r = getLocalBounds().reduced (sc (12));
    themeBtn.setBounds (r.getRight() - sc (48), r.getY(), sc (48), sc (20));
    r.removeFromTop (sc (22));                                // title

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
        const int bw = juce::jmin ((int) widest + sc (20), (contentW - sc (12)) / 3);
        presetBtns[0]->setBounds (cx0, prow.getY(), bw, prow.getHeight());
        presetBtns[1]->setBounds (cx0 + (contentW - bw) / 2, prow.getY(), bw, prow.getHeight());
        presetBtns[2]->setBounds (cx0 + contentW - bw, prow.getY(), bw, prow.getHeight());
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
    krow.removeFromLeft (kgap);
    resoAmt.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));
    krow.removeFromLeft (kgap);
    tailHold.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));
    krow.removeFromLeft (kgap);
    tailFade.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));

    // meter row: TRIGGER · GATE · AMOUNT · OUT spread across the content
    // width so its bounding box matches the knobs below
    const int mw = sc (46), fw = sc (62);
    const int mgap = (contentW - mw * 3 - fw) / 3;
    auto row = juce::Rectangle<int> (cx0, r.getY(), contentW, r.getHeight());

    trigMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (mgap);
    gateMeter.setBounds (row.removeFromLeft (mw));
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
