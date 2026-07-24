#pragma once
/*  ThemeColors.h — single source of colour truth, values from the UI mockup
    (index.html :root / .light). Band order: 0=LP(HPF) 1=HP(LPF) 2..6=K1..K5. */
#include <JuceHeader.h>

namespace theme
{
struct Palette
{
    juce::Colour bg, panel, panel2, line;
    juce::Colour txt, dim, faint;
    juce::Colour accent, open, tail, warn, gold, avg;
    juce::Colour knob, knobEdge, btn, btnHover, btnOn, btnOnText;
    juce::Colour band[7];
};

inline const Palette& dark()
{
    static const Palette p = []{
        Palette d;
        d.bg      = juce::Colour (0xff0d1117);
        d.panel   = juce::Colour (0xff161b23);
        d.panel2  = juce::Colour (0xff1b2129);
        d.line    = juce::Colour (0xff252d38);
        d.txt     = juce::Colour (0xffdde5ee);
        d.dim     = juce::Colour (0xff8592a3);
        d.faint   = juce::Colour (0xff5b6675);
        d.accent  = juce::Colour (0xff4aa8e0);
        d.open    = juce::Colour (0xff5ec27a);
        d.tail    = juce::Colour (0xffe0a84a);
        d.warn    = juce::Colour (0xffe0614a);
        d.gold    = juce::Colour (0xfff5c66b);
        d.avg     = juce::Colour (0xff9b7de8);    // Average history trace (violet)
        d.knob    = juce::Colour (0xff222a34);
        d.knobEdge= juce::Colour (0xff333d4a);
        d.btn     = juce::Colour (0xff222a34);
        d.btnHover= juce::Colour (0xff2c3641);
        d.btnOn   = juce::Colour (0xff2f4a63);
        d.btnOnText = juce::Colours::white;
        const juce::uint32 bc[7] = { 0xff4aa8e0, 0xff9b7de8, 0xffe0a84a, 0xffe07a4a,
                                     0xffe04a7a, 0xff4ae0b0, 0xff9ae04a };
        for (int i = 0; i < 7; ++i) d.band[i] = juce::Colour (bc[i]);
        return d;
    }();
    return p;
}

inline const Palette& light()
{
    static const Palette p = []{
        Palette l = dark();                       // accent/open/tail/warn/gold/bands unchanged
        l.bg      = juce::Colour (0xffe8ecf1);
        l.panel   = juce::Colour (0xfff4f6f9);
        l.panel2  = juce::Colour (0xffdfe5ec);
        l.line    = juce::Colour (0xffb9c3cf);
        l.txt     = juce::Colour (0xff1c2531);
        l.dim     = juce::Colour (0xff5d6a7a);
        l.faint   = juce::Colour (0xff8894a4);
        l.knob    = juce::Colour (0xffd4dbe4);
        l.knobEdge= juce::Colour (0xffaab6c4);
        l.btn     = juce::Colour (0xffd9e0e8);
        l.btnHover= juce::Colour (0xffd2dae3);
        l.btnOn   = juce::Colour (0xffa9cbe6);
        l.btnOnText = juce::Colour (0xff1c2531);
        return l;
    }();
    return p;
}

inline const Palette& get (bool darkMode)   { return darkMode ? dark() : light(); }
} // namespace theme
