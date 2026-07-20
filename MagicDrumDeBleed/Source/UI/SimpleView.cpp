#include "SimpleView.h"
#include "TailCanvas.h"

using namespace ui;

/*  Preset button with a simple vector line drawing of the drum type. */
class SimpleView::DrumButton : public juce::Button
{
public:
    enum Kind { kick = 0, snare, toms };
    DrumButton (Kind k, const juce::String& name) : juce::Button (name), kind (k) {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (over || down ? pal->btnHover : pal->btn);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (pal->line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        const float ih = r.getHeight() - scf (10.0f);
        auto icon = juce::Rectangle<float> (ih * 1.15f, ih)
                        .withPosition (r.getX() + scf (8.0f), r.getY() + scf (5.0f));
        drawIcon (g, icon);

        g.setColour (pal->txt);
        g.setFont (font (11.5f, true));
        g.drawText (getButtonText(), (int) icon.getRight() + sc (6), 0,
                    (int) (r.getRight() - icon.getRight()) - sc (10), getHeight(),
                    juce::Justification::centredLeft);
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
            g.setColour (strong);                           // beater pad + shaft + pedal
            g.fillEllipse (c.x - R * 0.14f, c.y - R * 0.14f, R * 0.28f, R * 0.28f);
            const float bx = c.x + R * 0.55f, by = b.getBottom();
            g.drawLine (bx, by, c.x + R * 0.08f, c.y + R * 0.10f, lw);
            g.drawLine (bx - R * 0.35f, by, bx + R * 0.35f, by, lw);
        }
        else
        {
            // side view: shell with hoops + lugs; snare is shallow with wires,
            // tom is deeper. Tilted top head hinted as a thin ellipse.
            const bool isSnare = kind == snare;
            const float shellH = b.getHeight() * (isSnare ? 0.42f : 0.62f);
            auto shell = juce::Rectangle<float> (b.getX() + b.getWidth() * 0.08f,
                                                 b.getCentreY() - shellH * (isSnare ? 0.38f : 0.48f),
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

            // tilted top head
            juce::Path head;
            head.addEllipse (shell.getX() - hoopOver, shell.getY() - shell.getHeight() * 0.16f,
                             shell.getWidth() + hoopOver * 2, shell.getHeight() * 0.30f);
            g.setColour (line);
            g.strokePath (head, juce::PathStrokeType (lw * 0.8f),
                          juce::AffineTransform::rotation (-0.06f, shell.getCentreX(), shell.getY()));

            if (isSnare)                                    // snare wires under the shell
            {
                g.setColour (strong);
                const float wy = shell.getBottom() + b.getHeight() * 0.10f;
                for (int i = 0; i < 3; ++i)
                    g.drawLine (shell.getCentreX() - shell.getWidth() * 0.18f,
                                wy + (float) i * 1.6f * scale,
                                shell.getCentreX() + shell.getWidth() * 0.18f,
                                wy + (float) i * 1.6f * scale, lw * 0.7f);
            }
        }
    }

    Kind kind;
};

SimpleView::SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView)
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
}

void SimpleView::paint (juce::Graphics& g)
{
    g.fillAll (pal->bg);
    g.setColour (pal->accent);
    g.setFont (font (13.0f, true));
    g.drawText ("MAGIC DRUM GATE", 0, sc (10), getWidth(), sc (16), juce::Justification::centred);
    g.setColour (pal->faint);
    g.setFont (font (9.0f, true));
    g.drawText ("AMOUNT", amountX, sc (34), sc (62), sc (12), juce::Justification::centred);
}

void SimpleView::resized()
{
    auto r = getLocalBounds().reduced (sc (12));
    r.removeFromTop (sc (22));                                // title
    advancedBtn.setBounds (r.removeFromBottom (sc (26)));
    r.removeFromBottom (sc (8));

    // preset row: Kick · Snare · Toms with drum line art
    auto prow = r.removeFromBottom (sc (40));
    r.removeFromBottom (sc (10));
    const int pw = (prow.getWidth() - sc (12)) / 3;
    for (int i = 0; i < 3; ++i)
    {
        presetBtns[i]->setBounds (prow.removeFromLeft (pw));
        prow.removeFromLeft (sc (6));
    }

    // knob row: Resonance (Learn under it) · Reso Amt · Tail hold · Tail fade
    auto knobs = r.removeFromBottom (sc (108));
    r.removeFromBottom (sc (6));
    const int kw = sc (66);
    auto krow = knobs.withSizeKeepingCentre (juce::jmin (kw * 4 + sc (24), knobs.getWidth()),
                                             knobs.getHeight());
    auto rcol = krow.removeFromLeft (kw);
    learnBtn.setBounds (rcol.removeFromBottom (sc (24)));
    rcol.removeFromBottom (sc (4));
    resonance.setBounds (rcol);
    krow.removeFromLeft (sc (8));
    resoAmt.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));
    krow.removeFromLeft (sc (8));
    tailHold.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));
    krow.removeFromLeft (sc (8));
    tailFade.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));

    // centred row: TRIGGER · GATE · AMOUNT · OUT
    const int mw = sc (46), fw = sc (62), gap = sc (14);
    const int total = mw * 3 + fw + gap * 3;
    auto row = r.withSizeKeepingCentre (juce::jmin (total, r.getWidth()), r.getHeight());

    trigMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (gap);
    gateMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (gap);
    auto fcol = row.removeFromLeft (fw);
    amountX = fcol.getX();
    fcol.removeFromTop (sc (14));
    amountVal.setFont (font (12.0f, true));
    amountVal.setColour (juce::Label::textColourId, pal->txt);
    amountVal.setBounds (fcol.removeFromBottom (sc (16)));
    fader.setBounds (fcol);
    row.removeFromLeft (gap);
    outMeter.setBounds (row.removeFromLeft (mw));
}
