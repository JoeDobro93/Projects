#include "SimpleView.h"

using namespace ui;

SimpleView::SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView)
    : processor (proc),
      trigMeter ("TRIGGER", [&proc] { return proc.getDetectorRmsDb(); },
                 proc.apvts.getParameter (ParamIDs::threshold)),
      redMeter ([&proc] { return proc.getRemovedPeakDb(); }),
      fader (proc.apvts.getParameter (ParamIDs::intensity)),
      outMeter ("OUT", [&proc] { return proc.getOutputPeakDb(); })
{
    setHint (trigMeter, "TRIGGER", "Detector level after the trigger filter. Drag the red line to set the Threshold.");
    addAndMakeVisible (trigMeter);
    setHint (redMeter, "REDUCTION", "How much bleed is being cancelled right now.");
    addAndMakeVisible (redMeter);
    setHint (fader, "AMOUNT", "How much bleed is removed when the gate is closed.");
    addAndMakeVisible (fader);
    amountVal.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (amountVal);
    amtAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (ParamIDs::intensity),
        [this] (float v) { amountVal.setText (juce::String ((int) std::round (v)) + " %", juce::dontSendNotification); });
    amtAtt->sendInitialUpdate();
    setHint (outMeter, "OUT", "Plugin output level.");
    addAndMakeVisible (outMeter);

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
    r.removeFromBottom (sc (10));

    // centred row: TRIGGER · REDUCTION · AMOUNT · OUT
    const int mw = sc (46), fw = sc (62), gap = sc (14);
    const int total = mw * 3 + fw + gap * 3;
    auto row = r.withSizeKeepingCentre (juce::jmin (total, r.getWidth()), r.getHeight());

    trigMeter.setBounds (row.removeFromLeft (mw));
    row.removeFromLeft (gap);
    redMeter.setBounds (row.removeFromLeft (mw));
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
