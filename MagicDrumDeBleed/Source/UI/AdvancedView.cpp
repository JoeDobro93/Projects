#include "AdvancedView.h"

using namespace ui;

AdvancedView::AdvancedView (MagicDrumDeBleedAudioProcessor& proc,
                            std::function<void()> onThemeToggle,
                            std::function<void()> onSimpleView)
    : processor (proc), presetBrowser (proc),
      trigger (proc), gate (proc), tail (proc), rail (proc)
{
    setHint (presetBrowser, "Presets",
             juce::String::fromUTF8 ("Factory starting points per drum. Both Thresholds, Ratio, Selectivity, Hysteresis and the MIDI toggle are deliberately never touched by presets \xe2\x80\x94 they depend on your track, not the drum."));
    addAndMakeVisible (presetBrowser);

    themeBtn.setButtonText (processor.isDarkTheme() ? "Light" : "Dark");
    setHint (themeBtn, "Theme", "Switch between the dark and light theme.");
    themeBtn.onClick = [cb = std::move (onThemeToggle)] { if (cb) cb(); };
    addAndMakeVisible (themeBtn);

    addAndMakeVisible (trigger);
    addAndMakeVisible (gate);
    addAndMakeVisible (tail);
    addAndMakeVisible (rail);

    hintBar.simpleBtn.onClick = [cb = std::move (onSimpleView)] { if (cb) cb(); };
    addAndMakeVisible (hintBar);
}

void AdvancedView::paint (juce::Graphics& g)
{
    g.fillAll (pal->bg);
    const int hh = sc (46);
    g.setColour (pal->panel);
    g.fillRect (0, 0, getWidth(), hh);
    g.setColour (pal->line);
    g.fillRect (0, hh - 1, getWidth(), 1);
    drawLogo (g);
}

void AdvancedView::resized()
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (sc (46));
    header.reduce (sc (12), sc (8));
    themeBtn.setBounds (header.removeFromRight (sc (56)));
    header.removeFromRight (sc (8));
    presetBrowser.setBounds (header.removeFromRight (sc (330)));

    hintBar.setBounds (r.removeFromBottom (sc (34)));
    rail.setBounds (r.removeFromRight (sc (190)));

    r.reduce (sc (9), sc (9));
    const int gap = sc (9);
    trigger.setBounds (r.removeFromTop (sc (164)));
    r.removeFromTop (gap);
    gate.setBounds (r.removeFromTop (sc (208)));
    r.removeFromTop (gap);
    tail.setBounds (r);                                 // stretches
}
