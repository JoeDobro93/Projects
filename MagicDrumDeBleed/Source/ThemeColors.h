#pragma once

/*
    ThemeColors.h — the single, centralized colour palette for the whole plugin.

    Every colour used anywhere in the UI is defined here, once per theme.
    Switching between the Dark and Light theme swaps the returned Palette
    only — no component does any colour math of its own.

    Band colour order: [0]=HPF, [1]=LPF, [2..6]=Notch bands 1..5.
*/

#include <JuceHeader.h>

namespace theme
{

struct Palette
{
    // Window / panels
    juce::Colour windowBackground;
    juce::Colour headerBackground;
    juce::Colour panelBackground;
    juce::Colour panelOutline;

    // Text
    juce::Colour text;
    juce::Colour textDim;
    juce::Colour title;

    // Knobs / sliders
    juce::Colour knobTrack;      // unfilled arc
    juce::Colour knobFill;       // filled arc
    juce::Colour knobPointer;
    juce::Colour knobBody;
    juce::Colour sliderTrack;
    juce::Colour sliderThumb;

    // Buttons
    juce::Colour buttonOff;
    juce::Colour buttonOn;
    juce::Colour buttonText;
    juce::Colour buttonTextOn;
    juce::Colour buttonOutline;

    // Gain-reduction meter
    juce::Colour meterBackground;
    juce::Colour meterFill;
    juce::Colour meterPeak;
    juce::Colour meterTicks;

    // Spectrum display
    juce::Colour spectrumBackground;
    juce::Colour spectrumGrid;
    juce::Colour spectrumLine;
    juce::Colour spectrumFill;
    juce::Colour spectrumAccum;   // accumulated (peak-hold) layer
    juce::Colour spectrumFrozen;  // tint used while frozen

    // EQ curve overlay + band solo
    juce::Colour eqCurve;
    juce::Colour soloActive;

    // Combo boxes / popups / text editors
    juce::Colour comboBackground;
    juce::Colour popupBackground;
    juce::Colour highlight;

    // EQ band colours — distinct, readable in both themes.
    juce::Colour bandColours[7];
};

inline const Palette& dark()
{
    static const Palette p = []
    {
        Palette d;
        d.windowBackground   = juce::Colour (0xff14161a);
        d.headerBackground   = juce::Colour (0xff1b1e24);
        d.panelBackground    = juce::Colour (0xff1e2128);
        d.panelOutline       = juce::Colour (0xff2c313a);

        d.text               = juce::Colour (0xffd8dce4);
        d.textDim            = juce::Colour (0xff8a919d);
        d.title              = juce::Colour (0xff62d0ff);

        d.knobTrack          = juce::Colour (0xff2c313a);
        d.knobFill           = juce::Colour (0xff62d0ff);
        d.knobPointer        = juce::Colour (0xffe8ecf2);
        d.knobBody           = juce::Colour (0xff262b33);
        d.sliderTrack        = juce::Colour (0xff2c313a);
        d.sliderThumb        = juce::Colour (0xff62d0ff);

        d.buttonOff          = juce::Colour (0xff262b33);
        d.buttonOn           = juce::Colour (0xff2f6e8e);
        d.buttonText         = juce::Colour (0xffb8bfc9);
        d.buttonTextOn       = juce::Colour (0xffffffff);
        d.buttonOutline      = juce::Colour (0xff3a404b);

        d.meterBackground    = juce::Colour (0xff15181d);
        d.meterFill          = juce::Colour (0xfff2a93b);
        d.meterPeak          = juce::Colour (0xffff5d5d);
        d.meterTicks         = juce::Colour (0xff525a66);

        d.spectrumBackground = juce::Colour (0xff15181d);
        d.spectrumGrid       = juce::Colour (0xff262b33);
        d.spectrumLine       = juce::Colour (0xff62d0ff);
        d.spectrumFill       = juce::Colour (0x3362d0ff);
        d.spectrumAccum      = juce::Colour (0x66f2a93b);
        d.spectrumFrozen     = juce::Colour (0x22ffffff);

        d.eqCurve            = juce::Colour (0xffe8ecf2);
        d.soloActive         = juce::Colour (0xffe5c07b);

        d.comboBackground    = juce::Colour (0xff262b33);
        d.popupBackground    = juce::Colour (0xff1e2128);
        d.highlight          = juce::Colour (0xff2f6e8e);

        d.bandColours[0]     = juce::Colour (0xffe06c75); // HPF   — red
        d.bandColours[1]     = juce::Colour (0xff61afef); // LPF   — blue
        d.bandColours[2]     = juce::Colour (0xffe5c07b); // B1    — amber
        d.bandColours[3]     = juce::Colour (0xff98c379); // B2    — green
        d.bandColours[4]     = juce::Colour (0xffc678dd); // B3    — purple
        d.bandColours[5]     = juce::Colour (0xff56b6c2); // B4    — teal
        d.bandColours[6]     = juce::Colour (0xffd19a66); // B5    — orange
        return d;
    }();
    return p;
}

inline const Palette& light()
{
    static const Palette p = []
    {
        Palette l;
        l.windowBackground   = juce::Colour (0xffe9ebee);
        l.headerBackground   = juce::Colour (0xffdde0e5);
        l.panelBackground    = juce::Colour (0xfff4f5f7);
        l.panelOutline       = juce::Colour (0xffc6cbd3);

        l.text               = juce::Colour (0xff23272e);
        l.textDim            = juce::Colour (0xff6b7280);
        l.title              = juce::Colour (0xff0f6ea0);

        l.knobTrack          = juce::Colour (0xffd2d6dd);
        l.knobFill           = juce::Colour (0xff0f6ea0);
        l.knobPointer        = juce::Colour (0xff23272e);
        l.knobBody           = juce::Colour (0xffe4e6ea);
        l.sliderTrack        = juce::Colour (0xffd2d6dd);
        l.sliderThumb        = juce::Colour (0xff0f6ea0);

        l.buttonOff          = juce::Colour (0xffe4e6ea);
        l.buttonOn           = juce::Colour (0xff1580ba);
        l.buttonText         = juce::Colour (0xff3d434d);
        l.buttonTextOn       = juce::Colour (0xffffffff);
        l.buttonOutline      = juce::Colour (0xffb9bfc9);

        l.meterBackground    = juce::Colour (0xffdfe2e7);
        l.meterFill          = juce::Colour (0xffe08c00);
        l.meterPeak          = juce::Colour (0xffd94040);
        l.meterTicks         = juce::Colour (0xff9aa1ac);

        l.spectrumBackground = juce::Colour (0xffe2e5ea);
        l.spectrumGrid       = juce::Colour (0xffc9ced6);
        l.spectrumLine       = juce::Colour (0xff0f6ea0);
        l.spectrumFill       = juce::Colour (0x330f6ea0);
        l.spectrumAccum      = juce::Colour (0x66e08c00);
        l.spectrumFrozen     = juce::Colour (0x22000000);

        l.eqCurve            = juce::Colour (0xff23272e);
        l.soloActive         = juce::Colour (0xffe08c00);

        l.comboBackground    = juce::Colour (0xffe4e6ea);
        l.popupBackground    = juce::Colour (0xfff4f5f7);
        l.highlight          = juce::Colour (0xff1580ba);

        l.bandColours[0]     = juce::Colour (0xffc0392b); // HPF   — red
        l.bandColours[1]     = juce::Colour (0xff2471a3); // LPF   — blue
        l.bandColours[2]     = juce::Colour (0xffb8860b); // B1    — amber
        l.bandColours[3]     = juce::Colour (0xff487a2d); // B2    — green
        l.bandColours[4]     = juce::Colour (0xff8e44ad); // B3    — purple
        l.bandColours[5]     = juce::Colour (0xff148f9c); // B4    — teal
        l.bandColours[6]     = juce::Colour (0xffb35c1e); // B5    — orange
        return l;
    }();
    return p;
}

inline const Palette& get (bool darkMode)   { return darkMode ? dark() : light(); }

} // namespace theme
