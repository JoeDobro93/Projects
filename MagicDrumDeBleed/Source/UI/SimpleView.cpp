#include "SimpleView.h"
#include "TailCanvas.h"

using namespace ui;

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
    setHint (learnBtn, "Learn", "Listens for up to 3 seconds and parks the plugin on this drum's dominant frequency.");
    addAndMakeVisible (learnBtn);

    static const char* presetNames[3] = { "Kick", "Snare", "Toms" };
    for (int i = 0; i < 3; ++i)
    {
        presetBtns[i].setButtonText (presetNames[i]);
        setHint (presetBtns[i], presetNames[i],
                 "Load the " + juce::String (presetNames[i]) + " starting point. Threshold is kept.");
        presetBtns[i].onClick = [this, i]
        {
            for (auto& fp : presets::kFactoryPresets)
                if (juce::String (fp.name) == presetBtns[i].getButtonText())
                    eqids::applyFactoryPreset (processor, fp);
        };
        addAndMakeVisible (presetBtns[i]);
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

    // preset row: Kick · Snare · Toms
    auto prow = r.removeFromBottom (sc (26));
    r.removeFromBottom (sc (10));
    const int pw = (prow.getWidth() - sc (12)) / 3;
    for (int i = 0; i < 3; ++i)
    {
        presetBtns[i].setBounds (prow.removeFromLeft (pw));
        prow.removeFromLeft (sc (6));
    }

    // knob row: Resonance (Learn under it) · Reso Amt · Tail hold
    auto knobs = r.removeFromBottom (sc (108));
    r.removeFromBottom (sc (6));
    const int kw = sc (70);
    auto krow = knobs.withSizeKeepingCentre (juce::jmin (kw * 3 + sc (20), knobs.getWidth()),
                                             knobs.getHeight());
    auto rcol = krow.removeFromLeft (kw);
    learnBtn.setBounds (rcol.removeFromBottom (sc (24)));
    rcol.removeFromBottom (sc (4));
    resonance.setBounds (rcol);
    krow.removeFromLeft (sc (10));
    resoAmt.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));
    krow.removeFromLeft (sc (10));
    tailHold.setBounds (krow.removeFromLeft (kw).withTrimmedBottom (sc (28)));

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
